#pragma once

#include <ml_common.h>
#include <ml_i2c.h>

#ifdef __cplusplus
extern "C" {
#endif

// I2C addresses 7 bits
#define ML_SHT3X_I2C_ADDR_GND 0x44
#define ML_SHT3X_I2C_ADDR_VDD 0x45

/**
 * @brief Repeatability setting for the SHT3x sensor
 *
 */
typedef enum
{
    ML_SHT3X_REPEAT_LOW = 0,
    ML_SHT3X_REPEAT_MEDIUM,
    ML_SHT3X_REPEAT_HIGH
} ml_sht3x_repeat_t;

/**
 * @brief Measurement mode for the SHT3x sensor
 */
typedef enum
{
    ML_SHT3X_MODE_SINGLE = 0,
    ML_SHT3X_MODE_PERIODIC
} ml_sht3x_mode_t;

/**
 * @brief Enumeration representing the measurement frequency for the SHT3x sensor
 */
typedef enum
{
    ML_SHT3X_FREQ_0_5 = 0, // 0.5 measures/s
    ML_SHT3X_FREQ_1,
    ML_SHT3X_FREQ_2,
    ML_SHT3X_FREQ_4,
    ML_SHT3X_FREQ_10
} ml_sht3x_freq_t;

/**
 * @brief convenience structure representing the status register of the SHT3x sensor
 */
typedef struct
{
    uint16_t raw;
    bool     alert_pending;
    bool     heater_on;
    bool     rh_alert;
    bool     t_alert;
    bool     system_reset;
    bool     cmd_error;
    bool     write_crc_error;
} ml_sht3x_status_t;

/**
 * @brief Structure representing the SHT3x device. Should be considered opaque to the user.
 */
typedef struct
{
    ml_i2c_t          i2c;
    ml_sht3x_repeat_t repeat;
    bool              clock_stretch;
    ml_sht3x_mode_t   mode;
    ml_sht3x_freq_t   freq;
    float             temp_offset;
    float             hum_offset;
    bool              last_valid;
    float             last_temperature;
    float             last_humidity;
} ml_sht3x_t;

/**
 * @brief Set temperature and humidity offsets for the sensor readings.
 *
 * @param dev  Pointer to the SHT3x device structure
 * @param temp_offset  Temperature offset to apply to the readings
 * @param hum_offset  Humidity offset to apply to the readings
 */
void ml_sht3x_set_offset(ml_sht3x_t *dev, float temp_offset, float hum_offset);

// --- Init / reset -----------------------------------------------------------

/**
 * @brief Initialize the SHT3x sensor with the specified configuration.
 *
 * @param dev  Pointer to the SHT3x device structure
 * @param i2c_port  I2C port to which the sensor is connected
 * @param i2c_address  I2C address of the sensor
 * @param repeat  Repeatability setting for the sensor
 * @param clock_stretch  Enable or disable clock stretching
 * @return true  If the initialization was successful
 * @return false If the initialization failed
 */
bool ml_sht3x_init(ml_sht3x_t       *dev,
                   ml_i2c_port_t     i2c_port,
                   uint8_t           i2c_address,
                   ml_sht3x_repeat_t repeat,
                   bool              clock_stretch);

/**
 * @brief Perform a soft reset of the SHT3x sensor.
 *
 * @param dev  Pointer to the SHT3x device structure
 * @return true  If the soft reset was successful
 * @return false If the soft reset failed
 */
bool ml_sht3x_soft_reset(ml_sht3x_t *dev);

/**
 * @brief Check if the SHT3x sensor exists and is responsive.
 *
 * @param dev  Pointer to the SHT3x device structure
 * @return true  If the sensor exists and is responsive
 * @return false If the sensor does not exist or is unresponsive
 */
bool ml_sht3x_exists(ml_sht3x_t *dev);

/**
 * @brief Modify the repeatability (precision/power consumption) on the fly.
 *        If the sensor is in periodic mode, it will be automatically
 *        restarted with the new setting.
 * @param dev  Pointer to the SHT3x device structure
 * @param repeat  New repeatability setting for the sensor
 * @return true  If the repeatability was successfully updated
 * @return false If the update failed
 */
bool ml_sht3x_set_repeatability(ml_sht3x_t *dev, ml_sht3x_repeat_t repeat);

// --- Single shot measurement -----------------------------------------------------

/**
 * @brief Read raw temperature and humidity values from the sensor
 *
 * @param dev  Pointer to the SHT3x device structure
 * @param raw_temp  Pointer to store the raw temperature value
 * @param raw_rh  Pointer to store the raw humidity value
 * @return true  If the read operation was successful
 * @return false If the read operation failed
 */
bool ml_sht3x_read_raw(ml_sht3x_t *dev, uint16_t *raw_temp, uint16_t *raw_rh);

/**
 * @brief Read temperature and humidity values from the sensor
 *
 * @param dev  Pointer to the SHT3x device structure
 * @param temperature  Pointer to store the temperature value
 * @param humidity  Pointer to store the humidity value
 * @return true  If the read operation was successful
 * @return false If the read operation failed
 */
bool ml_sht3x_read(ml_sht3x_t *dev, float *temperature, float *humidity);

// --- Status & heater --------------------------------------------------------

/**
 * @brief Read the raw status register from the sensor
 *
 * @param dev  Pointer to the SHT3x device structure
 * @param status  Pointer to store the raw status value
 * @return true  If the read operation was successful
 * @return false If the read operation failed
 */
bool ml_sht3x_read_status_raw(ml_sht3x_t *dev, uint16_t *status);

/**
 * @brief Read the status register from the sensor and parse it into a structured format
 *
 * @param dev  Pointer to the SHT3x device structure
 * @param status  Pointer to store the parsed status value
 * @return true  If the read operation was successful
 * @return false If the read operation failed
 */
bool ml_sht3x_read_status(ml_sht3x_t *dev, ml_sht3x_status_t *status);

/**
 * @brief Clear the status register of the sensor
 *
 * @param dev  Pointer to the SHT3x device structure
 * @return true  If the clear operation was successful
 * @return false If the clear operation failed
 */
bool ml_sht3x_clear_status(ml_sht3x_t *dev);

/**
 * @brief Enable or disable the heater of the sensor
 *
 * @param dev  Pointer to the SHT3x device structure
 * @param enable  true to enable the heater, false to disable it
 * @return true  If the operation was successful
 * @return false If the operation failed
 */
bool ml_sht3x_set_heater(ml_sht3x_t *dev, bool enable);

// --- Periodic Mode ----------------------------------------

/**
 * @brief Start the sensor in periodic mode with the specified repeat and frequency settings
 *
 * @param dev  Pointer to the SHT3x device structure
 * @param repeat  Repeatability setting for the measurements
 * @param freq  Frequency setting for the periodic measurements
 * @return true  If the start operation was successful
 * @return false If the start operation failed
 */
bool ml_sht3x_start_periodic(ml_sht3x_t *dev, ml_sht3x_repeat_t repeat, ml_sht3x_freq_t freq);

/**
 * @brief Fetch the latest temperature and humidity measurements from the sensor in periodic mode
 *
 * @param dev  Pointer to the SHT3x device structure
 * @param temperature  Pointer to store the fetched temperature value
 * @param humidity  Pointer to store the fetched humidity value
 * @return true  If the fetch operation was successful
 * @return false If the fetch operation failed
 */
bool ml_sht3x_fetch(ml_sht3x_t *dev, float *temperature, float *humidity);

/**
 * @brief Stop the sensor from periodic mode
 *
 * @param dev  Pointer to the SHT3x device structure
 * @return true  If the stop operation was successful
 * @return false If the stop operation failed
 */
bool ml_sht3x_stop_periodic(ml_sht3x_t *dev);

// --- Alert Mode ------------------------------------------

typedef enum
{
    ML_SHT3X_LIMIT_HIGH_SET = 0, // High alert threshold
    ML_SHT3X_LIMIT_HIGH_CLEAR,   // High alert clear threshold (hysteresis)
    ML_SHT3X_LIMIT_LOW_CLEAR,    // Low alert clear threshold (hysteresis)
    ML_SHT3X_LIMIT_LOW_SET       // Low alert threshold
} ml_sht3x_limit_t;

/**
 * @brief Set a hardware alert threshold for temperature and humidity.
 *
 * @param dev  Pointer to the SHT3x device structure
 * @param limit  The alert limit to set
 * @param temperature  The temperature threshold for the alert
 * @param humidity  The humidity threshold for the alert
 * @return true  If the alert limit was successfully set
 * @return false If the alert limit setting failed
 * @note: The physical trigger (ALERT pin) requires periodic mode.
 */
bool ml_sht3x_set_alert_limit(ml_sht3x_t *dev, ml_sht3x_limit_t limit, float temperature, float humidity);

/**
 * @brief Get the currently configured hardware alert threshold from the sensor.
 *
 * @param dev  Pointer to the SHT3x device structure
 * @param limit  The alert limit to get
 * @param temperature  Pointer to store the fetched temperature threshold
 * @param humidity  Pointer to store the fetched humidity threshold
 * @return true  If the alert limit was successfully fetched
 * @return false If the alert limit fetching failed
 */
bool ml_sht3x_get_alert_limit(ml_sht3x_t *dev, ml_sht3x_limit_t limit, float *temperature, float *humidity);

// --- Tools -----------------------------------------------------------------

/**
 * @brief Calculate the CRC8 checksum for the given data.
 *
 * @param data  Pointer to the data array
 * @param len  Length of the data array
 * @return uint8_t  The calculated CRC8 checksum
 */
uint8_t ml_sht3x_crc8(const uint8_t *data, size_t len);

#ifdef __cplusplus
}
#endif
