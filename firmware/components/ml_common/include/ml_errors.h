#pragma once

typedef enum
{
    ML_OK                       = 0,

    /* Generic errors */

    ML_ERR_INVALID_ARG          = -1,
    ML_ERR_UNSUPPORTED          = -2,
    ML_ERR_NO_MEM               = -3,
    ML_ERR_NOT_FOUND            = -4,
    ML_ERR_NOT_INIT             = -5,
    ML_ERR_TIMEOUT              = -6,
    ML_ERR_BUSY                 = -7,
    ML_ERR_GENERIC              = -8,
    ML_ERR_INVALID_STATE        = -9, 
    ML_ERR_EARLY_POLL           = -10,   
    ML_ERR_DUPLICATE            = -11,
    ML_ERR_IO                   = -12,
    ML_ERR_LOCKED               = -13,

    
    /* SPI -20 to -40 */
    ML_SPI_ERR_INVALID_STATE    = -20, /* TODO : Remove */
    ML_SPI_ERR_IO               = -21, /* TODO : Remove */
    ML_SPI_ERR_TIMEOUT          = -22, /* TODO : Remove */
    ML_SPI_ERR_BUSY             = -23, /* TODO : Remove */
    ML_SPI_ERR_LOCKED           = -24, /* TODO : Remove */
    ML_SPI_ERR_TOO_MANY_DEVICES = -25, 
    ML_SPI_ERR_NOT_ACQUIRED     = -26,
    ML_SPI_ERR_NEED_FINALIZE    = -27,

    /* I2C -41 to -60 */
    ML_I2C_ERR_IO               = -41,

    /* LCD -61 to -80 */
    ML_LCD_ERR_LAST             = -80,

    /* DS18B20 - 81 to - 90 */
    ML_DS18B20_ERR_EARLY_PROBE  = -81,
    ML_DS18B20_ERR_NOT_FOUND    = -82

} ml_err_t;
