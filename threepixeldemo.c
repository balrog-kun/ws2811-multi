/*
 * Copyright 2012 Alan Burlison, alan@bleaklow.com.  All rights reserved.
 * Use is subject to license terms.
 *
 * Demo of the WS2811 driver, driving 3 pixels.
 */

#include <avr/io.h>
#include <util/delay.h>
#include <WS2811.h>

// Adjust as necessary.
#define PORT  PORTB
#define DDR    DDRB
#define PIN       0
#define PAUSE  1000     // msec
#define DELAY    10	// msec

// Bit twiddling macros.
#define BIT(B)           (0x01 << (uint8_t)(B))
#define SET_BIT_HI(V, B) (V) |= (uint8_t)BIT(B)
#define SET_BIT_LO(V, B) (V) &= (uint8_t)~BIT(B)
 
// Define the output function.
DEFINE_WS2811_FN(WS2811RGB, PORT, PIN)

// Drive the three pixels in an infinite loop.
void threepixeldemo(void)
{
    // Configure pin for output.
    SET_BIT_HI(DDR, PIN);
    SET_BIT_LO(PORT, PIN);

    // off->red, off->green, off->blue
    RGB_t rgb[3] = {{0,0,0},{0,0,0},{0,0,0}};
    WS2811RGB(rgb, ARRAYLEN(rgb));
    _delay_ms(PAUSE);
    for (int i = 0; i < 255; i++) {
        rgb[0].r += 1; 
        rgb[1].g += 1; 
        rgb[2].b += 1; 
        WS2811RGB(rgb, ARRAYLEN(rgb));
        _delay_ms(DELAY);
    }

    // loop forever.
    for (;;) {
        // red->yellow, green->cyan, blue->magenta    
        for (int i = 0; i < 255; i++) {
            rgb[0].g += 1; 
            rgb[1].b += 1; 
            rgb[2].r += 1; 
            WS2811RGB(rgb, ARRAYLEN(rgb));
            _delay_ms(DELAY);
        }
        // yellow->green, cyan->blue, magenta->white
        for (int i = 0; i < 255; i++) {
            rgb[0].r -= 1; 
            rgb[1].g -= 1; 
            rgb[2].g += 1; 
            WS2811RGB(rgb, ARRAYLEN(rgb));
            _delay_ms(DELAY);
        }
        // green->cyan, blue->magenta, white->off
        for (int i = 0; i < 255; i++) {
            rgb[0].b += 1; 
            rgb[1].r += 1; 
            rgb[2].r -= 1; 
            rgb[2].g -= 1; 
            rgb[2].b -= 1; 
            WS2811RGB(rgb, ARRAYLEN(rgb));
            _delay_ms(DELAY);
        }
        // cyan->blue, magenta->white, off->red
        for (int i = 0; i < 255; i++) {
            rgb[0].g -= 1;
            rgb[1].g += 1;
            rgb[2].r += 1;
            WS2811RGB(rgb, ARRAYLEN(rgb));
            _delay_ms(DELAY);
        }
        // blue->magenta, white->off, red->yellow
        for (int i = 0; i < 255; i++) {
            rgb[0].r += 1;
            rgb[1].r -= 1;
            rgb[1].g -= 1;
            rgb[1].b -= 1;
            rgb[2].g += 1;
            WS2811RGB(rgb, ARRAYLEN(rgb));
            _delay_ms(DELAY);
        }
        // magenta->white, off->red, yellow->green
        for (int i = 0; i < 255; i++) {
            rgb[0].g += 1;
            rgb[1].r += 1;
            rgb[2].r -= 1;
            WS2811RGB(rgb, ARRAYLEN(rgb));
            _delay_ms(DELAY);
        }
        // white->off, red->yellow, green->cyan
        for (int i = 0; i < 255; i++) {
            rgb[0].r -= 1;
            rgb[0].g -= 1;
            rgb[0].b -= 1;
            rgb[1].g += 1;
            rgb[2].b += 1;
            WS2811RGB(rgb, ARRAYLEN(rgb));
            _delay_ms(DELAY);
        }
        // off->red, yellow->green, cyan->blue
        for (int i = 0; i < 255; i++) {
            rgb[0].r += 1;
            rgb[1].r -= 1;
            rgb[2].g -= 1;
            WS2811RGB(rgb, ARRAYLEN(rgb));
            _delay_ms(DELAY);
        }
    }
}
