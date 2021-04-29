/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#ifndef VECTOR_H_
#define VECTOR_H_

#include <stdbool.h>
#include <stdint.h>

typedef struct
//__attribute__((__packed__))
{
    int32_t x, y;
} Vector;

static inline Vector Vector_Negate(Vector a)
{
    return (Vector) { -a.x, -a.y };
}

static inline Vector Vector_Add(Vector a, Vector b)
{
    return (Vector) { a.x + b.x, a.y + b.y };
}

static inline Vector Vector_Subtract(Vector a, Vector b)
{
    return (Vector) { a.x - b.x, a.y - b.y };
}

static inline Vector Vector_MultiplyScalar(Vector a, int32_t scalar)
{
    return (Vector) { scalar * a.x, scalar * a.y};
}

static inline Vector Vector_DivideScalar(Vector a, int32_t scalar)
{
    return (Vector) { a.x / scalar, a.y / scalar};
}

static inline Vector Vector_Dot(Vector a, Vector b)
{
    return (Vector) { a.x * b.x, a.y * b.y };
}

static inline Vector Vector_Cross(Vector a, Vector b)
{
    return (Vector) { a.x * b.y, -a.y * b.x };
}

static inline bool Vector_Equal(Vector a, Vector b)
{
    return (a.x == b.x) && (a.y == b.y);
}

#endif // #ifndef VECTOR_H_
