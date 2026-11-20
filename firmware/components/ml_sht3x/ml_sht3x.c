#include "ml_sht3x.h"
#include <string.h>

/* SHT3x commands (SHT3x-DIS datasheet) */
#define SHT3X_CMD_SOFT_RESET            0x30A2
#define SHT3X_CMD_CLEAR_STATUS          0x3041
#define SHT3X_CMD_READ_STATUS           0xF32D

/* Single-shot, clock stretching disabled */
#define SHT3X_CMD_SINGLE_HIGH_CS_OFF    0x2400
#define SHT3X_CMD_SINGLE_MED_CS_OFF     0x240B
#define SHT3X_CMD_SINGLE_LOW_CS_OFF     0x2416

/* Single-shot, clock stretching enabled (for reference) */
#define SHT3X_CMD_SINGLE_HIGH_CS_ON     0x2C06
#define SHT3X_CMD_SINGLE_MED_CS_ON      0x2C0D
#define SHT3X_CMD_SINGLE_LOW_CS_ON      0x2C10

/* Alert Mode commands (read) */
#define SHT3X_CMD_READ_ALRT_HIGH_SET    0xE11F
#define SHT3X_CMD_READ_ALRT_HIGH_CLEAR  0xE114
#define SHT3X_CMD_READ_ALRT_LOW_CLEAR   0xE109
#define SHT3X_CMD_READ_ALRT_LOW_SET     0xE102

/* Alert Mode commands (write) */
#define SHT3X_CMD_WRITE_ALRT_HIGH_SET   0x611D
#define SHT3X_CMD_WRITE_ALRT_HIGH_CLEAR 0x6116
#define SHT3X_CMD_WRITE_ALRT_LOW_CLEAR  0x610B
#define SHT3X_CMD_WRITE_ALRT_LOW_SET    0x6100

/* Periodic mode: frequency / repeatability (clock stretching unavailable) */
/* 0.5 mps */
#define SHT3X_CMD_PER_0_5_H             0x2032
#define SHT3X_CMD_PER_0_5_M             0x2024
#define SHT3X_CMD_PER_0_5_L             0x202F
/* 1 mps */
#define SHT3X_CMD_PER_1_H               0x2130
#define SHT3X_CMD_PER_1_M               0x2120
#define SHT3X_CMD_PER_1_L               0x212D
/* 2 mps */
#define SHT3X_CMD_PER_2_H               0x2236
#define SHT3X_CMD_PER_2_M               0x2220
#define SHT3X_CMD_PER_2_L               0x222B
/* 4 mps */
#define SHT3X_CMD_PER_4_H               0x2334
#define SHT3X_CMD_PER_4_M               0x2322
#define SHT3X_CMD_PER_4_L               0x2329
/* 10 mps */
#define SHT3X_CMD_PER_10_H              0x2737
#define SHT3X_CMD_PER_10_M              0x2722
#define SHT3X_CMD_PER_10_L              0x272A

/* Fetch data / break command / heater */
#define SHT3X_CMD_FETCH_DATA            0xE000
#define SHT3X_CMD_BREAK                 0x3093
#define SHT3X_CMD_HEATER_ON             0x306D
#define SHT3X_CMD_HEATER_OFF            0x3066

/* Measurement durations (typical + margin) in ms */
#define SHT3X_DELAY_HIGH_MS             20
#define SHT3X_DELAY_MED_MS              10
#define SHT3X_DELAY_LOW_MS              5

// -----------------------------------------------------------------------------
// Internal helpers
// -----------------------------------------------------------------------------

static uint16_t sht3x_get_single_shot_cmd(ml_sht3x_repeat_t repeat, bool clock_stretch)
{
    if (clock_stretch)
    {
        switch (repeat)
        {
            case ML_SHT3X_REPEAT_LOW:
                return SHT3X_CMD_SINGLE_LOW_CS_ON;
            case ML_SHT3X_REPEAT_MEDIUM:
                return SHT3X_CMD_SINGLE_MED_CS_ON;
            case ML_SHT3X_REPEAT_HIGH:
            default:
                return SHT3X_CMD_SINGLE_HIGH_CS_ON;
        }
    }
    else
    {
        switch (repeat)
        {
            case ML_SHT3X_REPEAT_LOW:
                return SHT3X_CMD_SINGLE_LOW_CS_OFF;
            case ML_SHT3X_REPEAT_MEDIUM:
                return SHT3X_CMD_SINGLE_MED_CS_OFF;
            case ML_SHT3X_REPEAT_HIGH:
            default:
                return SHT3X_CMD_SINGLE_HIGH_CS_OFF;
        }
    }
}

static uint32_t sht3x_get_delay_ms(ml_sht3x_repeat_t repeat)
{
    switch (repeat)
    {
        case ML_SHT3X_REPEAT_LOW:
            return SHT3X_DELAY_LOW_MS;
        case ML_SHT3X_REPEAT_MEDIUM:
            return SHT3X_DELAY_MED_MS;
        case ML_SHT3X_REPEAT_HIGH:
        default:
            return SHT3X_DELAY_HIGH_MS;
    }
}

/* Select the periodic command based on frequency / repeatability. */
static uint16_t sht3x_get_periodic_cmd(ml_sht3x_repeat_t repeat, ml_sht3x_freq_t freq)
{
    switch (freq)
    {
        case ML_SHT3X_FREQ_0_5:
            switch (repeat)
            {
                case ML_SHT3X_REPEAT_LOW:
                    return SHT3X_CMD_PER_0_5_L;
                case ML_SHT3X_REPEAT_MEDIUM:
                    return SHT3X_CMD_PER_0_5_M;
                case ML_SHT3X_REPEAT_HIGH:
                default:
                    return SHT3X_CMD_PER_0_5_H;
            }
        case ML_SHT3X_FREQ_1:
            switch (repeat)
            {
                case ML_SHT3X_REPEAT_LOW:
                    return SHT3X_CMD_PER_1_L;
                case ML_SHT3X_REPEAT_MEDIUM:
                    return SHT3X_CMD_PER_1_M;
                case ML_SHT3X_REPEAT_HIGH:
                default:
                    return SHT3X_CMD_PER_1_H;
            }
        case ML_SHT3X_FREQ_2:
            switch (repeat)
            {
                case ML_SHT3X_REPEAT_LOW:
                    return SHT3X_CMD_PER_2_L;
                case ML_SHT3X_REPEAT_MEDIUM:
                    return SHT3X_CMD_PER_2_M;
                case ML_SHT3X_REPEAT_HIGH:
                default:
                    return SHT3X_CMD_PER_2_H;
            }
        case ML_SHT3X_FREQ_4:
            switch (repeat)
            {
                case ML_SHT3X_REPEAT_LOW:
                    return SHT3X_CMD_PER_4_L;
                case ML_SHT3X_REPEAT_MEDIUM:
                    return SHT3X_CMD_PER_4_M;
                case ML_SHT3X_REPEAT_HIGH:
                default:
                    return SHT3X_CMD_PER_4_H;
            }
        case ML_SHT3X_FREQ_10:
        default:
            switch (repeat)
            {
                case ML_SHT3X_REPEAT_LOW:
                    return SHT3X_CMD_PER_10_L;
                case ML_SHT3X_REPEAT_MEDIUM:
                    return SHT3X_CMD_PER_10_M;
                case ML_SHT3X_REPEAT_HIGH:
                default:
                    return SHT3X_CMD_PER_10_H;
            }
    }
}

static bool sht3x_send_cmd(ml_sht3x_t *dev, uint16_t cmd)
{
    if (!dev)
    {
        return false;
    }

    /* The SHT3x expects 16-bit commands in big-endian order (MSB first) */
    uint8_t tx_buf[2];
    tx_buf[0] = (uint8_t)(cmd >> 8);   // MSB
    tx_buf[1] = (uint8_t)(cmd & 0xFF); // LSB

    /* Raw atomic 2-byte I2C transaction */
    return ml_i2c_write_only(&dev->i2c, tx_buf, 2);
}

/**
 * Reads a 6-byte block (T, CRC_T, RH, CRC_RH) and returns the raw values.
 */
static bool sht3x_read_measurement_block(ml_sht3x_t *dev, uint16_t *raw_temp, uint16_t *raw_rh)
{
    if (!dev || !raw_temp || !raw_rh)
    {
        return false;
    }

    uint8_t buf[6] = {0};

    if (!ml_i2c_read_only(&dev->i2c, buf, sizeof(buf)))
    {
        return false;
    }

    /* Temperature CRC check */
    if (ml_sht3x_crc8(&buf[0], 2) != buf[2])
    {
        return false;
    }
    /* Humidity CRC check */
    if (ml_sht3x_crc8(&buf[3], 2) != buf[5])
    {
        return false;
    }

    *raw_temp = (uint16_t)((buf[0] << 8) | buf[1]);
    *raw_rh   = (uint16_t)((buf[3] << 8) | buf[4]);

    return true;
}

// -----------------------------------------------------------------------------
// CRC-8 Sensirion
// -----------------------------------------------------------------------------

uint8_t ml_sht3x_crc8(const uint8_t *data, size_t len)
{
    uint8_t crc = 0xFF; /* init */

    for (size_t i = 0; i < len; i++)
    {
        crc ^= data[i];
        for (uint8_t bit = 0; bit < 8; bit++)
        {
            if (crc & 0x80)
            {
                crc = (uint8_t)((crc << 1) ^ 0x31);
            }
            else
            {
                crc <<= 1;
            }
        }
    }

    return crc;
}

// -----------------------------------------------------------------------------
// Public API: init / reset
// -----------------------------------------------------------------------------

bool ml_sht3x_init(ml_sht3x_t       *dev,
                   ml_i2c_port_t     i2c_port,
                   uint8_t           i2c_address,
                   ml_sht3x_repeat_t repeat,
                   bool              clock_stretch)
{
    if (!dev)
    {
        return false;
    }

    memset(dev, 0, sizeof(*dev));
    dev->i2c.i2c_port  = i2c_port;
    dev->i2c.i2c_addr  = i2c_address;
    dev->repeat        = repeat;
    dev->clock_stretch = clock_stretch;
    dev->mode          = ML_SHT3X_MODE_SINGLE;
    dev->freq          = ML_SHT3X_FREQ_1;
    dev->last_valid    = false;

    if (!ml_i2c_device_exists(&dev->i2c))
    {
        return false;
    }

    /* Clear status + soft reset are recommended at startup.*/
    (void)sht3x_send_cmd(dev, SHT3X_CMD_CLEAR_STATUS);
    ml_delay_ms(2);
    (void)sht3x_send_cmd(dev, SHT3X_CMD_SOFT_RESET);
    ml_delay_ms(2);

    return true;
}

bool ml_sht3x_soft_reset(ml_sht3x_t *dev)
{
    if (!dev)
    {
        return false;
    }

    if (!sht3x_send_cmd(dev, SHT3X_CMD_SOFT_RESET))
    {
        return false;
    }

    ml_delay_ms(2);
    dev->last_valid = false;
    dev->mode       = ML_SHT3X_MODE_SINGLE;

    return true;
}

bool ml_sht3x_exists(ml_sht3x_t *dev)
{
    return ml_i2c_device_exists(&dev->i2c);
}

bool ml_sht3x_set_repeatability(ml_sht3x_t *dev, ml_sht3x_repeat_t repeat)
{
    if (!dev)
    {
        return false;
    }

    /* If the value is already correct, do nothing */
    if (dev->repeat == repeat)
    {
        return true;
    }

    if (dev->mode == ML_SHT3X_MODE_SINGLE)
    {
        /* In single-shot mode, just update the context.
           The next read will automatically use the correct command and delay. */
        dev->repeat = repeat;
        return true;
    }
    else if (dev->mode == ML_SHT3X_MODE_PERIODIC)
    {
        /* In periodic mode, send a BREAK, change the configuration, and restart. */
        ml_sht3x_freq_t current_freq = dev->freq;

        if (!ml_sht3x_stop_periodic(dev))
        {
            return false;
        }

        return ml_sht3x_start_periodic(dev, repeat, current_freq);
    }

    return false;
}

void ml_sht3x_set_offset(ml_sht3x_t *dev, float temp_offset, float hum_offset)
{
    if (dev)
    {
        dev->temp_offset = temp_offset;
        dev->hum_offset  = hum_offset;
    }
}

// -----------------------------------------------------------------------------
// Single-shot
// -----------------------------------------------------------------------------

bool ml_sht3x_read_raw(ml_sht3x_t *dev, uint16_t *raw_temp, uint16_t *raw_rh)
{
    if (!dev || !raw_temp || !raw_rh)
    {
        return false;
    }

    dev->mode    = ML_SHT3X_MODE_SINGLE;

    uint16_t cmd = sht3x_get_single_shot_cmd(dev->repeat, dev->clock_stretch);

    if (!sht3x_send_cmd(dev, cmd))
    {
        return false;
    }

    ml_delay_ms(sht3x_get_delay_ms(dev->repeat));

    return sht3x_read_measurement_block(dev, raw_temp, raw_rh);
}

bool ml_sht3x_read(ml_sht3x_t *dev, float *temperature, float *humidity)
{
    if (!dev)
    {
        return false;
    }

    uint16_t raw_t = 0;
    uint16_t raw_r = 0;

    if (!ml_sht3x_read_raw(dev, &raw_t, &raw_r))
    {
        return false;
    }

    /* Formulas from the datasheet */
    float rh = 100.0f * ((float)raw_r / 65535.0f);
    float t  = -45.0f + (175.0f * ((float)raw_t / 65535.0f));

    /* apply offset */
    t += dev->temp_offset;
    rh += dev->hum_offset;

    if (rh < 0.0f) rh = 0.0f;
    if (rh > 100.0f) rh = 100.0f;

    if (temperature) *temperature = t;
    if (humidity) *humidity = rh;

    dev->last_temperature = t;
    dev->last_humidity    = rh;
    dev->last_valid       = true;

    return true;
}

// -----------------------------------------------------------------------------
// Status & heater
// -----------------------------------------------------------------------------

bool ml_sht3x_read_status_raw(ml_sht3x_t *dev, uint16_t *status)
{
    if (!dev || !status)
    {
        return false;
    }

    if (!sht3x_send_cmd(dev, SHT3X_CMD_READ_STATUS))
    {
        return false;
    }

    uint8_t buf[3] = {0};

    if (!ml_i2c_read_only(&dev->i2c, buf, sizeof(buf)))
    {
        return false;
    }

    if (ml_sht3x_crc8(buf, 2) != buf[2])
    {
        return false;
    }

    *status = (uint16_t)((buf[0] << 8) | buf[1]);
    return true;
}

bool ml_sht3x_read_status(ml_sht3x_t *dev, ml_sht3x_status_t *st)
{
    if (!dev || !st)
    {
        return false;
    }

    uint16_t raw = 0;
    if (!ml_sht3x_read_status_raw(dev, &raw))
    {
        return false;
    }

    memset(st, 0, sizeof(*st));
    st->raw             = raw;
    st->alert_pending   = (raw & (1u << 15)) != 0;
    st->heater_on       = (raw & (1u << 13)) != 0;
    st->rh_alert        = (raw & (1u << 11)) != 0;
    st->t_alert         = (raw & (1u << 10)) != 0;
    st->system_reset    = (raw & (1u << 4)) != 0;
    st->cmd_error       = (raw & (1u << 1)) != 0;
    st->write_crc_error = (raw & (1u << 0)) != 0;
    /* The other bits are reserved / unused here */

    return true;
}

bool ml_sht3x_clear_status(ml_sht3x_t *dev)
{
    if (!dev)
    {
        return false;
    }

    if (!sht3x_send_cmd(dev, SHT3X_CMD_CLEAR_STATUS))
    {
        return false;
    }

    ml_delay_ms(2);
    return true;
}

bool ml_sht3x_set_heater(ml_sht3x_t *dev, bool enable)
{
    if (!dev)
    {
        return false;
    }

    uint16_t cmd = enable ? SHT3X_CMD_HEATER_ON : SHT3X_CMD_HEATER_OFF;

    if (!sht3x_send_cmd(dev, cmd))
    {
        return false;
    }

    /* The heater does not require a specific delay here, but you can wait
       a little before reading T/RH to see the effect */
    return true;
}

// -----------------------------------------------------------------------------
// Periodic mode
// -----------------------------------------------------------------------------

bool ml_sht3x_start_periodic(ml_sht3x_t *dev, ml_sht3x_repeat_t repeat, ml_sht3x_freq_t freq)
{
    if (!dev)
    {
        return false;
    }

    uint16_t cmd = sht3x_get_periodic_cmd(repeat, freq);

    if (!sht3x_send_cmd(dev, cmd))
    {
        return false;
    }

    dev->mode       = ML_SHT3X_MODE_PERIODIC;
    dev->repeat     = repeat;
    dev->freq       = freq;
    dev->last_valid = false;

    /* Datasheet: wait at least one measurement duration before the
       first fetch */
    ml_delay_ms(sht3x_get_delay_ms(repeat));

    return true;
}

bool ml_sht3x_fetch(ml_sht3x_t *dev, float *temperature, float *humidity)
{
    if (!dev)
    {
        return false;
    }

    if (dev->mode != ML_SHT3X_MODE_PERIODIC)
    {
        /* For safety, we could also fall back to a single-shot read. */
        return false;
    }

    /* 1) Fetch Data command */
    if (!sht3x_send_cmd(dev, SHT3X_CMD_FETCH_DATA))
    {
        return false;
    }

    /* 2) Read block */
    uint16_t raw_t = 0;
    uint16_t raw_r = 0;

    if (!sht3x_read_measurement_block(dev, &raw_t, &raw_r))
    {
        return false;
    }

    /* 3) Conversion (same as ml_sht3x_read) */
    float rh = 100.0f * ((float)raw_r / 65535.0f);
    float t  = -45.0f + (175.0f * ((float)raw_t / 65535.0f));

    if (rh < 0.0f) rh = 0.0f;
    if (rh > 100.0f) rh = 100.0f;

    if (temperature) *temperature = t;
    if (humidity) *humidity = rh;

    dev->last_temperature = t;
    dev->last_humidity    = rh;
    dev->last_valid       = true;

    return true;
}

bool ml_sht3x_stop_periodic(ml_sht3x_t *dev)
{
    if (!dev)
    {
        return false;
    }

    /* Break command to return to single-shot mode */
    if (!sht3x_send_cmd(dev, SHT3X_CMD_BREAK))
    {
        return false;
    }

    ml_delay_ms(2);
    dev->mode = ML_SHT3X_MODE_SINGLE;
    return true;
}

// -----------------------------------------------------------------------------
// Alert Mode / Limits
// -----------------------------------------------------------------------------

/* Helper: converts floats to a compressed 16-bit word (7 RH bits | 9 T bits) */
static uint16_t sht3x_limit_to_raw16(float t, float rh)
{
    /* Clamp values */
    if (t < -45.0f) t = -45.0f;
    if (t > 130.0f) t = 130.0f;
    if (rh < 0.0f) rh = 0.0f;
    if (rh > 100.0f) rh = 100.0f;

    /* Convert to full 16-bit integers */
    uint32_t raw_t  = (uint32_t)(((t + 45.0f) / 175.0f) * 65535.0f);
    uint32_t raw_rh = (uint32_t)((rh / 100.0f) * 65535.0f);

    /* Build the reduced format:
       RH uses the 7 MSBs, T uses the 9 MSBs; combine them. */
    return (uint16_t)((raw_rh & 0xFE00) | ((raw_t >> 7) & 0x01FF));
}

/* Helper: extracts floats from the compressed 16-bit word */
static void sht3x_raw16_to_limit(uint16_t raw16, float *t, float *rh)
{
    /* Reconstruct the full 16-bit values (with intentional loss of resolution) */
    uint16_t raw_rh = raw16 & 0xFE00;
    uint16_t raw_t  = (raw16 & 0x01FF) << 7;

    if (t)
    {
        *t = -45.0f + (175.0f * (float)raw_t / 65535.0f);
    }
    if (rh)
    {
        *rh = 100.0f * (float)raw_rh / 65535.0f;
    }
}

bool ml_sht3x_set_alert_limit(ml_sht3x_t *dev, ml_sht3x_limit_t limit, float temperature, float humidity)
{
    if (!dev)
    {
        return false;
    }

    uint16_t cmd;
    switch (limit)
    {
        case ML_SHT3X_LIMIT_HIGH_SET:
            cmd = SHT3X_CMD_WRITE_ALRT_HIGH_SET;
            break;
        case ML_SHT3X_LIMIT_HIGH_CLEAR:
            cmd = SHT3X_CMD_WRITE_ALRT_HIGH_CLEAR;
            break;
        case ML_SHT3X_LIMIT_LOW_CLEAR:
            cmd = SHT3X_CMD_WRITE_ALRT_LOW_CLEAR;
            break;
        case ML_SHT3X_LIMIT_LOW_SET:
            cmd = SHT3X_CMD_WRITE_ALRT_LOW_SET;
            break;
        default:
            return false;
    }

    /* 1) Calculate the 16-bit word */
    uint16_t raw16         = sht3x_limit_to_raw16(temperature, humidity);

    /* 2) Prepare the bytes (big-endian) for CRC calculation */
    uint8_t  data_bytes[2] = {
        (uint8_t)(raw16 >> 8),  // Data MSB
        (uint8_t)(raw16 & 0xFF) // Data LSB
    };
    uint8_t crc = ml_sht3x_crc8(data_bytes, 2);

    /* 3) Assemble the final 5-byte buffer (16-bit command + 16-bit data + 8-bit CRC) */
    uint8_t tx_buf[5];
    tx_buf[0] = (uint8_t)(cmd >> 8);   // Command MSB
    tx_buf[1] = (uint8_t)(cmd & 0xFF); // Command LSB
    tx_buf[2] = data_bytes[0];         // Data MSB
    tx_buf[3] = data_bytes[1];         // Data LSB
    tx_buf[4] = crc;                   // Data CRC

    /* 4) Send in a single raw I2C frame */
    return ml_i2c_write_only(&dev->i2c, tx_buf, 5);
}

bool ml_sht3x_get_alert_limit(ml_sht3x_t *dev, ml_sht3x_limit_t limit, float *temperature, float *humidity)
{
    if (!dev)
    {
        return false;
    }

    uint16_t cmd;
    switch (limit)
    {
        case ML_SHT3X_LIMIT_HIGH_SET:
            cmd = SHT3X_CMD_READ_ALRT_HIGH_SET;
            break;
        case ML_SHT3X_LIMIT_HIGH_CLEAR:
            cmd = SHT3X_CMD_READ_ALRT_HIGH_CLEAR;
            break;
        case ML_SHT3X_LIMIT_LOW_CLEAR:
            cmd = SHT3X_CMD_READ_ALRT_LOW_CLEAR;
            break;
        case ML_SHT3X_LIMIT_LOW_SET:
            cmd = SHT3X_CMD_READ_ALRT_LOW_SET;
            break;
        default:
            return false;
    }

    /* Send the read command */
    if (!sht3x_send_cmd(dev, cmd)) return false;

    /* Read the 3 bytes (value MSB, value LSB, CRC) */
    uint8_t buf[3] = {0};
    if (!ml_i2c_read_only(&dev->i2c, buf, 3)) return false;

    /* CRC check */
    if (ml_sht3x_crc8(buf, 2) != buf[2]) return false;

    /* Decode */
    uint16_t raw16 = (uint16_t)((buf[0] << 8) | buf[1]);
    sht3x_raw16_to_limit(raw16, temperature, humidity);

    return true;
}
