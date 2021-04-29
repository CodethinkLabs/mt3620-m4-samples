/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include "display.h"

#ifndef PRIMITIVE_H_
#define PRIMITIVE_H_

#include <stdbool.h>
#include <stdint.h>

#include "color.h"

typedef struct Primitive Primitive;

typedef struct PrimitiveNode PrimitiveNode;
struct PrimitiveNode {
    Primitive primitive;

    PrimitiveNode *child;
    PrimitiveNode *next;
    bool           used;
};

Primitive *Primitive_Point    (Vector pos);
Primitive *Primitive_Line     (Vector start, Vector end);
Primitive *Primitive_Circle   (Vector center, uint32_t radius);
Primitive *Primitive_Rectangle(Vector topLeft, Vector bottomRight);

void Primitive_Reset(Primitive *primitive);

#endif // #ifndef PRIMITIVE_H_
