#ifndef COLOR_H_
#define COLOR_H_

#include <stdbool.h>
#include <stdint.h>

typedef struct
__attribute__((__packed__))
{
    unsigned r : 5;
    unsigned g : 6;
    unsigned b : 5;
} ColorData;

typedef union
__attribute__((__packed__))
{
    ColorData channels;
    uint16_t  mask;
} Color;

#define COLOR_RGB(r, g, b) (Color){ .channels  = { (r), (g), (b) } }

#define COLOR_BLACK    COLOR_RGB(0x00, 0x00, 0x00)
#define COLOR_WHITE    COLOR_RGB(0x1F, 0x3F, 0x1F)
#define COLOR_RED      COLOR_RGB(0x1F, 0x00, 0x00)
#define COLOR_GREEN    COLOR_RGB(0x00, 0x3F, 0x00)
#define COLOR_BLUE     COLOR_RGB(0x00, 0x00, 0x1F)
#define COLOR_CYAN     COLOR_RGB(0x00, 0x3F, 0x1F)
#define COLOR_MAGENTA  COLOR_RGB(0x1F, 0x00, 0x1F)
#define COLOR_YELLOW   COLOR_RGB(0x1F, 0x3F, 0x00)


#endif // #ifndef COLOR_H_