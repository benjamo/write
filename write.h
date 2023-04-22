/**
 * Write is a simple rewrite for displaying text on an display
 * 
*/
#pragma once

#include <pico/stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string>
#include <cstring>

#include "gfxfont.h"
#include "font.c"

// Many (but maybe not all) non-AVR board installs define macros
// for compatibility with existing PROGMEM-reading AVR code.
// Do our own checks and defines here for good measure...

#ifndef pgm_read_byte
#define pgm_read_byte(addr) (*(const unsigned char *)(addr))
#endif
#ifndef pgm_read_word
#define pgm_read_word(addr) (*(const unsigned short *)(addr))
#endif
#ifndef pgm_read_dword
#define pgm_read_dword(addr) (*(const unsigned long *)(addr))
#endif

inline GFXglyph *pgm_read_glyph_ptr(const GFXfont *gfxFont, uint8_t c) {
#ifdef __AVR__
  return &(((GFXglyph *)pgm_read_pointer(&gfxFont->glyph))[c]);
#else
  // expression in __AVR__ section may generate "dereferencing type-punned
  // pointer will break strict-aliasing rules" warning In fact, on other
  // platforms (such as STM32) there is no need to do this pointer magic as
  // program memory may be read in a usual way So expression may be simplified
  return gfxFont->glyph + c;
#endif //__AVR__
}

inline uint8_t *pgm_read_bitmap_ptr(const GFXfont *gfxFont) {
#ifdef __AVR__
  return (uint8_t *)pgm_read_pointer(&gfxFont->bitmap);
#else
  // expression in __AVR__ section generates "dereferencing type-punned pointer
  // will break strict-aliasing rules" warning In fact, on other platforms (such
  // as STM32) there is no need to do this pointer magic as program memory may
  // be read in a usual way So expression may be simplified
  return gfxFont->bitmap;
#endif //__AVR__
}

class Write {
    public:
        Write(uint8_t* buffer);
        void print(const std::string& text);
        void print(const char c);
        void println(const std::string& text);
        void printerf(const char *format, ...);
        void newline();
        uint8_t _unicodeEasy(uint8_t c);
        void setRotation(uint8_t x);
    private:
        int16_t _width;         // Display width as modified by current rotation
        int16_t _height;        // Display height as modified by current rotation
        int16_t cursor_x;       // x location to start print()ing text
        int16_t cursor_y;       // y location to start print()ing text
        uint16_t textcolor;     // 16-bit background color for print()
        int16_t textbgcolor;    // 16-bit text color for print()
        uint8_t textsize_x;     // Desired magnification in X-axis of text to print()
        uint8_t textsize_y;     // Desired magnification in Y-axis of text to print()
        bool wrap;
        int16_t WIDTH;          // This is the 'raw' display width - never changes
        int16_t HEIGHT;         // This is the 'raw' display height - never changes
        uint8_t *_mono_buffer; 
        GFXfont *gfxFont;       // Pointer to special font
        uint8_t rotation;
        size_t writeC(uint8_t c);
        size_t writeC(const char *str);
        size_t writeC(const uint8_t *buffer, size_t size);
        size_t writeC(const char *buffer, size_t size);
        void drawChar(int16_t x, int16_t y, unsigned char c, uint16_t color, uint16_t bg, uint8_t size);
        void drawChar(int16_t x, int16_t y, unsigned char c, uint16_t color, uint16_t bg, uint8_t size_x, uint8_t size_y);
        void swap(int16_t &a, int16_t &b);  
        uint8_t getRotation(void);
        void drawPixel(int16_t x, int16_t y, uint16_t color);
};