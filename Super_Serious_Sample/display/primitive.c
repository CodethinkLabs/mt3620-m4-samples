/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include <stdlib.h>

#include "primitive.h"

#define MAX_NUM_PRIMITIVES 512

static Primitive pool[MAX_NUM_PRIMITIVES];

typedef struct {
    Vector point;
} PointData;

typedef struct {
    Vector start;
    Vector end;
} LineData;

typedef struct {
    Vector   center;
    uint32_t radius;
} CircleData;

typedef struct {
    Vector topLeft;
    Vector bottomRight;
} RectangleData;

typedef struct {
    union {
        PointData     point;
        LineData      line;
        CircleData    circle;
        RectangleData rectangle;
    };

    Vector   offset;
    Color    color;
    uint32_t thickness;
    bool     filled;
} PrimitiveData;

struct Primitive {
    PrimitiveData  data;
    bool (*draw)(
        const Primitive *primitive,
        Frame           *frame);
};

static void Primitive__ClearChildren(
    Primitive *primitive)
{
    if (!primitive) {
        return;
    }

    Primitive *temp;
    while (primitive->child) {
        temp             = primitive->child;
        primitive->child = NULL;
        primitive        = temp;
    }
}

static Primitive *Primitive__New(
    PrimitiveData *data,
    bool          (*draw)(const Primitive *primitive, Frame *frame))
{
    Primitive *out = NULL;
    for (unsigned i = 0; i < MAX_NUM_PRIMITIVES; i++) {
        if (!pool[i].used) {
            out = &(pool[i]);
            break;
        }
    }

    if (!out) {
        return NULL;
    }
    out->used = true;

    out->data  = *data;
    out->child = NULL;
    out->next  = NULL;
    out->used  = false;
    out->draw  = draw;

    return out;
}

void Primitive_Reset(Primitive *primitive)
{
    if (!primitive) {
        return;
    }

    primitive->next = NULL;
    primitive->used = false;

    Primitive__ClearChildren(primitive);
}

bool Primitive_AddChild(Primitive *primitive, Primitive *child)
{
    if (!primitive || !child) {
        return false;
    }

    while (primitive->child) {
        primitive = primitive->child;
    }
    primitive->child = child;
}

bool Primitive_RemoveChild(Primitive *primitive, Primitive *child)
{
    if (!primitive || !child) {
        return false;
    }

    while (primitive->child) {
        if (primitive->child == child) {
            primitive->child = primitive->child->child;
            return true;
        }
        primitive = primitive->child;
    }
    return false;
}

static bool drawPoint(const Primitive *primitive, Frame *frame)
{
    if (!primitive || !frame) {
        return false;
    }

    return Frame_DrawPixel(frame, primitive->data.point.point, primitive->data.color);
}

static bool drawLine(const Primitive *primitive, Frame *frame)
{
    if (!primitive || !frame) {
        return false;
    }

    // Midpoint line algorithm, first ensure left.x < right.x
    Vector left, right;
    if (primitive->data.line.start.x < primitive->data.line.end.x) {
        left  = primitive->data.line.start;
        right = primitive->data.line.end;
    }
    else {
        left  = primitive->data.line.end;
        right = primitive->data.line.start;
    }

    // ensure that left.y < right.y, if not reflect
    bool reflectHoriz = (left.y >= right.y);
    int32_t dx = right.x - left.x;
    int32_t dy = right.y - left.y;;
    if (reflectHoriz) {
        dy = -dy;
    }

    // ensure that m < 1, if not reflect
    bool reflect45 = ((dx == 0) || ((dy / dx) > 1));

    if (reflect45) {
        int32_t temp = dx;
        dx = dy;
        dy = temp;
    }

    // initial decision value
    int32_t decision = (2 * dy) - dx;

    int32_t incr_x  = 2 * dy;
    int32_t incr_xy = 2 * (dy - dx);

    int32_t incr_y;
    if (!reflectHoriz) {
        incr_y = 1;
    }
    else {
        incr_y = -1;
    }

    bool success = true;
    Vector pixel, temp;

    for (temp = left; (temp.x <= right.x) && success; temp.x++) {
        pixel.x = reflect45 ? temp.y : temp.x;
        pixel.y = reflect45 ? temp.x : temp.y;
        success &= Frame_DrawPixel(frame, Vector_Add(pixel, primitive->data.offset),
                                   primitive->data.color);

        if (decision > 0) {
            // midpoint above line, so increment y too
            decision += incr_xy;
            temp.y += incr_y;
        }
        else {
            decision += incr_x;
        }
    }

    return success;
}

static bool drawCircle(const Primitive *primitive, Frame *frame)
{
    if (!primitive || !frame) {
        return false;
    }
    // Midpoint circle algorithm
    int32_t x = primitive->data.circle.radius;
    int32_t y = 0;

    Vector center = primitive->data.circle.center;

    bool success = true;

    int32_t decision = 1 - x;

    Vector pos = primitive->data.offset;

    while ((x >= y) && success) {
        success &= Frame_DrawPixel(frame, (Vector) { x + center.x + pos.x, y + center.y + pos.y },
                                   primitive->data.color);
        success &= Frame_DrawPixel(frame, (Vector) { -x + center.x + pos.x, y + center.y + pos.y },
                                   primitive->data.color);
        success &= Frame_DrawPixel(frame, (Vector) { x + center.x + pos.x, -y + center.y + pos.y },
                                   primitive->data.color);
        success &= Frame_DrawPixel(frame, (Vector) { -x + center.x + pos.x, -y + center.y + pos.y },
                                   primitive->data.color);

        if (x != y) {
            success &= Frame_DrawPixel(frame, (Vector) { y + center.x + pos.x, x + center.y + pos.y },
                                       primitive->data.color);
            success &= Frame_DrawPixel(frame, (Vector) { -y + center.x + pos.x, x + center.y + pos.y },
                                       primitive->data.color);
            success &= Frame_DrawPixel(frame, (Vector) { y + center.x + pos.x, -x + center.y + pos.y },
                                       primitive->data.color);
            success &= Frame_DrawPixel(frame, (Vector) { -y + center.x + pos.x, -x + center.y + pos.y },
                                       primitive->data.color);
        }

        y++;

        if (decision <= 0) {
            decision += (2*y) + 1;
        }
        else {
            x--;
            decision += (2*y) - (2*x) + 1;
            if (x < y) {
                break;
            }
        }
    }
    return success;
}

static bool drawRectangle(const Primitive *primitive, Frame *frame)
{
    if (!primitive || !frame) {
        return false;
    }

    Vector topLeft     = primitive->data.rectangle.topLeft;
    Vector bottomRight = primitive->data.rectangle.bottomRight;

    bool success = true;
    Vector pixelA, pixelB;

    Vector pos = primitive->data.offset;

    // draw verticals
    for (
        pixelA = topLeft,
        pixelB = (Vector){bottomRight.x, topLeft.y};
        pixelA.y <= bottomRight.y; pixelA.y++, pixelB.y++)
    {
        success &= Frame_DrawPixel(frame, Vector_Add(pixelA, pos), primitive->data.color);
        success &= Frame_DrawPixel(frame, Vector_Add(pixelB, pos), primitive->data.color);
    }

    // draw horizontals
    for (pixelA = (Vector){ topLeft.x + 1, topLeft.y }, pixelB = (Vector){topLeft.x + 1, bottomRight.y};
         pixelA.x < bottomRight.x;
         pixelA.x++, pixelB.x++)
    {
        success &= Frame_DrawPixel(frame, Vector_Add(pixelA, pos), primitive->data.color);
        success &= Frame_DrawPixel(frame, Vector_Add(pixelB, pos), primitive->data.color);
    }

    return success;
}

Primitive *Primitive_Point(Vector pos)
{
    PrimitiveData data = { .point.point = pos };
    return Primitive__New(&data, drawPoint);
}

Primitive *Primitive_Line(Vector start, Vector end)
{
    PrimitiveData data = {
        .line.start = start,
        .line.end   = end
    };
    return Primitive__New(&data, drawLine);
}

Primitive *Primitive_Circle(Vector center, uint32_t radius)
{
    PrimitiveData data = {
        .circle.center = center,
        .circle.radius = radius
    };
    return Primitive__New(&data, drawCircle);
}

Primitive *Primitive_Rectangle(Vector topLeft, Vector bottomRight)
{
    PrimitiveData data = {
        .rectangle.topLeft     = topLeft,
        .rectangle.bottomRight = bottomRight
    };
    return Primitive__New(&data, drawRectangle);
}
