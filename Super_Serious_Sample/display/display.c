/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include "display.h"
#include "SSD1306.h"
#include "SSD1331.h"
#include "../lib/GPIO.h"

#define NUM_ALLOCATED_DISPLAYS 2
#define PRIMITIVES_PER_DISPLAY 256

// TODO: work out how to allocate the correct frame size on demand for each display type
static uint8_t I2C_FrameData[(SSD1306_WIDTH * SSD1306_HEIGHT) / 8];
static Color   SPI_FrameData[SSD1331_WIDTH * SSD1331_HEIGHT];

struct Frame {
    void             *data;
    Vector            size;
    Display_BitDepth  bitDepth;
    bool              invertedFormat;
};

static uint32_t Frame_GetBufferLength(Frame *frame)
{
    uint32_t pixels = frame->size.x * frame->size.y;
    uint32_t bits   = pixels * frame->bitDepth;
    uint32_t bytes  = bits / 8;
    return bytes;
}

bool Frame_DrawPixel(Frame *frame, Vector position, Color color)
{
    // TODO: do we want to batch the writes? Avoiding traversing the function ptr too much?
    if (!frame) {
        return false;
    }

    if ((position.x >= frame->size.x) || (position.x < 0)
        || (position.y >= frame->size.y) || (position.y < 0)) {
        return false;
    }

    // TODO: need to consider bitDepth here
    if (frame->invertedFormat) {
        uint8_t *byte = &(((uint8_t*)frame->data)[((position.x * frame->size.y) + position.y) / 8]);
        unsigned n = position.y % 8;
        uint8_t mask = 1U << n;
        if (color.mask) {
            *byte |= mask; // Set
        } else {
            *byte &= ~mask; // Clear
        }
    }
    else {
        ((Color*)frame->data)[(position.y * frame->size.x) + position.x].mask = color.mask;
    }

    return true;
}

struct Display {
    bool           open;
    void          *driver;
    Display_Type   type;
    Platform_Unit  isu;
    Color          background;
    Frame          frame;
    Primitive     *pHead;
    Primitive     *pTail;
    bool           (*drawFrame)(Display *display);
};

static Display allocatedDisplays[NUM_ALLOCATED_DISPLAYS] = {{0}};

static bool I2C_DrawFrame(Display *display)
{
    if (!display || !display->driver) {
        return false;
    }

    // TODO: just write a diff
    Frame frame = display->frame;
    if ((frame.size.x != SSD1306_WIDTH) || (frame.size.y != SSD1306_HEIGHT)) {
        return false;
    }
    return SSD1306_WriteFullBuffer(
        (I2CMaster *)display->driver, (uint8_t *)frame.data,
        Frame_GetBufferLength(&frame));
}

static bool SPI_DrawFrame(Display *display)
{
    if (!display || !display->driver) {
        return false;
    }
    // TODO: work out if there's a way to just write a diff
    Frame frame = display->frame;
    if ((frame.size.x != SSD1331_WIDTH) || (frame.size.y != SSD1331_HEIGHT)) {
        return false;
    }
    return SSD1331_Upload((SSD1331 *)display->driver,
                          (Color *)frame.data, Frame_GetBufferLength(&frame));
}

static I2CMaster* I2C_DriverInit(Display *display, Platform_Unit isu)
{
    I2CMaster *i2cDriver;

    i2cDriver = I2CMaster_Open(isu);
    if (!i2cDriver) {
        return NULL;
    }
    I2CMaster_SetBusSpeed(i2cDriver, I2C_BUS_SPEED_FAST_PLUS);

    if (!Ssd1306_Init(i2cDriver)) {
        return NULL;
    }

    if (!SSD1306_SetDisplayAllOn(i2cDriver, false)) {
        return NULL;
    }

    return i2cDriver;
}

static SSD1331* SPI_DriverInit(Display *display, Platform_Unit isu, const DisplaySPIPinConfig *config)
{
    // TODO: see if we need / want to pass config
    if (!config) {
        config = &displaySPIConfigDefault;
    }
    // Set the pins as outputs.
    GPIO_ConfigurePinForOutput(config->pin0);
    GPIO_ConfigurePinForOutput(config->pin1);
    GPIO_ConfigurePinForOutput(config->pin2);
    GPIO_ConfigurePinForOutput(config->pin3);

    SPIMaster *spiDriver;

    spiDriver = SPIMaster_Open(isu);
    if (!spiDriver) {
        return NULL;
    }

    SSD1331 *spiDisplay = SSD1331_Open(spiDriver, 0, 1, 2, 3);
    if (!spiDisplay) {
        return NULL;
    }

    return spiDisplay;
}

Display* Display_Open(Display_Type type, Platform_Unit isu)
{
    if ((type < DISPLAY_TYPE_1306_I2C) || (type >= DISPLAY_TYPE_COUNT)
        || (isu < MT3620_UNIT_ISU0) || (isu > MT3620_UNIT_ISU5)) {
        return NULL;
    }

    // Search for avaliable displays
    Display *display = NULL;
    for (unsigned i = 0; i < NUM_ALLOCATED_DISPLAYS; i++) {
        if (!allocatedDisplays[i].open) {
            display = &allocatedDisplays[i];
            break;
        }
    }

    if (!display) {
        return NULL;
    }

    switch (type) {
    case DISPLAY_TYPE_1306_I2C:
        display->driver = I2C_DriverInit(display, isu);
        if (!display->driver) {
            return NULL;
        }

        display->drawFrame            = I2C_DrawFrame;
        display->frame.data           = I2C_FrameData;
        display->frame.bitDepth       = DISPLAY_BITDEPTH_1_BIT;
        display->frame.size           = (Vector){ SSD1306_WIDTH, SSD1306_HEIGHT };
        display->frame.invertedFormat = true;

        break;

    case DISPLAY_TYPE_1331_SPI:
        display->driver = SPI_DriverInit(display, isu, NULL);
        if (!display->driver) {
            return NULL;
        }

        display->drawFrame            = SPI_DrawFrame;
        display->frame.data           = SPI_FrameData;
        display->frame.bitDepth       = DISPLAY_BITDEPTH_16_BIT;
        display->frame.size           = (Vector){ SSD1331_WIDTH, SSD1331_HEIGHT };
        display->frame.invertedFormat = false;

        break;

    default:
        return NULL;
    }

    display->open = true;

    display->type = type;
    display->isu  = isu;
    display->background = COLOR_BLACK;

    Display_Clear(display);

    display->pHead  = NULL;
    display->pHead  = NULL;

    return display;
}

void Display_Close(Display *display)
{
    if (!display || !display->open) {
        return;
    }

    display->open = false;
}

bool Display_Draw(const Display *display)
{
    if (!display || !display->open) {
        return false;
    }
    // Update frame
    // TODO: just write a diff to the frame / driver

    Primitive *current = display->pHead;
    if (current && current->used) {
        while (current != NULL) {
            current->draw(current, &display->frame);
            current = current->next;
        }
    }

    return display->drawFrame(display);
}

Primitive* Display_PrimitiveAlloc(Display *display)
{
    if (!display) {
        return NULL;
    }

    Primitive *out = NULL;
    for (unsigned i = 0; i < PRIMITIVES_PER_DISPLAY; i++) {
        if (!display->primitive[i].used) {
            out = &(display->primitive[i]);
            break;
        }
    }

    if (!out) {
        return NULL;
    }
    out->used = true;

    if (!display->pHead) {
        display->pHead = out;
    }

    if (display->pHead) {
        display->pHead->next = out;
    }
    display->pHead = out;

    return out;
}

bool Display_PrimitiveFree(Display *display, Primitive *primitive)
{
    if (!display || !primitive) {
        return false;
    }

    Primitive *current = display->pHead;
    Primitive *last    = NULL;
    bool       found   = false;
    while (current != NULL) {
        if (current == primitive) {
            // handle when removed primitive is head or tail
            if (current == display->pHead) {
                display->pHead = current->next;
            }
            if (current == display->pHead) {
                display->pHead = last;
            }

            // point last at next
            if (last != NULL) {
                last->next = current->next;
            }

            Primitive_Reset(current);
            found = true;
            break;
        }
        last    = current;
        current = current->next;
    }

    return found;
}

bool Display_Clear(Display *display)
{
    if (!display) {
        return false;
    }

    __builtin_memset(display->frame.data, 0xFF, Frame_GetBufferLength(&display->frame));
    return true;
}

bool Display_SetBackground(Display *display, Color color)
{
    if (!display) {
        return false;
    }

    display->background = color;

    bool success = true;
    Vector pixel;

    for (pixel.y = 0; pixel.y < display->frame.size.y; pixel.y++) {
        for (pixel.x = 0; pixel.x < display->frame.size.x; pixel.x++) {
            success &= Frame_DrawPixel(&(display->frame), pixel, color);
        }
    }

    return success;
}
