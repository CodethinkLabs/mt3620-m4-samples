/* Copyright (c) Codethink Ltd. All rights reserved.
   Licensed under the MIT License. */

#include "LSM6DS3.h"

bool LSM6DS3_RegWrite(void *driver, uint8_t addr, uint8_t value)
{
#if defined(USE_I2C_THREADX_API)

    const uint8_t cmd[] = { addr, value };

    i2c_rtos_transfer_t xfer;

    xfer.address = LSM6DS3_ADDRESS;
    xfer.count = 1;
    xfer.transfers[0] = (I2C_Transfer){
        .writeData = cmd,
        .readData = NULL,
        .length = sizeof(cmd),
    };
    xfer.timeout = 100;

    return (I2CMaster_RTOS_Transfer((i2c_rtos_handle_t *)driver, &xfer) == ERROR_NONE);

#else
    const uint8_t cmd[] = { addr, value };
    return (I2CMaster_WriteSync((I2CMaster *)driver, LSM6DS3_ADDRESS, cmd, sizeof(cmd)) == ERROR_NONE);
#endif
}

bool LSM6DS3_RegRead(void *driver, uint8_t addr, uint8_t *value)
{
#if defined(USE_I2C_THREADX_API)
    uint8_t v;
    int32_t status;
    i2c_rtos_transfer_t xfer;

    xfer.address = LSM6DS3_ADDRESS;
    xfer.count = 2;
    xfer.transfers[0] = (I2C_Transfer){ .writeData = &addr, .readData = NULL, .length = sizeof(addr) };
    xfer.transfers[1] = (I2C_Transfer){ .writeData = NULL , .readData = &v,   .length = sizeof(v) };
    xfer.timeout = 100;

    status = I2CMaster_RTOS_Transfer((i2c_rtos_handle_t *)driver, &xfer);

    if (value != NULL) {
        *value = v;
    }
    return (status == ERROR_NONE);
#else
    uint8_t v;
    int32_t status = I2CMaster_WriteThenReadSync(
        driver, LSM6DS3_ADDRESS, &addr, sizeof(addr), &v, sizeof(v));
    if (value) {
        *value = v;
    }
    return (status == ERROR_NONE);
#endif
}

bool LSM6DS3_Reset(void *driver)
{
    if (!driver) {
        return false;
    }

    if (!LSM6DS3_RegWrite(driver, LSM6DS3_REG_CTRL3_C, 0x01)) {
        return false;
    }

    uint8_t status;
    while (true) {
        if (LSM6DS3_RegRead(driver, LSM6DS3_REG_CTRL3_C, &status)
            && ((status & 0x01) == 0)) {
            break;
        }
    }

    return true;
}

bool LSM6DS3_CheckWhoAmI(void *driver)
{
    if (!driver) {
        return false;
    }

    uint8_t ident;
    return (LSM6DS3_RegRead(driver, LSM6DS3_REG_WHO_AM_I, &ident)
        && (ident == LSM6DS3_WHO_AM_I));
}

bool LSM6DS3_ConfigXL(void *driver, unsigned odr, unsigned fs, unsigned bw)
{
    if (!driver) {
        return false;
    }

    LSM6DS3_ctrl1_xl_t ctrl1_xl;

    if ((odr >> 4) != 0) {
        return false;
    }
    ctrl1_xl.odr_xl = odr;

    switch (fs) {
    case  2:
        ctrl1_xl.fs_xl = 0;
        break;
    case 16:
        ctrl1_xl.fs_xl = 1;
        break;
    case  4:
        ctrl1_xl.fs_xl = 2;
        break;
    case  8:
        ctrl1_xl.fs_xl = 3;
        break;
    default:
        return false;
    }

    switch (bw)
    {
    case  50:
        ctrl1_xl.bw_xl = 3;
        break;
    case 100:
        ctrl1_xl.bw_xl = 2;
        break;
    case 200:
        ctrl1_xl.bw_xl = 1;
        break;
    case 400:
        ctrl1_xl.bw_xl = 0;
        break;
    default:
        return false;
    }

    return LSM6DS3_RegWrite(driver, LSM6DS3_REG_CTRL1_XL, ctrl1_xl.mask);
}


bool LSM6DS3_ConfigG(void *driver, unsigned odr, unsigned fs)
{
    if (!driver) {
        return false;
    }

    LSM6DS3_ctrl2_g_t ctrl2_g;

    if ((odr >> 4) != 0) {
        return false;
    }
    ctrl2_g.odr_g = odr;

    switch (fs) {
    case  125:
        ctrl2_g.fs_125 = true;
        ctrl2_g.fs_g   = 0;
        break;
    case  250:
        ctrl2_g.fs_125 = false;
        ctrl2_g.fs_g   = 1;
        break;
    case  500:
        ctrl2_g.fs_125 = false;
        ctrl2_g.fs_g   = 1;
        break;
    case 1000:
        ctrl2_g.fs_125 = false;
        ctrl2_g.fs_g   = 2;
        break;
    case 2000:
        ctrl2_g.fs_125 = false;
        ctrl2_g.fs_g   = 3;
        break;
    default:
        return false;
    }

    return LSM6DS3_RegWrite(driver, LSM6DS3_REG_CTRL2_G, ctrl2_g.mask);
}

bool LSM6DS3_Status(void *driver, bool *tda, bool *gda, bool *xlda)
{
    if (!driver) {
        return false;
    }

    LSM6DS3_status_t status;
    if (!LSM6DS3_RegRead(driver, LSM6DS3_REG_STATUS_REG, &status.mask)) {
        return false;
    }

    if (tda) {
        *tda = status.tda;
    }
    if (gda) {
        *gda = status.gda;
    }
    if (xlda) {
        *xlda = status.xlda;
    }
    return true;
}

bool LSM6DS3_ReadTemp(void *driver, int16_t *temp)
{
    if (!driver) {
        return false;
    }

    int16_t t = 0;
    if (!LSM6DS3_RegRead(driver, LSM6DS3_REG_OUT_TEMP_L, &((uint8_t *)&t)[0])
        || !LSM6DS3_RegRead(driver, LSM6DS3_REG_OUT_TEMP_H, &((uint8_t *)&t)[1])) {
        return false;
    }

    if (temp) {
        *temp = t;
    }
    return true;
}

bool LSM6DS3_ReadTempHuman(void *driver, int16_t *t)
{
    int16_t th;
    if (!LSM6DS3_ReadTemp(driver, &th)) {
        return false;
    }

    if (t) *t = 25000 + ((th * 1000) >> 4);
    return true;
}

bool LSM6DS3_ReadG(void *driver, int16_t *x, int16_t *y, int16_t *z)
{
    if (!driver) {
        return false;
    }

    int16_t g_x = 0;
    int16_t g_y = 0;
    int16_t g_z = 0;

    if (!LSM6DS3_RegRead(driver, LSM6DS3_REG_OUTX_L_G, &((uint8_t *)&g_x)[0])
        || !LSM6DS3_RegRead(driver, LSM6DS3_REG_OUTX_H_G, &((uint8_t *)&g_x)[1])
        || !LSM6DS3_RegRead(driver, LSM6DS3_REG_OUTY_L_G, &((uint8_t *)&g_y)[0])
        || !LSM6DS3_RegRead(driver, LSM6DS3_REG_OUTY_H_G, &((uint8_t *)&g_y)[1])
        || !LSM6DS3_RegRead(driver, LSM6DS3_REG_OUTZ_L_G, &((uint8_t *)&g_z)[0])
        || !LSM6DS3_RegRead(driver, LSM6DS3_REG_OUTZ_H_G, &((uint8_t *)&g_z)[1])) {
        return false;
    }

    if (x) {
        *x = g_x;
    }
    if (y) {
        *y = g_y;
    }
    if (z) {
        *z = g_z;
    }
    return true;
}

bool LSM6DS3_ReadGHuman(void *driver, int16_t *x, int16_t *y, int16_t *z)
{
    LSM6DS3_ctrl2_g_t ctrl2_g;
    if (!LSM6DS3_RegRead(driver, LSM6DS3_REG_CTRL2_G, &ctrl2_g.mask)) {
        return false;
    }

    // Calculate fixed point fract for scale as between 0.061 and 0.488.
    uint8_t fs_g = 0;
    if (!ctrl2_g.fs_125) {
        fs_g = (ctrl2_g.fs_g + 1);
    }
    int32_t scale = 35840 >> (4 - fs_g);

    int16_t xh, yh, zh;
    if (!LSM6DS3_ReadXL(driver, &xh, &yh, &zh)) {
        return false;
    }

    if (x) *x = (xh * scale) >> 9;
    if (y) *y = (yh * scale) >> 9;
    if (z) *z = (zh * scale) >> 9;
    return true;
}

bool LSM6DS3_ReadXL(void *driver, int16_t *x, int16_t *y, int16_t *z)
{
    if (!driver) {
        return false;
    }

    int16_t xl_x = 0;
    int16_t xl_y = 0;
    int16_t xl_z = 0;

    if (!LSM6DS3_RegRead(driver, LSM6DS3_REG_OUTX_L_XL, &((uint8_t *)&xl_x)[0])
        || !LSM6DS3_RegRead(driver, LSM6DS3_REG_OUTX_H_XL, &((uint8_t *)&xl_x)[1])
        || !LSM6DS3_RegRead(driver, LSM6DS3_REG_OUTY_L_XL, &((uint8_t *)&xl_y)[0])
        || !LSM6DS3_RegRead(driver, LSM6DS3_REG_OUTY_H_XL, &((uint8_t *)&xl_y)[1])
        || !LSM6DS3_RegRead(driver, LSM6DS3_REG_OUTZ_L_XL, &((uint8_t *)&xl_z)[0])
        || !LSM6DS3_RegRead(driver, LSM6DS3_REG_OUTZ_H_XL, &((uint8_t *)&xl_z)[1])) {
        return false;
    }

    if (x) {
        *x = xl_x;
    }
    if (y) {
        *y = xl_y;
    }
    if (z) {
        *z = xl_z;
    }
    return true;
}

bool LSM6DS3_ReadXLHuman(void *driver, int16_t *x, int16_t *y, int16_t *z)
{
    LSM6DS3_ctrl1_xl_t ctrl1_xl;
    if (!LSM6DS3_RegRead(driver, LSM6DS3_REG_CTRL1_XL, &ctrl1_xl.mask)) {
        return false;
    }

    // Calculate fixed point fract for scale as between 0.061 and 0.488.
    uint8_t fs_xl_swiz[4] = { 0, 3, 1, 2 };
    int32_t scale = 31982 >> (3 - fs_xl_swiz[ctrl1_xl.fs_xl]);

    int16_t xh, yh, zh;
    if (!LSM6DS3_ReadXL(driver, &xh, &yh, &zh)) {
        return false;
    }

    if (x) *x = (xh * scale) >> 16;
    if (y) *y = (yh * scale) >> 16;
    if (z) *z = (zh * scale) >> 16;
    return true;
}
