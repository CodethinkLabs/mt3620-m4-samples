/* Copyright (c) Codethink Ltd. All rights reserved.
   Licensed under the MIT License. */

#include <stddef.h>

#include "joystick.h"

#define JOYSTICK_HANDLE_MAX 4
//The following macros set the threshold for a high/low at >1.8V and <0.5V respectively
#define JOYSTICK_DEADZONE_MAX 2950
#define JOYSTICK_DEADZONE_MIN 820

struct Joystick {
    ADC_Data* data;
    uint32_t  numChannels;
    uint16_t  channelX;
    uint16_t  channelY;
    int32_t   centerPosX;
    int32_t   centerPosY;
    int32_t   xMax;
    int32_t   yMax;
    int32_t   xMin;
    int32_t   yMin;
    uint8_t   yDir;
    uint8_t   xDir;
};

static Joystick Joystick_Handles[JOYSTICK_HANDLE_MAX] = {0};

Joystick *Joystick_Open(ADC_Data *data, uint32_t numChannels, uint16_t channelX, uint16_t channelY)
{
    Joystick *handle = NULL;
    unsigned h;
    for (h = 0; h < JOYSTICK_HANDLE_MAX; h++) {
        if (!Joystick_Handles[h].data) {
            handle = &Joystick_Handles[h];
            break;
        }
    }
    if (!handle) {
        return NULL;
    }

    handle->data = data;
    handle->numChannels = numChannels;
    handle->channelX = channelX;
    handle->channelY = channelY;

    return handle;
}

void Joystick_Close(Joystick *handle)
{
    handle->data = NULL;
}

Joystick_XY Joystick_GetRawXY(const Joystick *handle)
{
    Joystick_XY joystick_data = { 0 };

    for (int i = 0; i < handle->numChannels; i++) {
        if (handle->data[i].channel == handle->channelX) {
            joystick_data.x = handle->data[i].value;
        }
        else if (handle->data[i].channel == handle->channelY) {
            joystick_data.y = handle->data[i].value;
        }
    }
    return joystick_data;
}

int32_t Joystick_Calibrate(Joystick *handle, uint8_t direction)
{
    switch (direction) {
    case JOYSTICK_CENTER:
        handle->centerPosX = Joystick_GetRawXY(handle).x;
        handle->centerPosY = Joystick_GetRawXY(handle).y;
        if (handle->centerPosX == 0 || handle->centerPosY == 0) {
            return ERROR_JOYSTICK_CAL;
        } else {
            return ERROR_NONE;
        }
    case JOYSTICK_Y_MAX:
    {
        Joystick_XY value = Joystick_GetRawXY(handle);

        //Check for orientation of x/y and swap the channels if needed
        if (value.y >= JOYSTICK_DEADZONE_MAX || value.y <= JOYSTICK_DEADZONE_MIN) {
            handle->yMax = value.y;
        }
        else if (value.x >= JOYSTICK_DEADZONE_MAX || value.x <= JOYSTICK_DEADZONE_MIN) {
            uint16_t channelY_buff = handle->channelY;
            handle->channelY = handle->channelX;
            handle->channelX = channelY_buff;
            handle->yMax = value.x;
        }

        if (handle->yMax >= JOYSTICK_DEADZONE_MAX) {
            handle->yDir = 1;
        }
        else if (handle->yMax <= JOYSTICK_DEADZONE_MIN) {
            handle->yDir = 0;
        } else {
            return ERROR_JOYSTICK_CAL;
        }
        return ERROR_NONE;
    }
    case JOYSTICK_X_MAX:
        handle->xMax = Joystick_GetRawXY(handle).x;

        if (handle->xMax >= JOYSTICK_DEADZONE_MAX) {
            handle->xDir = 1;
        }
        else if (handle->xMax <= JOYSTICK_DEADZONE_MAX) {
            handle->xDir = 0;
        } else {
            return ERROR_JOYSTICK_CAL;
        }
        return ERROR_NONE;

    case JOYSTICK_Y_MIN:
        handle->yMin = Joystick_GetRawXY(handle).y;
        if ((handle->yMin >= handle->centerPosY && handle->yDir == 1) ||
            (handle->yMin <= handle->centerPosY && handle->yDir == 0)) {
            return ERROR_JOYSTICK_CAL;
        } else {
            return ERROR_NONE;
        }
    case JOYSTICK_X_MIN:
        handle->xMin = Joystick_GetRawXY(handle).x;
        if ((handle->xMin >= handle->centerPosX && handle->xDir == 1) ||
            (handle->xMin <= handle->centerPosX && handle->xDir == 0)) {
            return ERROR_JOYSTICK_CAL;
        }
        else {
            return ERROR_NONE;
        }
    default:
        return ERROR_JOYSTICK_NOT_A_DIRECTION;
    }
}

Joystick_XY Joystick_GetXY(const Joystick *handle)
{
    Joystick_XY raw = Joystick_GetRawXY(handle);
    Joystick_XY pos;

    pos.x = ((raw.x - handle->centerPosX) * 100);
    if (raw.x >= handle->centerPosX && handle->xDir == 1) {
        pos.x /= (int32_t)(handle->xMax - handle->centerPosX);
    } else {
        pos.x /= (int32_t)(handle->centerPosX - handle->xMin);
    }

    pos.y = ((raw.y - handle->centerPosY) * 100);
    if (raw.y >= handle->centerPosY && handle->yDir == 1) {
        pos.y /= (int32_t)(handle->yMax - handle->centerPosY);
    } else {
        pos.y /= (int32_t)(handle->centerPosY - handle->yMin);
    }

    //Clip joystick values if they exceeed 100/-100 due to inaccuracy
    if (pos.y > 100) {
        pos.y = 100;
    } else if (pos.y < -100) {
        pos.y = -100;
    }
    if (pos.x > 100) {
        pos.x = 100;
    }
    else if (pos.x < -100) {
        pos.x = -100;
    }

    return pos;
}
