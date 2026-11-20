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
#include "ml_i2c.h"

#if defined(ML_PLATFORM_ESP)
/* One bus handle per port (0, 1, …) */
static i2c_master_bus_handle_t s_i2c_bus[I2C_NUM_MAX]       = {0};
static uint32_t                s_i2c_bus_speed[I2C_NUM_MAX] = {0};

static bool ml_i2c_ensure_bus(ml_i2c_port_t port, uint32_t speed, uint8_t sda_pin, uint8_t scl_pin, bool pull_up)
{
    if (port < 0 || port >= I2C_NUM_MAX)
    {
        return false;
    }
    if (s_i2c_bus[port] != NULL)
    {
        /* Bus already created */
        return true;
    }

    i2c_master_bus_config_t cfg = {
        .clk_source        = I2C_CLK_SRC_DEFAULT,
        .i2c_port          = port,
        .scl_io_num        = scl_pin,
        .sda_io_num        = sda_pin,
        .glitch_ignore_cnt = 7,
        .flags =
            {
                    .enable_internal_pullup = pull_up,
                    },
    };

    esp_err_t err = i2c_new_master_bus(&cfg, &s_i2c_bus[port]);
    if (err != ESP_OK)
    {
        return false;
    }
    s_i2c_bus_speed[port] = speed;
    return true;
}

static bool ml_i2c_ensure_device(ml_i2c_t *i2c)
{
    if (!i2c) return false;
    if (i2c->dev) return true;

    if (i2c->i2c_port < 0 || i2c->i2c_port >= I2C_NUM_MAX)
    {
        return false;
    }

    i2c_master_bus_handle_t bus = s_i2c_bus[i2c->i2c_port];
    if (!bus)
    {
        return false;
    }

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address  = i2c->i2c_addr,
        .scl_speed_hz    = s_i2c_bus_speed[i2c->i2c_port],
    };

    esp_err_t err = i2c_master_bus_add_device(bus, &dev_cfg, &i2c->dev);
    return (err == ESP_OK);
}

#endif // ML_PLATFORM_ESP

bool ml_i2c_device_exists(ml_i2c_t *i2c)
{
    if (i2c == NULL)
    {
        return false;
    }
#if defined(ML_PLATFORM_PICO)
    uint8_t val;
    int     ret = i2c_read_blocking(i2c->i2c_port, i2c->i2c_addr, &val, 1, false);
    return (ret != PICO_ERROR_GENERIC);
#elif defined(ML_PLATFORM_ESP)
    if (!i2c) return false;
    ml_i2c_port_t port = i2c->i2c_port;
    if (port < 0 || port >= I2C_NUM_MAX)
    {
        return false;
    }
    if (!s_i2c_bus[port])
    {
        /* Bus not initialized : we cannot check */
        return false;
    }
    esp_err_t err = i2c_master_probe(s_i2c_bus[port], i2c->i2c_addr, 100 /* ms */);
    return (err == ESP_OK);
#else
#error "Unsupported platform for ml_i2c_device_exists()"
#endif
}

void ml_i2c_init(ml_i2c_port_t i2c_port, uint32_t speed, uint8_t sda_pin, uint8_t scl_pin, bool pull_up)
{
#if defined(ML_PLATFORM_PICO)
    i2c_init(i2c_port, speed);
    gpio_init(sda_pin);
    gpio_init(scl_pin);
    gpio_set_function(sda_pin, GPIO_FUNC_I2C);
    gpio_set_function(scl_pin, GPIO_FUNC_I2C);
    if (pull_up)
    {
        gpio_pull_up(sda_pin);
        gpio_pull_up(scl_pin);
    }
#elif defined(ML_PLATFORM_ESP)
    (void)ml_i2c_ensure_bus(i2c_port, speed, sda_pin, scl_pin, pull_up);
#endif
}

void ml_i2c_bus_clear(ml_i2c_port_t i2c_port, uint32_t speed, uint8_t sda_pin, uint8_t scl_pin, bool pull_up)
{
#if defined(ML_PLATFORM_PICO)
    /* Uninitialize I2C to control the pins manually */
    ml_i2c_deinit(i2c_port);

    /* Setup SDA & SCL as output GPIOs and set them to HIGH */
    gpio_init(sda_pin);
    gpio_set_dir(sda_pin, GPIO_OUT);
    gpio_put(sda_pin, 1);

    gpio_init(scl_pin);
    gpio_set_dir(scl_pin, GPIO_OUT);
    gpio_put(scl_pin, 1);

    sleep_us(5);

    /* Generate 9 clock cycles manually on SCL */
    for (int i = 0; i < 9; i++)
    {
        gpio_put(scl_pin, 0);
        sleep_us(5);
        gpio_put(scl_pin, 1);
        sleep_us(5);
    }

    /* Generate a stop condition manually : SDA changes from LOW to HIGH when SCL is HIGH */
    gpio_put(sda_pin, 0);
    sleep_us(5);
    gpio_put(scl_pin, 1);
    sleep_us(5);
    gpio_put(sda_pin, 1);
    sleep_us(5);

    ml_i2c_init(i2c_port, speed, sda_pin, scl_pin, pull_up);
#elif defined(ML_PLATFORM_ESP)
    if (!ml_i2c_ensure_bus(i2c_port, speed, sda_pin, scl_pin, pull_up))
    {
        return;
    }
    (void)i2c_master_bus_reset(s_i2c_bus[i2c_port]);
#else
#error "Unsupported platform for ml_i2c_bus_clear()"
#endif
}

void ml_i2c_deinit(ml_i2c_port_t i2c_port)
{
#if defined(ML_PLATFORM_PICO)
    i2c_deinit(i2c_port);
#elif defined(ML_PLATFORM_ESP)
    if (i2c_port >= 0 && i2c_port < I2C_NUM_MAX && s_i2c_bus[i2c_port])
    {
        i2c_del_master_bus(s_i2c_bus[i2c_port]);
        s_i2c_bus[i2c_port]       = NULL;
        s_i2c_bus_speed[i2c_port] = 0;
    }
#else
#error "Unsupported platform for ml_i2c_deinit()"
#endif
}

bool ml_i2c_read_buf(ml_i2c_t *i2c, uint8_t reg, uint8_t *buf, size_t len)
{
    if (i2c == NULL || buf == NULL || len == 0)
    {
        return false;
    }
#if defined(ML_PLATFORM_PICO)
    i2c_write_blocking(i2c->i2c_port, i2c->i2c_addr, &reg, 1,
                       true); // keep bus (restart)
    return i2c_read_blocking(i2c->i2c_port, i2c->i2c_addr, buf, len,
                             false) == len; // release bus
#elif defined(ML_PLATFORM_ESP)
    if (!ml_i2c_ensure_device(i2c))
    {
        return false;
    }
    esp_err_t err = i2c_master_transmit_receive(i2c->dev, &reg, 1, buf, len, 100);
    return (err == ESP_OK);
#else
#error "Unsupported platform for ml_i2c_read_buf()"
#endif
}

bool ml_i2c_read_only(ml_i2c_t *i2c, uint8_t *buf, size_t len)
{
    if (!i2c || !buf || len == 0)
    {
        return false;
    }
#if defined(ML_PLATFORM_PICO)
    /* direct read : START + ADDR|R, then bytes len, then STOP */
    int ret = i2c_read_blocking(i2c->i2c_port,
                                i2c->i2c_addr,
                                buf,
                                len,
                                false); // stop = true (false here because "no repeated start")
    return (ret == (int)len);

#elif defined(ML_PLATFORM_ESP)
    if (!ml_i2c_ensure_device(i2c))
    {
        return false;
    }
    esp_err_t err = i2c_master_receive(i2c->dev, buf, len, 100);
    return (err == ESP_OK);
#else
#error "Unsupported platform for ml_i2c_read_only()"
#endif
}

bool ml_i2c_write_begin(ml_i2c_t *i2c, uint8_t reg)
{
    if (!i2c)
    {
        return false;
    }
    i2c->last_byte_remain = false;
#if defined(ML_PLATFORM_PICO)
    return i2c_write_burst_blocking(i2c->i2c_port, i2c->i2c_addr, &reg, 1) == 1;
#elif defined(ML_PLATFORM_ESP)
    /* We save the register byte */
    i2c->tx_reg                 = reg;

    /* Setup the fragments list with the first byte */
    i2c->tx_fragments[0].buffer = &i2c->tx_reg;
    i2c->tx_fragments[0].size   = 1;

    i2c->tx_frag_count          = 1;
    i2c->tx_in_progress         = true;

    return true;
#else
#error "Unsupported platform for ml_i2c_write_begin()"
#endif
}

bool ml_i2c_write_data(ml_i2c_t *i2c, uint8_t *buf, size_t len)
{
    if (!i2c || len == 0 || !buf)
    {
        return false;
    }
#if defined(ML_PLATFORM_PICO)
    bool result = false;
    if (i2c->last_byte_remain)
    {
        result                = i2c_write_burst_blocking(i2c->i2c_port, i2c->i2c_addr, &i2c->last_byte, 1) == 1;
        i2c->last_byte_remain = false;
        if (!result)
        {
            return false;
        }
    }
    if (len > 1)
    {
        result = i2c_write_burst_blocking(i2c->i2c_port, i2c->i2c_addr, buf, len - 1) == (int)(len - 1);
        if (!result)
        {
            return false;
        }
        i2c->last_byte        = buf[len - 1];
        i2c->last_byte_remain = true;
    }
    else
    {
        i2c->last_byte        = *buf;
        i2c->last_byte_remain = true;
        result                = true;
    }
    return result;
#elif defined(ML_PLATFORM_ESP)
    if (!i2c->tx_in_progress)
    {
        return false;
    }

    if (i2c->tx_frag_count >= I2C_MAX_FRAGMENTS)
    {
        return false;
    }

    i2c->tx_fragments[i2c->tx_frag_count].buffer = buf;
    i2c->tx_fragments[i2c->tx_frag_count].size   = len;

    i2c->tx_frag_count++;

    return true;
#else
#error "Unsupported platform for ml_i2c_write_data()"
#endif
}

bool ml_i2c_write_data_8(ml_i2c_t *i2c, uint8_t value)
{
    return ml_i2c_write_data(i2c, &value, 1);
}

bool ml_i2c_write_data_16_le(ml_i2c_t *i2c, uint16_t value)
{
    uint8_t buf[2] = {value & 0xFF, value >> 8};
    return ml_i2c_write_data(i2c, buf, 2);
}

bool ml_i2c_write_data_16_be(ml_i2c_t *i2c, uint16_t value)
{
    uint8_t buf[2] = {value >> 8, value & 0xFF};
    return ml_i2c_write_data(i2c, buf, 2);
}

bool ml_i2c_write_end(ml_i2c_t *i2c)
{
    if (!i2c)
    {
        return false;
    }
#if defined(ML_PLATFORM_PICO)
    /* send last byte with STOP */
    /* on Pico, we cannot send STOP only, so we send the last byte + STOP */
    return i2c_write_blocking(i2c->i2c_port, i2c->i2c_addr, &i2c->last_byte, 1, false) == 1;
#elif defined(ML_PLATFORM_ESP)
    if (!i2c->tx_in_progress)
    {
        return false;
    }
    if (!ml_i2c_ensure_device(i2c))
    {
        return false; // S'assure que dev est prêt
    }
    /* We use the ESP-IDF 5+ new magic function */
    /* Our fragments list is sent in one transaction (START ... STOP) */
    esp_err_t err       = i2c_master_multi_buffer_transmit(i2c->dev,
                                                           (i2c_master_transmit_multi_buffer_info_t *)i2c->tx_fragments,
                                                           i2c->tx_frag_count,
                                                           pdMS_TO_TICKS(100) // timeout
    );

    i2c->tx_in_progress = false;
    i2c->tx_frag_count  = 0;

    return (err == ESP_OK);
#else
#error "Unsupported platform for ml_i2c_write_end()"
#endif
}

bool ml_i2c_write_buf(ml_i2c_t *i2c, uint8_t reg, const uint8_t *buf, size_t len)
{
    uint8_t dst[64];
    if (i2c == NULL || buf == NULL || len == 0)
    {
        return false;
    }
    if (len > 63)
    {
        len = 63;
    }
    dst[0] = reg;
    for (size_t i = 0; i < len; i++)
    {
        dst[1 + i] = buf[i];
    }
#if defined(ML_PLATFORM_PICO)
    return i2c_write_blocking(i2c->i2c_port, i2c->i2c_addr, dst, len + 1, false) == (int)(len + 1);
#elif defined(ML_PLATFORM_ESP)
    if (!ml_i2c_ensure_device(i2c))
    {
        return false;
    }
    esp_err_t err = i2c_master_transmit(i2c->dev, dst, len + 1, 100);
    return (err == ESP_OK);
#else
#error "Unsupported platform for ml_i2c_write_buf()"
#endif
}

uint8_t ml_i2c_read_8(ml_i2c_t *i2c, uint8_t reg)
{
    uint8_t buf = 0;
#if defined(ML_PLATFORM_PICO)
    i2c_write_blocking(i2c->i2c_port, i2c->i2c_addr, &reg, 1, true); // restart
    i2c_read_blocking(i2c->i2c_port, i2c->i2c_addr, &buf, 1, false); // stop
#elif defined(ML_PLATFORM_ESP)
    if (!ml_i2c_ensure_device(i2c))
    {
        return 0;
    }
    (void)i2c_master_transmit_receive(i2c->dev, &reg, 1, &buf, 1, 100);
#else
#error "Unsupported platform for ml_i2c_read_8()"
#endif
    return buf;
}

uint16_t ml_i2c_read_16_be(ml_i2c_t *i2c, uint8_t reg)
{
    uint8_t buf[2] = {0, 0};
#if defined(ML_PLATFORM_PICO)
    i2c_write_blocking(i2c->i2c_port, i2c->i2c_addr, &reg, 1, true); // restart
    sleep_us(100);
    i2c_read_blocking(i2c->i2c_port, i2c->i2c_addr, buf, 2, false); // stop
#elif defined(ML_PLATFORM_ESP)
    if (!ml_i2c_ensure_device(i2c))
    {
        return 0;
    }
    (void)i2c_master_transmit_receive(i2c->dev, &reg, 1, (void *)&buf, 2, 100);
#endif
    return ((uint16_t)buf[0] << 8) | buf[1];
}

uint16_t ml_i2c_read_16_le(ml_i2c_t *i2c, uint8_t reg)
{
    uint8_t buf[2] = {0, 0};
#if defined(ML_PLATFORM_PICO)
    i2c_write_blocking(i2c->i2c_port, i2c->i2c_addr, &reg, 1, true); // restart
    sleep_us(100);
    i2c_read_blocking(i2c->i2c_port, i2c->i2c_addr, buf, 2, false); // stop
#elif defined(ML_PLATFORM_ESP)
    if (!ml_i2c_ensure_device(i2c))
    {
        return 0;
    }
    (void)i2c_master_transmit_receive(i2c->dev, &reg, 1, (void *)&buf, 2, 100);
#endif
    return ((uint16_t)buf[1] << 8) | buf[0];
}

bool ml_i2c_write_only(ml_i2c_t *i2c, const uint8_t *buf, size_t len)
{
    if (!i2c || !buf || len == 0)
    {
        return false;
    }
#if defined(ML_PLATFORM_PICO)
    int ret = i2c_write_blocking(i2c->i2c_port, i2c->i2c_addr, buf, len, false);
    return (ret == (int)len);
#elif defined(ML_PLATFORM_ESP)
    if (!ml_i2c_ensure_device(i2c))
    {
        return false;
    }
    esp_err_t err = i2c_master_transmit(i2c->dev, buf, len, pdMS_TO_TICKS(100));
    return (err == ESP_OK);
#else
#error "Unsupported platform for ml_i2c_write_only()"
#endif
}

bool ml_i2c_write_read(ml_i2c_t *i2c, const uint8_t *wbuf, size_t wlen, uint8_t *rbuf, size_t rlen)
{
    if (!i2c || !wbuf || wlen == 0 || !rbuf || rlen == 0)
    {
        return false;
    }

#if defined(ML_PLATFORM_PICO)
    /* Write with repeated START (no STOP) then read with STOP. */
    int ret = i2c_write_blocking(i2c->i2c_port, i2c->i2c_addr, wbuf, wlen,
                                 true); // nostop = true → repeated START
    if (ret != (int)wlen)
    {
        return false;
    }

    ret = i2c_read_blocking(i2c->i2c_port, i2c->i2c_addr, rbuf, rlen,
                            false); // nostop = false → STOP at the end
    return (ret == (int)rlen);
#elif defined(ML_PLATFORM_ESP)
    if (!ml_i2c_ensure_device(i2c))
    {
        return false;
    }
    esp_err_t err = i2c_master_transmit_receive(i2c->dev,
                                                wbuf,
                                                wlen,
                                                rbuf,
                                                rlen,
                                                pdMS_TO_TICKS(100) // timeout 100 ms
    );
    return (err == ESP_OK);
#else
#error "Unsupported platform for ml_i2c_write_read()"
#endif
}

bool ml_i2c_write_8(ml_i2c_t *i2c, uint8_t reg, uint8_t val)
{
    uint8_t buf[2] = {reg, val};
#if defined(ML_PLATFORM_PICO)
    return i2c_write_blocking(i2c->i2c_port, i2c->i2c_addr, buf, 2, false) == 2;
#elif defined(ML_PLATFORM_ESP)
    if (!ml_i2c_ensure_device(i2c))
    {
        return false;
    }
    esp_err_t err = i2c_master_transmit(i2c->dev, buf, 2, 100);
    return (err == ESP_OK);
#else
#error "Unsupported platform for ml_i2c_write_8()"
#endif
}

bool ml_i2c_write_16_le(ml_i2c_t *i2c, uint8_t reg, uint16_t value)
{
    uint8_t buf[3] = {reg, (uint8_t)(value & 0xFF), (uint8_t)(value >> 8)};
#if defined(ML_PLATFORM_PICO)
    return i2c_write_blocking(i2c->i2c_port, i2c->i2c_addr, buf, 3, false) == 3;
#elif defined(ML_PLATFORM_ESP)
    if (!ml_i2c_ensure_device(i2c))
    {
        return false;
    }
    esp_err_t err = i2c_master_transmit(i2c->dev, buf, 3, 100);
    return (err == ESP_OK);
#else
#error "Unsupported platform for ml_i2c_write_16_le()"
#endif
}

bool ml_i2c_write_16_be(ml_i2c_t *i2c, uint8_t reg, uint16_t value)
{
    uint8_t buf[3] = {reg, (uint8_t)(value >> 8), (uint8_t)(value & 0xFF)};
#if defined(ML_PLATFORM_PICO)
    return i2c_write_blocking(i2c->i2c_port, i2c->i2c_addr, buf, 3, false) == 3;
#elif defined(ML_PLATFORM_ESP)
    if (!ml_i2c_ensure_device(i2c))
    {
        return false;
    }
    esp_err_t err = i2c_master_transmit(i2c->dev, buf, 3, 100);
    return (err == ESP_OK);
#else
#error "Unsupported platform for ml_i2c_write_16_be()"
#endif
}

#if defined(ML_PLATFORM_PICO)
i2c_inst_t *ml_i2c_inst_from_scl(uint pin_scl)
{
    if (pin_scl == 1 || pin_scl == 5 || pin_scl == 9 || pin_scl == 13 || pin_scl == 17 || pin_scl == 21)
        return i2c0;
    else if (pin_scl == 3 || pin_scl == 7 || pin_scl == 11 || pin_scl == 15 || pin_scl == 19 || pin_scl == 27)
        return i2c1;
    else
        return NULL;
}

i2c_inst_t *ml_i2c_inst_from_sda(uint pin_sda)
{
    if (pin_sda == 0 || pin_sda == 4 || pin_sda == 8 || pin_sda == 12 || pin_sda == 16 || pin_sda == 20)
        return i2c0;
    else if (pin_sda == 2 || pin_sda == 6 || pin_sda == 10 || pin_sda == 14 || pin_sda == 18 || pin_sda == 26)
        return i2c1;
    else
        return NULL;
}
#endif
