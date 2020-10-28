/* Copyright (c) Microsoft Corporation.
   Licensed under the MIT License. */

#ifndef __I2C_THREADX_H__
#define __I2C_THREADX_H__

#include "lib/I2CMaster.h"
#include "tx_api.h"

typedef struct _i2c_rtos_handle {
    I2CMaster       *i2c_handle;       
    int32_t         async_status;
    TX_MUTEX        mutex;
    TX_SEMAPHORE    semaphore;
} i2c_rtos_handle_t;

typedef struct _i2c_rtos_transfer {
    uint8_t         count;
    uint16_t        address;
    I2C_Transfer    transfers[2];
    uint32_t        timeout;
} i2c_rtos_transfer_t;

#if defined(__cplusplus)
extern "C" {
#endif

int32_t I2CMaster_RTOS_Init(i2c_rtos_handle_t* handle, Platform_Unit unit, I2C_BusSpeed speed);
int32_t I2CMaster_RTOS_Deinit(i2c_rtos_handle_t *handle);
int32_t I2CMaster_RTOS_Transfer(i2c_rtos_handle_t *handle, i2c_rtos_transfer_t *xfer);

#if defined(__cplusplus)
}
#endif

#endif 
