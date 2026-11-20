#include "ml_ec11.h"

/* Quadrature transition table.
 *
 * Index: (prev_state << 2) | curr_state, where state = (B << 1) | A.
 *
 * Values:
 *   +1  : one edge in clockwise direction
 *   -1  : one edge in counter-clockwise direction
 *    0  : no movement / invalid / bounce
 *
 * This table is derived from standard quadrature decoding patterns and
 * filters out most bouncing / invalid transitions without time-based debounce.
 */
static const int8_t ml_ec11_qdec_table[16] = {/* prev=00 -> curr=00,01,10,11 */
                                              0,
                                              +1,
                                              -1,
                                              0,
                                              /* prev=01 -> curr=00,01,10,11 */
                                              -1,
                                              0,
                                              0,
                                              +1,
                                              /* prev=10 -> curr=00,01,10,11 */
                                              +1,
                                              0,
                                              0,
                                              -1,
                                              /* prev=11 -> curr=00,01,10,11 */
                                              0,
                                              -1,
                                              +1,
                                              0};

/* Clamp helper. */
static inline int32_t ml_ec11_clamp(int32_t v, int32_t min_v, int32_t max_v)
{
    if (v < min_v) return min_v;
    if (v > max_v) return max_v;
    return v;
}

/* Initialize EC11 context. */
void ml_ec11_init(ml_ec11_t *ec,
                  int32_t    min_value,
                  int32_t    max_value,
                  int32_t    step,
                  int32_t    initial_value,
                  uint8_t    edges_per_step)
{
    if (!ec)
    {
        return;
    }

    ec->min_value  = min_value;
    ec->max_value  = max_value;
    ec->step       = (step == 0) ? 1 : step;
    ec->value      = ml_ec11_clamp(initial_value, min_value, max_value);

    /* Initial quadrature state: assume both low. You can override on first call. */
    uint8_t a      = 0;
    uint8_t b      = 0;
    ec->last_state = (uint8_t)((b << 1) | a);
    ec->accum      = 0;

    /* Encoders like EC11 usually generate 4 edges per detent.
     * If you prefer half-step behavior, you can pass 2 here.
     */
    if (edges_per_step == 0)
    {
        ec->edges_per_step = 4;
    }
    else
    {
        ec->edges_per_step = edges_per_step;
    }

    ec->last_timestamp_us      = 0;
    ec->last_step_timestamp_us = 0;

    ec->accel_thresh_mid_us    = 150000 /* e.g. 150 ms */;
    ec->accel_thresh_fast_us   = 60000; /* e.g.  (60 ms)  */
    ec->accel_mul_mid          = 3;     /* e.g. 3 */
    ec->accel_mul_fast         = 10;    /* e.g. 10 */
}

/* Change range at runtime. */
void ml_ec11_set_range(ml_ec11_t *ec, int32_t min_value, int32_t max_value)
{
    if (!ec)
    {
        return;
    }

    ec->min_value = min_value;
    ec->max_value = max_value;
    ec->value     = ml_ec11_clamp(ec->value, min_value, max_value);
}

/* Change logical step at runtime. */
void ml_ec11_set_step(ml_ec11_t *ec, int32_t step)
{
    if (!ec)
    {
        return;
    }

    ec->step = (step == 0) ? 1 : step;
}

/* Reset logical value only. */
void ml_ec11_set_value(ml_ec11_t *ec, int32_t value)
{
    if (!ec)
    {
        return;
    }

    ec->value = ml_ec11_clamp(value, ec->min_value, ec->max_value);
}

void ml_ec11_set_acceleration(ml_ec11_t *ec,
                              bool       acceleration_enabled,
                              int32_t    accel_thresh_mid_us,
                              int32_t    accel_thresh_fast_us,
                              int32_t    accel_mul_mid,
                              int32_t    accel_mul_fast)
{
    ec->acceleration_enabled = acceleration_enabled;
    ec->accel_thresh_mid_us  = (accel_thresh_mid_us == 0) ? 150000 : accel_thresh_mid_us;
    ec->accel_thresh_fast_us = (accel_thresh_fast_us == 0) ? 60000 : accel_thresh_fast_us;
    ec->accel_mul_mid        = (accel_mul_mid == 0) ? 3 : accel_mul_mid;
    ec->accel_mul_fast       = (accel_mul_fast == 0) ? 10 : accel_mul_fast;
}

/* Core quadrature decoder.
 *
 * This function is designed to be extremely lightweight and ISR-safe.
 * It does not allocate memory or perform blocking operations.
 *
 * You are responsible for providing level_a and level_b (0/1) according
 * to your GPIO abstraction (polling or interrupts).
 */
ml_ec11_event_e ml_ec11_process(ml_ec11_t *ec, uint8_t level_a, uint8_t level_b, uint64_t timestamp_us)
{
    if (!ec)
    {
        return EC11_EVENT_NONE;
    }

    ec->last_timestamp_us = timestamp_us;

    /* Compute current 2-bit state from levels. */
    uint8_t curr_state    = (uint8_t)(((level_b & 0x01u) << 1) | (level_a & 0x01u));

    /* Fast path: no change in state. */
    if (curr_state == ec->last_state)
    {
        return EC11_EVENT_NONE;
    }

    /* Lookup delta from state transition table. */
    uint8_t index  = (uint8_t)((ec->last_state << 2) | curr_state);
    int8_t  delta  = ml_ec11_qdec_table[index];

    /* Update last_state regardless of delta. */
    ec->last_state = curr_state;

    if (delta == 0)
    {
        /* Invalid transition / bounce / no movement. Ignore. */
        return EC11_EVENT_NONE;
    }

    /* Accumulate edges in signed accumulator. */
    ec->accum += delta;

    ml_ec11_event_e event = EC11_EVENT_NONE;

    uint64_t        dt    = timestamp_us - ec->last_step_timestamp_us;
    int32_t         mul   = 1;

    /* handle acceleration factor */
    if (ec->acceleration_enabled)
    {
        if (dt <= ec->accel_thresh_fast_us)
        {
            mul = ec->accel_mul_fast;
        }
        else if (dt <= ec->accel_thresh_mid_us)
        {
            mul = ec->accel_mul_mid;
        }
    }

    /* When accumulator reaches +/- edges_per_step, we have one logical step. */
    if (ec->accum >= (int8_t)ec->edges_per_step)
    {
        /* Clockwise step. */
        ec->accum                  = 0;

        int32_t new_value          = ec->value + (ec->step * mul);
        ec->value                  = ml_ec11_clamp(new_value, ec->min_value, ec->max_value);

        ec->last_step_timestamp_us = timestamp_us;
        event                      = EC11_EVENT_ROTATE_RIGHT;
    }
    else if (ec->accum <= -(int8_t)ec->edges_per_step)
    {
        /* Counter-clockwise step. */
        ec->accum                  = 0;

        int32_t new_value          = ec->value - (ec->step * mul);
        ec->value                  = ml_ec11_clamp(new_value, ec->min_value, ec->max_value);

        ec->last_step_timestamp_us = timestamp_us;
        event                      = EC11_EVENT_ROTATE_LEFT;
    }

    return event;
}
