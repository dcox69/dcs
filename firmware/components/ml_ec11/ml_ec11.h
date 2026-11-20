#pragma once

#include <stdbool.h>
#include <stdint.h>

/* Rotary-only events. Button events come from your touchsw component. */
typedef enum
{
    EC11_EVENT_NONE = 0,
    EC11_EVENT_ROTATE_LEFT,
    EC11_EVENT_ROTATE_RIGHT
} ml_ec11_event_e;

/* EC11 context */
typedef struct
{
    /* Logical value management */
    int32_t  min_value;
    int32_t  max_value;
    int32_t  step;  /* Logical step per detent (default 1) */
    int32_t  value; /* Current logical value */

    /* Quadrature decoding */
    uint8_t  last_state;     /* Previous AB state: (B << 1) | A, range 0..3 */
    int8_t   accum;          /* Edge accumulator, e.g. counts -4..+4 per detent */
    uint8_t  edges_per_step; /* Number of valid edges per logical step (2 or 4) */

    /* Timing info (if you want to use it later) */
    uint64_t last_timestamp_us;      /* Last time process() was called */
    uint64_t last_step_timestamp_us; /* Time of last completed step */

    bool     acceleration_enabled;
    uint64_t accel_thresh_mid_us;  /* e.g. 150000 (150 ms) */
    uint64_t accel_thresh_fast_us; /* e.g. 60000  (60 ms)  */
    int32_t  accel_mul_mid;        /* e.g. 3 */
    int32_t  accel_mul_fast;       /* e.g. 10 */
} ml_ec11_t;

/* Initialize EC11 context. */

/**
 * @brief Initialize the EC11 rotary encoder context with specified parameters.
 *
 * @param ec Pointer to EC11 context
 * @param min_value Minimum logical value
 * @param max_value Maximum logical value
 * @param step Logical step per detent (default 1)
 * @param initial_value Initial logical value
 * @param edges_per_step Number of valid edges per logical step (2 or 4)
 */
void ml_ec11_init(ml_ec11_t *ec,
                  int32_t    min_value,
                  int32_t    max_value,
                  int32_t    step,
                  int32_t    initial_value,
                  uint8_t    edges_per_step);

/**
 * @brief Change the logical range of the EC11 encoder at runtime, clamping the current value.
 *
 * @param ec Pointer to EC11 context
 * @param min_value New minimum logical value
 * @param max_value New maximum logical value
 */
void ml_ec11_set_range(ml_ec11_t *ec, int32_t min_value, int32_t max_value);

/**
 * @brief Change the logical step per detent of the EC11 encoder at runtime.
 *
 * @param ec Pointer to EC11 context
 * @param step New logical step per detent (default 1)
 */
void ml_ec11_set_step(ml_ec11_t *ec, int32_t step);

/**
 * @brief Reset the logical value of the EC11 encoder, keeping the decoder state intact.
 *
 * @param ec Pointer to EC11 context
 * @param value New logical value to set
 */
void ml_ec11_set_value(ml_ec11_t *ec, int32_t value);

/**
 * @brief Configure acceleration settings for the EC11 encoder.
 *
 * @param ec Pointer to EC11 context
 * @param acceleration_enabled Enable or disable acceleration
 * @param accel_thresh_mid_us Threshold for mid-speed acceleration in microseconds
 * @param accel_thresh_fast_us Threshold for fast-speed acceleration in microseconds
 * @param accel_mul_mid Multiplier for mid-speed acceleration
 * @param accel_mul_fast Multiplier for fast-speed acceleration
 */
void ml_ec11_set_acceleration(ml_ec11_t *ec,
                              bool       acceleration_enabled,
                              int32_t    accel_thresh_mid_us,
                              int32_t    accel_thresh_fast_us,
                              int32_t    accel_mul_mid,
                              int32_t    accel_mul_fast);

/**
 * @brief Main processing function for the EC11 encoder.
 *
 * @note This function should be called regularly to process encoder events.
 * @param ec Pointer to EC11 context
 * @param level_a Current level of encoder A (0 or 1)
 * @param level_b Current level of encoder B (0 or 1)
 * @param timestamp_us Current timestamp in microseconds
 * @return ML_EC11_EVENT_NONE if nothing changed,
 *         ML_EC11_EVENT_ROTATE_LEFT / RIGHT otherwise.
 *
 * @remark This function is safe to call from both polling loops and interrupt service routines.
 *
 * @note
 * Can be called:
 *  - from a periodic timer (polling)
 *  - or from GPIO interrupts (ISR-safe, no dynamic allocation, no blocking).
 *
 * Parameters:
 *   level_a       : current level of encoder A (0 or 1)
 *   level_b       : current level of encoder B (0 or 1)
 *   timestamp_us  : current timestamp in microseconds
 *
 * Returns:
 *   ML_EC11_EVENT_NONE if nothing changed,
 *   ML_EC11_EVENT_ROTATE_LEFT / RIGHT otherwise.
 *
 * On a non-NONE event, ec->value has already been updated and clamped to range.
 *
 */
ml_ec11_event_e ml_ec11_process(ml_ec11_t *ec, uint8_t level_a, uint8_t level_b, uint64_t timestamp_us);

/* Simple getter for current logical value. */
static inline int32_t ml_ec11_get_value(const ml_ec11_t *ec)
{
    return ec ? ec->value : 0;
}
