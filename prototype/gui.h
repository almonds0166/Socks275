
/*
 * BUTTONS
 */
 
// https://www.pjrc.com/teensy/td_libs_Bounce.html
#include <Bounce.h>

// button pin definitions
#define BTN_UP   A6
#define BTN_DOWN A5
#define BTN_SET  A4 // claim desired pressure
#define BTN_E    A3 // emergency valve release / reset desired pressure

// response rate of buttons
#define BTN_DEBOUNCE 100 // ms

/*
 * OLED display module
 */

/*
 * Useful OLED routines for my reference:
 * .clearBuffer(); // ready buffer for new screen
 * .setFont(u8g2_font_ncenB10_tr); // https://github.com/olikraus/u8g2/wiki/fntgrp
 * .drawStr(x, y, "I hope you're having a nice day!"); // write string at position 
 * .drawBox(x, y, w, h); // draw filled box
 * .updateDisplayArea(x, y, w, h); // send buffer for specified area
 * 
 * See here for more https://github.com/olikraus/u8g2/wiki/u8g2reference
 */
 
// https://github.com/olikraus/u8g2
#include <SPI.h>
#include <U8g2lib.h>

#define SPI_CLK 14 // move the SPI SCK pin from default of 13

#define OLED_ROTATION U8G2_R2 // R2 = 180 degrees
#define OLED_CS 10
#define OLED_DC 15
#define OLED_RESET 16

#define OLED_FONT u8g2_font_helvR08_tf
