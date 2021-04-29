/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#ifndef DISPLAY_H_
#define DISPLAY_H_

#include <stdbool.h>
#include <stdint.h>

#include "../lib/Platform.h"
#include "color.h"
#include "vector.h"

typedef enum {
    DISPLAY_BITDEPTH_1_BIT  = 1,
    DISPLAY_BITDEPTH_16_BIT = 16,
    DISPLAY_BITDEPTH_COUNT
} Display_BitDepth;

typedef struct Frame Frame;

bool Frame_DrawPixel(Frame *frame, Vector position, Color color);

typedef struct Display Display;

typedef enum {
    DISPLAY_TYPE_1306_I2C,
    DISPLAY_TYPE_1331_SPI,
    DISPLAY_TYPE_COUNT
} Display_Type;

typedef struct
{
    uint32_t pin0;
    uint32_t pin1;
    uint32_t pin2;
    uint32_t pin3;
} DisplaySPIPinConfig;

static const DisplaySPIPinConfig displaySPIConfigDefault =
{
    .pin0  = 0,
    .pin1  = 1,
    .pin2  = 2,
    .pin3  = 3,
};

# include "primitive.h"

Display*   Display_Open           (Display_Type type, Platform_Unit isu);
void       Display_Close          (Display *display);
bool       Display_Draw           (const Display *display);
bool       Display_PrimitiveAdd   (Display *display, Primitive *primitive);
bool       Display_PrimitiveRemove(Display *display, Primitive *primitive);
bool       Display_Clear          (Display *display);
bool       Display_SetBackground  (Display *display, Color color);

#endif // #ifndef DISPLAY_H_
