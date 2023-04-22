#include "write.h"

/**
 * Contructor 
 * 
 * @param uint8_t *buffer
 * 
*/
Write::Write(uint8_t* buffer) {
    WIDTH = 176;
    HEIGHT = 264;
    _width = WIDTH;
    _height = HEIGHT;
    rotation = 0;
    cursor_y = cursor_x = 0;
    textsize_x = textsize_y = 1;
    textcolor = 0x00; 
    textbgcolor = 0xFFFF;
    wrap = true;
    // _cp437 = false;
    gfxFont = NULL;
    _mono_buffer = buffer;
}

/**
 * Print 
 * 
 * @param const std::string& text
 * 
*/
void Write::print(const std::string& text) {
    for(auto c : text) {
        if (c==195 || c==194) continue; // Skip to next letter
        c = _unicodeEasy(c);
        writeC(uint8_t(c));
    }
}

/**
 * Print 
 * 
 * @param const char c
 * 
 * @return void
*/
void Write::print(const char c) {
     writeC(uint8_t(c));
}

/**
 * println 
 * 
 * @param const std::string& text
 * 
 * @return void
*/
void Write::println(const std::string& text) {
    for(auto c : text) {

        if (c==195 || c==194) {
          continue; // Skip to next letter
        }

        // _unicodeEasy will just sum 64 and get the right character, should be faster and cover more chars
        c = _unicodeEasy(c);
        //c = _unicodePerChar(c); // _unicodePerChar has more control since they are only hand-picked chars
        writeC(uint8_t(c));
    }
    writeC(10); // newline
}

/**
 * @brief Similar to printf
 * 
 * Note that buffer needs to end with null character
 * 
 * @param format 
 * @param ... va_list
 * 
 * @return void
 */
void Write::printerf(const char *format, ...) {
    va_list args;
    va_start(args, format);
    char max_buffer[1024];
    int size = vsnprintf(max_buffer, sizeof max_buffer, format, args);
    va_end(args);

    if (size < sizeof(max_buffer)) {
        print(std::string(max_buffer));
    } else {
        printf("Write::printerf: max_buffer out of range. Increase max_buffer!");
    }
}

/**
 * @brief newline prints a newline
 * 
 * @return void
 */
void Write::newline() {
    writeC(10);
}

/**
 * @brief _unicodeEasy
 * 
 * @param uint8_t c 
 * @param ... va_list
 * 
 * @return uint8_t
 */
uint8_t Write::_unicodeEasy(uint8_t c) {

    if (c<191 && c>131 && c!=176) { // 176 is °W 
        c+=64;
    }

    return c;
}

/**
* @brief Print one byte/character of data, used to support print()
* 
* @param uint8_t c The 8-bit ascii character to write
*
* @return size_t
*/
size_t Write::writeC(uint8_t c) {

  if (!gfxFont) { // 'Classic' built-in font
   
    // printf("write(%d) Custom font\n",c);
    if (c == '\n') {              // Newline?
        cursor_x = 0;               // Reset x to zero,
        cursor_y += textsize_y * 8; // advance y one line
    } else if (c != '\r') {       // Ignore carriage returns

        if (wrap && ((cursor_x + textsize_x * 6) > _width)) { // Off right?
            cursor_x = 0;                                       // Reset x to zero,
            cursor_y += textsize_y * 8; // advance y one line
        }
        drawChar(cursor_x, cursor_y, c, textcolor, textbgcolor, textsize_x, textsize_y);
        cursor_x += textsize_x * 6; // Advance x one char

    }

  } else { // Custom font
    //printf("write(%d) Custom font\n",c);
    if (c == '\n') {

        cursor_x = 0;
        cursor_y += (int16_t)textsize_y * (uint8_t)pgm_read_byte(&gfxFont->yAdvance);

    } else if (c != '\r') {

        uint8_t first = pgm_read_byte(&gfxFont->first);

        if ((c >= first) && (c <= (uint8_t)pgm_read_byte(&gfxFont->last))) {
            //printf("write() >=%d <=%d CHAR is in range\n",first, (uint8_t)pgm_read_byte(&gfxFont->last));

            GFXglyph *glyph = pgm_read_glyph_ptr(gfxFont, c - first);
            uint8_t w = pgm_read_byte(&glyph->width),
                    h = pgm_read_byte(&glyph->height);

            if ((w > 0) && (h > 0)) { // Is there an associated bitmap?

              int16_t xo = (int8_t) pgm_read_byte(&glyph->xOffset); // sic

              if (wrap && ((cursor_x + textsize_x * (xo + w)) > _width)) {
                cursor_x = 0;
                cursor_y += (int16_t) textsize_y * (uint8_t) pgm_read_byte(&gfxFont->yAdvance);
              }

              drawChar(cursor_x, cursor_y, c, textcolor, textbgcolor, textsize_x, textsize_y);

            }

            cursor_x += (uint8_t) pgm_read_byte(&glyph->xAdvance) * (int16_t) textsize_x;

        }
    }
  }
  return 1;
}

/**
* @brief writeC
* 
* @param const char *str
*
* @return size_t
*/
size_t Write::writeC(const char *str) {

    if(str == NULL) {
        return 0;
    }

    return writeC((const uint8_t *) str, strlen(str));

}

/**
* @brief writeC
* 
* @param const uint8_t *buffer
* @param size_t size
*
* @return size_t
*/
size_t Write::writeC(const uint8_t *buffer, size_t size) {

    size_t n = 0;

    while(size--) {
        n += writeC(*buffer++);
    }

    return n;
}

/**
* @brief writeC
* 
* @param const char *buffer
* @param size_t size
*
* @return size_t
*/
size_t Write::writeC(const char *buffer, size_t size) {
    return writeC((const uint8_t *) buffer, size);
}

/**
* @brief Draw a single character
*   
* @param int16_t x Bottom left corner x coordinate
* @param int16_t y Bottom left corner y coordinate
* @param unsigned char c The 8-bit font-indexed character (likely ascii)
* @param uint16_t color 16-bit 5-6-5 Color to draw chraracter with
* @param uint16_t bg 16-bit 5-6-5 Color to fill background with (if same as color, no background)
* @param uint8_t size  Font magnification level, 1 is 'original' size
*/
void Write::drawChar(int16_t x, int16_t y, unsigned char c, uint16_t color, uint16_t bg, uint8_t size) {
    drawChar(x, y, c, color, bg, size, size);
}

/**
*
*    @brief   Draw a single character
*    @param    x   Bottom left corner x coordinate
*    @param    y   Bottom left corner y coordinate
*    @param    c   The 8-bit font-indexed character (likely ascii)
*    @param    color 16-bit 5-6-5 Color to draw chraracter with
*    @param    bg 16-bit 5-6-5 Color to fill background with (if same as color, no background)
*    @param    size_x  Font magnification level in X-axis, 1 is 'original' size
*    @param    size_y  Font magnification level in Y-axis, 1 is 'original' size
*/
void Write::drawChar(int16_t x, int16_t y, unsigned char c, uint16_t color, uint16_t bg, uint8_t size_x, uint8_t size_y) {

  if (!gfxFont) { // 'Classic' built-in font

    if ((x >= _width) ||              // Clip right
        (y >= _height) ||             // Clip bottom
        ((x + 6 * size_x - 1) < 0) || // Clip left
        ((y + 8 * size_y - 1) < 0))   // Clip top
      return;

    /*
    if (!_cp437 && (c >= 176))
      c++; // Handle 'classic' charset behavior
    */

    // startWrite();
    for (int8_t i = 0; i < 5; i++) { // Char bitmap = 5 columns

        uint8_t line = pgm_read_byte(&font[c * 5 + i]);

        for (int8_t j = 0; j < 8; j++, line >>= 1) {
          
            if (line & 1) {
                if (size_x == 1 && size_y == 1) {
                  drawPixel(x + i, y + j, color);
                } else {
                    // writeFillRect(x + i * size_x, y + j * size_y, size_x, size_y, color);
                }

            } else if (bg != color) {

                if (size_x == 1 && size_y == 1) {
                  drawPixel(x + i, y + j, bg);
                } else {
                    // writeFillRect(x + i * size_x, y + j * size_y, size_x, size_y, bg);
                }
            }

        }
    }

    if (bg != color) { // If opaque, draw vertical line for last column

        if (size_x == 1 && size_y == 1) {
            // writeFastVLine(x + 5, y, 8, bg);
        } else {
            // writeFillRect(x + 5 * size_x, y, size_x, 8 * size_y, bg);
        }
        
        // endWrite();
    }

  } else { // Custom font

      // printf("draw(%d) Custom font in x:%d y:%d\n",c,x,y);
      // Character is assumed previously filtered by write() to eliminate
      // newlines, returns, non-printable characters, etc.  Calling
      // drawChar() directly with 'bad' characters of font may cause mayhem!

      c -= (uint8_t)pgm_read_byte(&gfxFont->first);
      GFXglyph *glyph = pgm_read_glyph_ptr(gfxFont, c);
      uint8_t *bitmap = pgm_read_bitmap_ptr(gfxFont);

      uint16_t bo = pgm_read_word(&glyph->bitmapOffset);
      uint8_t w = pgm_read_byte(&glyph->width), h = pgm_read_byte(&glyph->height);
      int8_t xo = pgm_read_byte(&glyph->xOffset),
            yo = pgm_read_byte(&glyph->yOffset);
      uint8_t xx, yy, bits = 0, bit = 0;
      int16_t xo16 = 0, yo16 = 0;

      if (size_x > 1 || size_y > 1) {
        xo16 = xo;
        yo16 = yo;
      }

    // Todo: Add character clipping here

    // NOTE: THERE IS NO 'BACKGROUND' COLOR OPTION ON CUSTOM FONTS.
    // THIS IS ON PURPOSE AND BY DESIGN.  The background color feature
    // has typically been used with the 'classic' font to overwrite old
    // screen contents with new data.  This ONLY works because the
    // characters are a uniform size; it's not a sensible thing to do with
    // proportionally-spaced fonts with glyphs of varying sizes (and that
    // may overlap).  To replace previously-drawn text when using a custom
    // font, use the getTextBounds() function to determine the smallest
    // rectangle encompassing a string, erase the area with fillRect(),
    // then draw new text.  This WILL infortunately 'blink' the text, but
    // is unavoidable.  Drawing 'background' pixels will NOT fix this,
    // only creates a new set of problems.  Have an idea to work around
    // this (a canvas object type for MCUs that can afford the RAM and
    // displays supporting setAddrWindow() and pushColors()), but haven't
    // implemented this yet.

    // startWrite();
    for (yy = 0; yy < h; yy++) {
      for (xx = 0; xx < w; xx++) {
        if (!(bit++ & 7)) {
          bits = pgm_read_byte(&bitmap[bo++]);
        }
        if (bits & 0x80) {
          if (size_x == 1 && size_y == 1) {
            drawPixel(x + xo + xx, y + yo + yy, color);
          } else {
           //  writeFillRect(x + (xo16 + xx) * size_x, y + (yo16 + yy) * size_y, size_x, size_y, color);
          }
        }
        bits <<= 1;
      }
    }
    // endWrite();

  }
}

/**
* @brief swap coordinates
* 
* @param int16_t &a
* @param int16_t &b
* 
* @return void
*/
void Write::swap(int16_t &a, int16_t &b) {                                                                           
    int16_t t = a; 
    a = b;
    b = t;
}

/**
* @brief      Get rotation setting for display
*
* @returns    0 thru 3 corresponding to 4 cardinal rotations
*/
uint8_t Write::getRotation(void) {
    return rotation; 
}

/**
* @brief Set rotation setting for display
* 
* @param uint8_t x  0 thru 3 corresponding to 4 cardinal rotations
*
* @return void
*/
void Write::setRotation(uint8_t x) {

  rotation = (x & 3);

  switch (rotation) {
    case 0:
    case 2:
        _width = WIDTH;
        _height = HEIGHT;
        break;
    case 1:
    case 3:
        _width = HEIGHT;
        _height = WIDTH;
        break;
    }
}

/**
 * @brief DrawPixel for drawing MonoPixels on an E-Ink display
 * 
 * @param int16_t x 
 * @param int16_t y
 * @param uint16_t color
 * 
 * @return void
*/
void Write::drawPixel(int16_t x, int16_t y, uint16_t color) {

  /*
  if ((x < 0) || (x >= WIDTH) || (y < 0) || (y >= HEIGHT)) {
    return;
  }*/

  // check rotation, move pixel around if necessary
  
  switch (getRotation()) {

    case 1:
      swap(x, y);
      x = WIDTH - x - 1;
      break;
    
    case 2:
      x = WIDTH - x - 1;
      y = HEIGHT - y - 1;
      break;
    
    case 3:
      swap(x, y);
      y = HEIGHT - y - 1;
      break;
  }

  uint16_t i = x / 8 + y * WIDTH / 8;

 if (1) {
    // This is the trick to draw colors right. Genious Jean-Marc
    if (color) {
        _mono_buffer[i] = (_mono_buffer[i] & (0xFF ^ (1 << (7 - x % 8))));
                //printf("color\n");
        //printf("%d", _mono_buffer[i]);
      } else {
        
        _mono_buffer[i] = (_mono_buffer[i] | (1 << (7 - x % 8)));
        //printf("%d", _mono_buffer[i]);
      }
 } /*else {
  // 4 gray mode
  uint8_t mask = 0x80 >> (x & 7);

  color >>= 6; // Color is from 0 (black) to 255 (white)
      
      switch (color)
      {
      case 1:
        // Dark gray: Correct
        _buffer1[i] = _buffer1[i] | mask;
        _buffer2[i] = _buffer2[i] & (0xFF ^ mask);
        break;
      case 2:
        // Light gray: Correct
        _buffer1[i] = _buffer1[i] & (0xFF ^ mask);
        _buffer2[i] = _buffer2[i] | mask;
        break;
      case 3:
        // WHITE
        _buffer1[i] = _buffer1[i] | mask;
        _buffer2[i] = _buffer2[i] | mask;
        break;
      default:
        // Black
        _buffer1[i] = _buffer1[i] & (0xFF ^ mask);
        _buffer2[i] = _buffer2[i] & (0xFF ^ mask);
        break;
      }
 }*/
}