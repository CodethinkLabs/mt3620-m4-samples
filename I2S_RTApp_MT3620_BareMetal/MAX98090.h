/* Copyright (c) Codethink Ltd. All rights reserved.
   Licensed under the MIT License. */

#ifndef MAX98090_H_
#define MAX98090_H_

#include <stdbool.h>
#include <stdint.h>
#include "lib/GPT.h"
#include "lib/I2CMaster.h"
#include "lib/I2S.h"

typedef struct MAX98090 MAX98090;

typedef enum {
    MAX98090_VARIANT_A = 0,
    MAX98090_VARIANT_B = 1,
    MAX98090_VARIANT_COUNT
} MAX98090_Variant;

typedef enum {
    MAX98090_OUTPUT_HEADPHONE = 0,
    MAX98090_OUTPUT_RECEIVER,
    MAX98090_OUTPUT_SPEAKER,
    MAX98090_OUTPUT_LINE_OUT,
    MAX98090_OUTPUT_COUNT
} MAX98090_Output;

MAX98090 *MAX98090_Open(
    I2CMaster *bus, Platform_Unit interface, GPT *timer,
    MAX98090_Variant variant, bool mclkExternal, unsigned mclk);
void MAX98090_Close(MAX98090 *handle);

bool MAX98090_Reset(MAX98090 *handle);

bool MAX98090_OutputEnable(MAX98090 *handle,
    MAX98090_Output output, unsigned channels, unsigned bits, unsigned rate,
    bool (*callback)(void *, uintptr_t));
bool MAX98090_InputEnable(MAX98090 *handle,
    unsigned input, unsigned channels, unsigned bits, unsigned rate,
    bool (*callback)(void *, uintptr_t));

#endif // #ifndef MAX98090_H_
