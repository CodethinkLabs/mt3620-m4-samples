/* Copyright (c) Microsoft Corporation.
   Licensed under the MIT License. */

#include "i2c_threadx.h"

static void I2CMaster_RTOS_Callback(int32_t status, uintptr_t count, void *userData)
{
    (void)count;

    i2c_rtos_handle_t *handle = (i2c_rtos_handle_t *)userData;
    handle->async_status = status;

    (void)tx_semaphore_put(&handle->semaphore);
}

int32_t I2CMaster_RTOS_Init(i2c_rtos_handle_t *handle, Platform_Unit unit, I2C_BusSpeed speed)
{
    if (handle == NULL) {
        return ERROR_PARAMETER;
    }

    memset(handle, 0, sizeof(i2c_rtos_handle_t));

    handle->i2c_handle = I2CMaster_Open(unit);
    if (handle->i2c_handle == NULL) {
        return ERROR;
    }

    (void)I2CMaster_SetBusSpeed(handle->i2c_handle, speed);    

    if (tx_mutex_create(&handle->mutex, "i2c mutex", TX_INHERIT) != TX_SUCCESS) {
        return ERROR;
    }

    if (tx_semaphore_create(&handle->semaphore, "i2c semaphore", 0) != TX_SUCCESS) {
        (void)tx_mutex_delete(&handle->mutex);
        return ERROR;
    }    

    return ERROR_NONE;
}

int32_t I2CMaster_RTOS_Deinit(i2c_rtos_handle_t *handle)
{
    I2CMaster_Close(handle->i2c_handle);

    (void)tx_semaphore_delete(&handle->semaphore);
    (void)tx_mutex_delete(&handle->mutex);

    return ERROR_NONE;
}

int32_t I2CMaster_RTOS_Transfer(i2c_rtos_handle_t *handle, i2c_rtos_transfer_t *xfer)
{
    int32_t rt;
    UINT    status;

    if (tx_mutex_get(&handle->mutex, TX_WAIT_FOREVER) != TX_SUCCESS) {
        return ERROR_BUSY;
    }

    rt = I2CMaster_TransferSequentialAsync_UserData(handle->i2c_handle, 
                                                    xfer->address, 
                                                    xfer->transfers, 
                                                    xfer->count, 
                                                    I2CMaster_RTOS_Callback,
                                                    (void *)handle);
    
    if (rt != ERROR_NONE) {
        tx_mutex_put(&handle->mutex);
        return rt;
    }

    status = tx_semaphore_get(&handle->semaphore, xfer->timeout > 0 ? xfer->timeout : TX_WAIT_FOREVER);
    if (status == TX_NO_INSTANCE) {
        handle->async_status = ERROR_TIMEOUT;
    } else if (status != TX_SUCCESS) {
        handle->async_status = ERROR;
    } 

    (void)tx_mutex_put(&handle->mutex);

    return handle->async_status;
}
