/*
 * =============<< ********* >>=============
 * Author       : dcox69 (David GOMEZ)
 * Email        : dcox69@proton.me
 * Organization :
 * Created On   : 2025-08-04
 * Title        :
 * Description  :
 * Copyright (c) 2025 dcox69 (David GOMEZ).
 * =============<< ********* >>=============
 */
#pragma once

#include <ml_common.h>

#if defined(ML_PLATFORM_PICO)
#include "hardware/i2c.h"
#include "hardware/gpio.h"
typedef i2c_inst_t *ml_i2c_port_t;
#elif defined(ML_PLATFORM_ESP)
#include "driver/i2c_master.h"
#include "driver/gpio.h"
typedef i2c_port_t ml_i2c_port_t;
#endif

#if defined(ML_PLATFORM_ESP)
#define I2C_MAX_FRAGMENTS 8

typedef struct
{
    uint8_t *buffer;
    size_t size;
} esp_i2c_fragment_t;
#endif

typedef struct
{
    ml_i2c_port_t i2c_port;
    uint8_t i2c_addr;
    uint8_t last_byte;
    bool last_byte_remain;
#if defined(ML_PLATFORM_ESP)
    i2c_master_dev_handle_t dev; 
    /* array of fragments */
    esp_i2c_fragment_t tx_fragments[I2C_MAX_FRAGMENTS]; 
    size_t tx_frag_count;
    bool tx_in_progress;
    uint8_t tx_reg; 
#endif
} ml_i2c_t;

/**
 * @brief Check if a device is present on the bus
 *
 * @param i2c The I2C device structure
 * @param addr The I2C device address
 * @return true if the device is present
 * @return false if the device is not present or an error occured
 */
bool ml_i2c_device_exists(ml_i2c_t *i2c);

/**
 * @brief Initialize the I2C peripheral with the specified parameters.
 *
 * @param i2c_port The I2C port to initialize (platform-specific).
 * @param speed The I2C bus speed in Hz.
 * @param sda_pin The GPIO pin number for the I2C SDA line.
 * @param scl_pin The GPIO pin number for the I2C SCL line.
 * @param pull_up Enable or disable the internal pull-up resistors for the I2C lines.
 */
void ml_i2c_init(ml_i2c_port_t i2c_port, uint32_t speed, uint8_t sda_pin, uint8_t scl_pin, bool pull_up);

/**
 * @brief Reset the i2c Bus & Initialize it
 *
 * @param i2c_port The I2C port to reset and initialize (platform-specific).
 * @param speed The I2C bus speed in Hz.
 * @param sda_pin The GPIO pin number for the I2C SDA line.
 * @param scl_pin The GPIO pin number for the I2C SCL line.
 * @param pull_up Enable or disable the internal pull-up resistors for the I2C lines.
 */
void ml_i2c_bus_clear(ml_i2c_port_t i2c_port, uint32_t speed, uint8_t sda_pin, uint8_t scl_pin, bool pull_up);

/**
 * @brief Deinitialize the I2C peripheral and release any associated resources.
 *
 * @param i2c_port The I2C port to deinitialize (platform-specific).
 */
void ml_i2c_deinit(ml_i2c_port_t i2c_port);

#if defined(ML_PLATFORM_PICO)
/* THESE FUNCTION ARE DEPRECATED AND WILL BE REMOVED */
/**
 * @brief Get the I2C instance associated with the specified SCL pin.
 *
 * @param pin_scl The GPIO pin number for the I2C SCL line.
 * @return i2c_inst_t* The I2C instance associated with the specified SCL pin.
 */
i2c_inst_t *ml_i2c_inst_from_scl(uint pin_scl);

/**
 * @brief Get the I2C instance associated with the specified SDA pin.
 *
 * @param pin_sda The GPIO pin number for the I2C SDA line.
 * @return i2c_inst_t* The I2C instance associated with the specified SDA pin.
 */
i2c_inst_t *ml_i2c_inst_from_sda(uint pin_sda);
#endif

/**
 * @brief Begin an I2C write sequence by specifying the register to write to.
 *
 * @param i2c The I2C device structure.
 * @param reg The register address to write to.
 * @return true if the sequence was successfully started.
 * @return false if there was an error starting the sequence.
 */
bool ml_i2c_write_begin(ml_i2c_t *i2c, uint8_t reg);

/**
 * @brief Write a sequence of data to the I2C device.
 *
 * @param i2c The I2C device structure.
 * @param buf Pointer to the data buffer to write.
 * @param len The number of bytes to write.
 * @return true if the data was successfully queued for transmission.
 * @return false if there was an error queuing the data for transmission.
 */
bool ml_i2c_write_data(ml_i2c_t *i2c, uint8_t *buf, size_t len);

/**
 * @brief Write an 8-bit value to the I2C device as part of a write sequence.
 *
 * @param i2c The I2C device structure.
 * @param value The 8-bit value to write.
 * @return true if the data was successfully queued for transmission.
 * @return false if there was an error queuing the data for transmission.
 */
bool ml_i2c_write_data_8(ml_i2c_t *i2c, uint8_t value);

/**
 * @brief Write a 16-bit value in little-endian order (LSB first) to the I2C device as part of a write sequence.
 *
 * @param i2c The I2C device structure.
 * @param value The 16-bit value to write.
 * @return true if the data was successfully queued for transmission.
 * @return false if there was an error queuing the data for transmission.
 */
bool ml_i2c_write_data_16_le(ml_i2c_t *i2c, uint16_t value);

/**
 * @brief Write a 16-bit value in big-endian order (MSB first) to the I2C device as part of a write sequence.
 *
 * @param i2c The I2C device structure.
 * @param value The 16-bit value to write.
 * @return true if the data was successfully queued for transmission.
 * @return false if there was an error queuing the data for transmission.
 */
bool ml_i2c_write_data_16_be(ml_i2c_t *i2c, uint16_t value);

/**
 * @brief End the I2C write sequence, sending all queued data to the device.
 *
 * @param i2c The I2C device structure.
 * @return true if the sequence was successfully completed.
 * @return false if there was an error completing the sequence.
 */
bool ml_i2c_write_end(ml_i2c_t *i2c);

/**
 * @brief Read some data from the I2C device starting from the specified register.
 *
 * @param i2c The I2C device structure.
 * @param reg The register address to read from.
 * @param buf Pointer to the buffer to store the read data.
 * @param len The number of bytes to read.
 * @return true if the data was successfully read.
 * @return false if there was an error reading the data.
 */
bool ml_i2c_read_buf(ml_i2c_t *i2c, uint8_t reg, uint8_t *buf, size_t len);

/**
 * @brief Read some data from the I2C device without specifying a register.
 *
 * @param i2c The I2C device structure.
 * @param buf Pointer to the buffer to store the read data.
 * @param len The number of bytes to read.
 * @return true if the data was successfully read.
 * @return false if there was an error reading the data.
 */
bool ml_i2c_read_only(ml_i2c_t *i2c, uint8_t *buf, size_t len);

/**
 * @brief Write some data to the I2C device without specifying a register.
 *
 * @param i2c The I2C device structure.
 * @param buf Pointer to the buffer containing the data to write.
 * @param len The number of bytes to write.
 * @return true if the data was successfully queued for transmission.
 * @return false if there was an error queuing the data for transmission.
 */
bool ml_i2c_write_only(ml_i2c_t *i2c, const uint8_t *buf, size_t len);

/**
 * @brief Write some data to the I2C device and then read some data from it as part of a single transaction.
 *
 * @param i2c The I2C device structure.
 * @param wbuf Pointer to the buffer containing the data to write.
 * @param wlen The number of bytes to write.
 * @param rbuf Pointer to the buffer to store the read data.
 * @param rlen The number of bytes to read.
 * @return true if the write and read operations were successfully completed.
 * @return false if there was an error during the write or read operations.
 */
bool ml_i2c_write_read(ml_i2c_t *i2c,
                       const uint8_t *wbuf,
                       size_t wlen,
                       uint8_t *rbuf,
                       size_t rlen);

/**
 * @brief Write some data to the I2C device starting from the specified register.
 *
 * @param i2c The I2C device structure.
 * @param reg The register address to write to.
 * @param buf Pointer to the buffer containing the data to write.
 * @param len The number of bytes to write.
 * @return true if the data was successfully queued for transmission.
 * @return false if there was an error queuing the data for transmission.
 */
bool ml_i2c_write_buf(ml_i2c_t *i2c, uint8_t reg, const uint8_t *buf, size_t len);

/**
 * @brief Read a 16-bit value from the I2C device in big-endian format starting from the specified register.
 *
 * @param i2c The I2C device structure.
 * @param reg The register address to read from.
 * @param value Pointer that receives the value on success.
 * @return true if the complete I2C transaction succeeded; false otherwise.
 */
uint16_t ml_i2c_read_16_be(ml_i2c_t *i2c, uint8_t reg);

/**
 * @brief Read a 16-bit value from the I2C device in little-endian format starting from the specified register.
 *
 * @param i2c The I2C device structure.
 * @param reg The register address to read from.
 * @param value Pointer that receives the value on success.
 * @return true if the complete I2C transaction succeeded; false otherwise.
 */
uint16_t ml_i2c_read_16_le(ml_i2c_t *i2c, uint8_t reg);

/**
 * @brief Write a 16-bit value to the I2C device in big-endian format starting from the specified register.
 *
 * @param i2c The I2C device structure.
 * @param reg The register address to write to.
 * @param value The 16-bit value to write.
 * @return true if the data was successfully queued for transmission.
 * @return false if there was an error queuing the data for transmission.
 */
bool ml_i2c_write_16_be(ml_i2c_t *i2c, uint8_t reg, uint16_t value);

/**
 * @brief Write a 16-bit value to the I2C device in little-endian format starting from the specified register.
 *
 * @param i2c The I2C device structure.
 * @param reg The register address to write to.
 * @param value The 16-bit value to write.
 * @return true if the data was successfully queued for transmission.
 * @return false if there was an error queuing the data for transmission.
 */
bool ml_i2c_write_16_le(ml_i2c_t *i2c, uint8_t reg, uint16_t value);

/**
 * @brief Read an 8-bit value from the I2C device starting from the specified register.
 *
 * @param i2c The I2C device structure.
 * @param reg The register address to read from.
 * @param value Pointer that receives the value on success.
 * @return true if the complete I2C transaction succeeded; false otherwise.
 */
uint8_t ml_i2c_read_8(ml_i2c_t *i2c, uint8_t reg);

/**
 * @brief Write an 8-bit value to the I2C device starting from the specified register.
 *
 * @param i2c The I2C device structure.
 * @param reg The register address to write to.
 * @param value The 8-bit value to write.
 * @return true if the data was successfully queued for transmission.
 * @return false if there was an error queuing the data for transmission.
 */
bool ml_i2c_write_8(ml_i2c_t *i2c, uint8_t reg, uint8_t value);
