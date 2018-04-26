// Display Library for SPI e-paper panels from Dalian Good Display and boards from Waveshare.
// Requires HW SPI and Adafruit_GFX. Caution: these e-papers require 3.3V supply AND data lines!
//
// based on Demo Example from Good Display: http://www.good-display.com/download_list/downloadcategoryid=34&isMode=false.html
//
// Author: Jean-Marc Zingg
//
// Version: see library.properties
//
// License: GNU GENERAL PUBLIC LICENSE V3, see LICENSE
//
// Library: https://github.com/ZinggJM/GxEPD2_AVR

#include "GxEPD2_32_3C.h"
#include "WaveTables.h"

// this workaround for GDEW042Z15 updates the whole screen
#define USE_PARTIAL_UPDATE_WORKAROUND_ON_GDEW042Z15

const uint8_t GxEPD2_32_3C::bw2grey[] =
{
  0b00000000, 0b00000011, 0b00001100, 0b00001111,
  0b00110000, 0b00110011, 0b00111100, 0b00111111,
  0b11000000, 0b11000011, 0b11001100, 0b11001111,
  0b11110000, 0b11110011, 0b11111100, 0b11111111,
};

GxEPD2_32_3C::GxEPD2_32_3C(GxEPD2::Panel panel, int8_t cs, int8_t dc, int8_t rst, int8_t busy) :
  Adafruit_GFX(GxEPD2::ScreenDimensions[panel].width, GxEPD2::ScreenDimensions[panel].height),
  _panel(panel), _cs(cs), _dc(dc), _rst(rst), _busy(busy),
  _current_page(-1), _using_partial_mode(false), _mirror(false)
{
  _initial = true;
  _power_is_on = false;
  _width_bytes = uint16_t(WIDTH) / 8; // just discard any (WIDTH % 8) pixels
  _pixel_bytes = _width_bytes * uint16_t(HEIGHT); // save uint16_t range
  _page_height = buffer_size / _width_bytes;
  _pages = (HEIGHT / _page_height) + ((HEIGHT % _page_height) > 0);
  _busy_active_level = LOW;
}

void GxEPD2_32_3C::drawPixel(int16_t x, int16_t y, uint16_t color)
{
  if ((x < 0) || (x >= width()) || (y < 0) || (y >= height())) return;
  if (_mirror) x = width() - x - 1;
  // check rotation, move pixel around if necessary
  switch (getRotation())
  {
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
  uint16_t i = x / 8 + y * _width_bytes;
  if (_current_page < 1)
  {
    if (i >= sizeof(_black_buffer)) return;
  }
  else
  {
    y -= _current_page * _page_height;
    if ((y < 0) || (y >= _page_height)) return;
    i = x / 8 + y * _width_bytes;
  }

  _black_buffer[i] = (_black_buffer[i] & (0xFF ^ (1 << (7 - x % 8)))); // white
  _red_buffer[i] = (_red_buffer[i] & (0xFF ^ (1 << (7 - x % 8)))); // white
  if (color == GxEPD_WHITE) return;
  else if (color == GxEPD_BLACK) _black_buffer[i] = (_black_buffer[i] | (1 << (7 - x % 8)));
  else if (color == GxEPD_RED) _red_buffer[i] = (_red_buffer[i] | (1 << (7 - x % 8)));
}

bool GxEPD2_32_3C::mirror(bool m)
{
  swap (_mirror, m);
  return m;
}

void GxEPD2_32_3C::init()
{
  //  Serial.print(WIDTH); Serial.print("x"); Serial.print(HEIGHT);
  //  Serial.print(" : "); Serial.print(_pages); Serial.print(" pages of ");
  //  Serial.print(_page_height); Serial.println(" height");
  if (_cs >= 0)
  {
    digitalWrite(_cs, HIGH);
    pinMode(_cs, OUTPUT);
  }
  if (_dc >= 0)
  {
    digitalWrite(_dc, HIGH);
    pinMode(_dc, OUTPUT);
  }
  if (_rst >= 0)
  {
    digitalWrite(_rst, HIGH);
    pinMode(_rst, OUTPUT);
    delay(20);
    digitalWrite(_rst, LOW);
    delay(20);
    digitalWrite(_rst, HIGH);
    delay(200);
  }
  if (_busy >= 0)
  {
    pinMode(_busy, INPUT);
  }
  SPI.begin();
  SPI.setDataMode(SPI_MODE0);
  SPI.setBitOrder(MSBFIRST);
#if defined(SPI_HAS_TRANSACTION)
  // true also for STM32F1xx Boards
  SPISettings settings(4000000, MSBFIRST, SPI_MODE0);
  SPI.beginTransaction(settings);
  SPI.endTransaction();
  //Serial.println("SPI has Transaction");
#elif defined(ESP8266) || defined(ESP32)
  SPI.setFrequency(4000000);
#endif
  fillScreen(GxEPD_WHITE);
  _initial = true;
  _power_is_on = false;
  _current_page = -1;
}

void GxEPD2_32_3C::fillScreen(uint16_t color)
{
  uint8_t black = 0x00;
  uint8_t red = 0x00;
  if (color == GxEPD_WHITE);
  else if (color == GxEPD_BLACK) black = 0xFF;
  else if (color == GxEPD_RED) red = 0xFF;
  for (uint16_t x = 0; x < sizeof(_black_buffer); x++)
  {
    _black_buffer[x] = black;
    _red_buffer[x] = red;
  }
}

void GxEPD2_32_3C::setFullWindow()
{
  _using_partial_mode = false;
}

void GxEPD2_32_3C::setPartialWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
  if (_panel != GxEPD2::GDEW0154Z04)
  {
    _rotate(x, y, w, h);
    _using_partial_mode = true;
    _pw_x = gx_uint16_min(x, WIDTH);
    _pw_y = gx_uint16_min(y, HEIGHT);
    _pw_w = gx_uint16_min(w, WIDTH - _pw_x);
    _pw_h = gx_uint16_min(h, HEIGHT - _pw_y);
  }
  else
  {
    Serial.println("GDEW0154Z04 does not support partial update");
  }
}

void GxEPD2_32_3C::firstPage()
{
  _current_page = 0;
  _second_phase = false;
  if (!_using_partial_mode)
  {
    _Init_Full();
    _writeCommand(0x10);
  }
  else
  {
    if ((_pw_w > 0) && (_pw_h > 0))
    {
      switch (_panel)
      {
        case GxEPD2::GDEW0213Z16:
        case GxEPD2::GDEW029Z10:
        case GxEPD2::GDEW042Z15:
        case GxEPD2::GDEW075Z09:
          _Init_Part();
          _writeCommand(0x91); // partial in
          _setPartialRamArea(_pw_x, _pw_y, _pw_w, _pw_h);
          _writeCommand(0x10);
          break;
        case GxEPD2::GDEW027C44:
          _Init_Part();
          _setPartialRamArea27(0x14, _pw_x, _pw_y, _pw_w, _pw_h);
          break;
      }
    }
  }
}

bool GxEPD2_32_3C::nextPage()
{
  if (!_using_partial_mode)
  {
    switch (_panel)
    {
      case GxEPD2::GDEW0154Z04:
        return _nextPageFull154();
      case GxEPD2::GDEW0213Z16:
      case GxEPD2::GDEW029Z10:
      case GxEPD2::GDEW042Z15:
        return _nextPageFull();
      case GxEPD2::GDEW027C44:
        return _nextPageFull27();
      case GxEPD2::GDEW075Z09:
        return _nextPageFull75();
    }
  }
  else if ((_pw_w > 0) && (_pw_h > 0))
  {
    switch (_panel)
    {
      case GxEPD2::GDEW0213Z16:
      case GxEPD2::GDEW029Z10:
      case GxEPD2::GDEW042Z15:
        return _nextPagePart();
      case GxEPD2::GDEW027C44:
        return _nextPagePart27();
      case GxEPD2::GDEW075Z09:
        return _nextPagePart75();
    }
  }
  return false;
}

void GxEPD2_32_3C::powerOff(void)
{
  _PowerOff();
}

void GxEPD2_32_3C::drawInvertedBitmap(int16_t x, int16_t y, const uint8_t bitmap[], int16_t w, int16_t h, uint16_t color)
{
  // taken from Adafruit_GFX.cpp, modified
  int16_t byteWidth = (w + 7) / 8; // Bitmap scanline pad = whole byte
  uint8_t byte = 0;
  for (int16_t j = 0; j < h; j++)
  {
    for (int16_t i = 0; i < w; i++ )
    {
      if (i & 7) byte <<= 1;
      else
      {
#if defined(__AVR) || defined(ESP8266) || defined(ESP32)
        byte = pgm_read_byte(&bitmap[j * byteWidth + i / 8]);
#else
        byte = bitmap[j * byteWidth + i / 8];
#endif
      }
      if (!(byte & 0x80))
      {
        drawPixel(x + i, y + j, color);
      }
    }
  }
}

void GxEPD2_32_3C::clearScreen(uint8_t black_value, uint8_t red_value)
{
  switch (_panel)
  {
    case GxEPD2::GDEW0154Z04:
      _Init_Full();
      _writeCommand(0x10);
      for (int16_t i = 0; i < WIDTH * HEIGHT / 8; i++)
      {
        _writeData(bw2grey[(black_value & 0xF0) >> 4]);
        _writeData(bw2grey[black_value & 0x0F]);
      }
      _writeCommand(0x13);
      for (int16_t i = 0; i < WIDTH * HEIGHT / 8; i++)
      {
        _writeData(red_value);
      }
      _Update_Full();
      break;
    case GxEPD2::GDEW0213Z16:
    case GxEPD2::GDEW029Z10:
    case GxEPD2::GDEW042Z15:
      _Init_Part();
      _writeCommand(0x91); // partial in
      _setPartialRamArea(0, 0, WIDTH, HEIGHT);
      _writeCommand(0x10);
      for (int16_t i = 0; i < WIDTH * HEIGHT / 8; i++)
      {
        _writeData(black_value);
      }
      _writeCommand(0x13);
      for (int16_t i = 0; i < WIDTH * HEIGHT / 8; i++)
      {
        _writeData(red_value);
      }
      _Update_Part();
      _writeCommand(0x92); // partial out
      break;
    case GxEPD2::GDEW027C44:
      _Init_Part();
      _setPartialRamArea27(0x14, 0, 0, WIDTH, HEIGHT);
      for (int16_t i = 0; i < WIDTH * HEIGHT / 8; i++)
      {
        _writeData(~black_value);
      }
      _setPartialRamArea27(0x15, 0, 0, WIDTH, HEIGHT);
      for (int16_t i = 0; i < WIDTH * HEIGHT / 8; i++)
      {
        _writeData(~red_value);
      }
      _refreshWindow(0, 0, WIDTH, HEIGHT);
      _waitWhileBusy("clearScreen");
      break;
    case GxEPD2::GDEW075Z09:
      _Init_Part();
      _writeCommand(0x91); // partial in
      _setPartialRamArea(0, 0, WIDTH, HEIGHT);
      _writeCommand(0x10);
      for (int16_t i = 0; i < WIDTH * HEIGHT / 8; i++)
      {
        _send8pixel(~black_value, ~red_value);
      }
      _Update_Part();
      _writeCommand(0x92); // partial out
      break;
  }
}

void GxEPD2_32_3C::writeScreenBuffer(uint8_t black_value, uint8_t red_value)
{
  switch (_panel)
  {
    case GxEPD2::GDEW0154Z04:
      _Init_Full();
      _writeCommand(0x10);
      for (int16_t i = 0; i < WIDTH * HEIGHT / 8; i++)
      {
        _writeData(bw2grey[(black_value & 0xF0) >> 4]);
        _writeData(bw2grey[black_value & 0x0F]);
      }
      _writeCommand(0x13);
      for (int16_t i = 0; i < WIDTH * HEIGHT / 8; i++)
      {
        _writeData(red_value);
      }
      break;
    case GxEPD2::GDEW0213Z16:
    case GxEPD2::GDEW029Z10:
    case GxEPD2::GDEW042Z15:
      _Init_Part();
      _writeCommand(0x91); // partial in
      _setPartialRamArea(0, 0, WIDTH, HEIGHT);
      _writeCommand(0x10);
      for (int16_t i = 0; i < WIDTH * HEIGHT / 8; i++)
      {
        _writeData(black_value);
      }
      _writeCommand(0x13);
      for (int16_t i = 0; i < WIDTH * HEIGHT / 8; i++)
      {
        _writeData(red_value);
      }
      _writeCommand(0x92); // partial out
      break;
    case GxEPD2::GDEW027C44:
      _Init_Part();
      _setPartialRamArea27(0x14, 0, 0, WIDTH, HEIGHT);
      for (int16_t i = 0; i < WIDTH * HEIGHT / 8; i++)
      {
        _writeData(~black_value);
      }
      _setPartialRamArea27(0x15, 0, 0, WIDTH, HEIGHT);
      for (int16_t i = 0; i < WIDTH * HEIGHT / 8; i++)
      {
        _writeData(~red_value);
      }
      break;
    case GxEPD2::GDEW075Z09:
      _Init_Part();
      _writeCommand(0x91); // partial in
      _setPartialRamArea(0, 0, WIDTH, HEIGHT);
      _writeCommand(0x10);
      for (int16_t i = 0; i < WIDTH * HEIGHT / 8; i++)
      {
        _send8pixel(~black_value, ~red_value);
      }
      _writeCommand(0x92); // partial out
      break;
  }
}

void GxEPD2_32_3C::writeImage(const uint8_t bitmap[], int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  writeImage(bitmap, NULL, x, y, w, h, invert, mirror_y, pgm);
}

void GxEPD2_32_3C::writeImage(const uint8_t* black, const uint8_t* red, int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  x -= x % 8; // byte boundary
  w -= x % 8; // byte boundary
  int16_t x1 = x < 0 ? 0 : x; // limit
  int16_t y1 = y < 0 ? 0 : y; // limit
  int16_t w1 = x + w < WIDTH ? w : WIDTH - x; // limit
  int16_t h1 = y + h < HEIGHT ? h : HEIGHT - y; // limit
  int16_t dx = x1 - x;
  int16_t dy = y1 - y;
  w1 -= dx;
  h1 -= dy;
  if ((w1 <= 0) || (h1 <= 0)) return;
  switch (_panel)
  {
    case GxEPD2::GDEW0154Z04:
      _Init_Full();
      _writeCommand(0x10);
      for (int16_t i = 0; i < HEIGHT; i++)
      {
        for (int16_t j = 0; j < WIDTH; j += 8)
        {
          uint8_t data = 0xFF;
          if (black)
          {
            // use w, h of bitmap for index!
            //if ((i + dy < h) && (j + dx < w))
            if (((x + j) >= 0) && ((x + j) < w) && ((y + i) >= 0) && ((y + i) < h))
            {
              int16_t idx = mirror_y ? (x + j) / 8 + ((h - 1 - (y + i))) * (w / 8) : (x + j) / 8 + (y + i) * (w / 8);
              if (pgm)
              {
#if defined(__AVR) || defined(ESP8266) || defined(ESP32)
                data = pgm_read_byte(&black[idx]);
#else
                data = black[idx];
#endif
              }
              else
              {
                data = black[idx];
              }
              if (invert) data = ~data;
            }
          }
          //_writeData(data);
          _writeData(bw2grey[(data & 0xF0) >> 4]);
          _writeData(bw2grey[data & 0x0F]);
        }
      }
      _writeCommand(0x13);
      for (int16_t i = 0; i < HEIGHT; i++)
      {
        for (int16_t j = 0; j < WIDTH; j += 8)
        {
          uint8_t data = 0xFF;
          if (red)
          {
            // use w, h of bitmap for index!
            //if ((i + dy < h) && (j + dx < w))
            if (((x + j) >= 0) && ((x + j) < w) && ((y + i) >= 0) && ((y + i) < h))
            {
              int16_t idx = mirror_y ? (x + j) / 8 + ((h - 1 - (y + i))) * (w / 8) : (x + j) / 8 + (y + i) * (w / 8);
              if (pgm)
              {
#if defined(__AVR) || defined(ESP8266) || defined(ESP32)
                data = pgm_read_byte(&red[idx]);
#else
                data = red[idx];
#endif
              }
              else
              {
                data = red[idx];
              }
              if (invert) data = ~data;
            }
          }
          _writeData(data);
        }
      }
      return;
    case GxEPD2::GDEW075Z09:
      _Init_Part();
      _writeCommand(0x91); // partial in
      _setPartialRamArea(x1, y1, w1, h1);
      _writeCommand(0x10);
      for (int16_t i = 0; i < h1; i++)
      {
        for (int16_t j = 0; j < w1 / 8; j++)
        {
          uint8_t black_data = 0xFF;
          uint8_t red_data = 0xFF;
          if (black)
          {
            // use w, h of bitmap for index!
            int16_t idx = mirror_y ? j + dx / 8 + ((h - 1 - (i + dy))) * (w / 8) : j + dx / 8 + (i + dy) * (w / 8);
            if (pgm)
            {
#if defined(__AVR) || defined(ESP8266) || defined(ESP32)
              black_data = pgm_read_byte(&black[idx]);
#else
              black_data = black[idx];
#endif
            }
            else
            {
              black_data = black[idx];
            }
            if (invert) black_data = ~black_data;
          }
          if (red)
          {
            // use w, h of bitmap for index!
            int16_t idx = mirror_y ? j + dx / 8 + ((h - 1 - (i + dy))) * (w / 8) : j + dx / 8 + (i + dy) * (w / 8);
            if (pgm)
            {
#if defined(__AVR) || defined(ESP8266) || defined(ESP32)
              red_data = pgm_read_byte(&red[idx]);
#else
              red_data = red[idx];
#endif
            }
            else
            {
              red_data = red[idx];
            }
            if (invert) red_data = ~red_data;
          }
          _send8pixel(~black_data, ~red_data);
        }
      }
      _writeCommand(0x92); // partial out
      return;
    case GxEPD2::GDEW0213Z16:
    case GxEPD2::GDEW029Z10:
    case GxEPD2::GDEW042Z15:
      _Init_Part();
      _writeCommand(0x91); // partial in
      _setPartialRamArea(x1, y1, w1, h1);
      _writeCommand(0x10);
      break;
    case GxEPD2::GDEW027C44:
      _Init_Part();
      _setPartialRamArea27(0x14, x1, y1, w1, h1);
      break;
  }
  for (int16_t i = 0; i < h1; i++)
  {
    for (int16_t j = 0; j < w1 / 8; j++)
    {
      uint8_t data = 0xFF;
      if (black)
      {
        // use w, h of bitmap for index!
        int16_t idx = mirror_y ? j + dx / 8 + ((h - 1 - (i + dy))) * (w / 8) : j + dx / 8 + (i + dy) * (w / 8);
        if (pgm)
        {
#if defined(__AVR) || defined(ESP8266) || defined(ESP32)
          data = pgm_read_byte(&black[idx]);
#else
          data = black[idx];
#endif
        }
        else
        {
          data = black[idx];
        }
        if (invert) data = ~data;
      }
      if (_panel == GxEPD2::GDEW027C44) data = ~data;
      _writeData(data);
    }
  }
  switch (_panel)
  {
    case GxEPD2::GDEW0213Z16:
    case GxEPD2::GDEW029Z10:
    case GxEPD2::GDEW042Z15:
      _writeCommand(0x13);
      break;
    case GxEPD2::GDEW027C44:
      _setPartialRamArea27(0x15, x1, y1, w1, h1);
      break;
  }
  for (int16_t i = 0; i < h1; i++)
  {
    for (int16_t j = 0; j < w1 / 8; j++)
    {
      uint8_t data = 0xFF;
      if (red)
      {
        // use w, h of bitmap for index!
        int16_t idx = mirror_y ? j + dx / 8 + ((h - 1 - (i + dy))) * (w / 8) : j + dx / 8 + (i + dy) * (w / 8);
        if (pgm)
        {
#if defined(__AVR) || defined(ESP8266) || defined(ESP32)
          data = pgm_read_byte(&red[idx]);
#else
          data = red[idx];
#endif
        }
        else
        {
          data = red[idx];
        }
        if (invert) data = ~data;
      }
      if (_panel == GxEPD2::GDEW027C44) data = ~data;
      _writeData(data);
    }
  }
  if (_panel != GxEPD2::GDEW027C44)
  {
    _writeCommand(0x92); // partial out
  }
}

void GxEPD2_32_3C::drawImage(const uint8_t bitmap[], int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  writeImage(bitmap, x, y, w, h, invert, mirror_y, pgm);
  refresh(x, y, w, h);
}

void GxEPD2_32_3C::drawImage(const uint8_t* black, const uint8_t* red, int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  writeImage(black, red, x, y, w, h, invert, mirror_y, pgm);
  refresh(x, y, w, h);
}

void GxEPD2_32_3C::refresh(bool partial_update_mode)
{
  if (partial_update_mode) refresh(0, 0, WIDTH, HEIGHT);
  else _Update_Full();
}

void GxEPD2_32_3C::refresh(int16_t x, int16_t y, int16_t w, int16_t h)
{
  x -= x % 8; // byte boundary
  w -= x % 8; // byte boundary
  int16_t x1 = x < 0 ? 0 : x; // limit
  int16_t y1 = y < 0 ? 0 : y; // limit
  int16_t w1 = x + w < WIDTH ? w : WIDTH - x; // limit
  int16_t h1 = y + h < HEIGHT ? h : HEIGHT - y; // limit
  w1 -= x1 - x;
  h1 -= y1 - y;
  switch (_panel)
  {
    case GxEPD2::GDEW0154Z04:
      _Update_Full();
      break;
    case GxEPD2::GDEW0213Z16:
    case GxEPD2::GDEW029Z10:
    case GxEPD2::GDEW042Z15:
    case GxEPD2::GDEW075Z09:
      _Init_Part();
      _setPartialRamArea(x1, y1, w1, h1);
      _Update_Part();
      break;
    case GxEPD2::GDEW027C44:
      _refreshWindow(x1, y1, w1, h1);
      _waitWhileBusy("refresh");
      break;
  }
}

bool GxEPD2_32_3C::_nextPageFull()
{
  uint16_t page_ys = _current_page * _page_height;
  uint16_t bytes = (_current_page < (_pages - 1) ? _page_height : HEIGHT - page_ys) * _width_bytes;
  if (!_second_phase)
  {
    for (uint16_t idx = 0; idx < bytes; idx++)
    {
      uint8_t data = (idx < sizeof(_black_buffer)) ? _black_buffer[idx] : 0x00;
      _writeData(~data);
    }
    _current_page++;
    if (_current_page < _pages)
    {
      return true;
    }
    _second_phase = true;
    _current_page = 0;
    _writeCommand(0x13);
    return true;
  }
  for (uint16_t idx = 0; idx < bytes; idx++)
  {
    uint8_t data = (idx < sizeof(_red_buffer)) ? _red_buffer[idx] : 0x00;
    _writeData(~data);
  }
  _current_page++;
  if (_current_page < _pages)
  {
    fillScreen(GxEPD_WHITE);
    return true;
  }
  _Update_Full();
  _PowerOff();
  _current_page = -1;
  return false;
}

bool GxEPD2_32_3C::_nextPagePart()
{
  uint16_t page_ys = _current_page * _page_height;
  uint16_t page_ye = _current_page < (_pages - 1) ? page_ys + _page_height : HEIGHT;
  uint16_t dest_ys = gx_uint16_max(_pw_y, page_ys);
  uint16_t dest_ye = gx_uint16_min(_pw_y + _pw_h, page_ye);
  if (dest_ye > dest_ys)
  {
    uint16_t xs_d8 = _pw_x / 8;
    uint16_t xe_d8 = (_pw_x + _pw_w + 7) / 8;
    uint16_t ys = dest_ys - page_ys;
    uint16_t ye = dest_ye - page_ys;
    uint8_t* buffer = _second_phase ? _red_buffer : _black_buffer;
    for (uint16_t y = ys; y < ye; y++)
    {
      for (uint16_t x = xs_d8; x < xe_d8; x++)
      {
        uint16_t idx = x + y * _width_bytes;
        uint8_t data = (idx < sizeof(_black_buffer)) ? buffer[idx] : 0x00;
        _writeData(~data);
      }
    }
  }
  _current_page++;
  if (_current_page < _pages)
  {
    fillScreen(GxEPD_WHITE);
    return true;
  }
  if (!_second_phase)
  {
    fillScreen(GxEPD_WHITE);
    _second_phase = true;
    _current_page = 0;
    _writeCommand(0x13);
    return true;
  }
#ifdef USE_PARTIAL_UPDATE_WORKAROUND_ON_GDEW042Z15
  if (_panel == GxEPD2::GDEW042Z15)
  {
    _setPartialRamArea(0, 0, WIDTH, HEIGHT);
  }
#endif
  _Update_Part();
  _writeCommand(0x92); // partial out
  delay(200);
  _current_page = -1;
  return false;
}

bool GxEPD2_32_3C::_nextPageFull154()
{
  uint16_t page_ys = _current_page * _page_height;
  uint16_t bytes = (_current_page < (_pages - 1) ? _page_height : HEIGHT - page_ys) * _width_bytes;
  if (!_second_phase)
  {
    for (uint16_t idx = 0; idx < bytes; idx++)
    {
      uint8_t data = (idx < sizeof(_black_buffer)) ? ~_black_buffer[idx] : 0xFF;
      _writeData(bw2grey[(data & 0xF0) >> 4]);
      _writeData(bw2grey[data & 0x0F]);
    }
    _current_page++;
    if (_current_page < _pages)
    {
      fillScreen(GxEPD_WHITE);
      return true;
    }
    fillScreen(GxEPD_WHITE);
    _second_phase = true;
    _current_page = 0;
    _writeCommand(0x13);
    return true;
  }
  for (uint16_t idx = 0; idx < bytes; idx++)
  {
    _writeData((idx < sizeof(_red_buffer)) ? ~_red_buffer[idx] : 0xFF);
  }
  _current_page++;
  if (_current_page < _pages)
  {
    fillScreen(GxEPD_WHITE);
    return true;
  }
  _Update_Full();
  _PowerOff();
  _current_page = -1;
  return false;
}

bool GxEPD2_32_3C::_nextPageFull27()
{
  uint16_t page_ys = _current_page * _page_height;
  uint16_t bytes = (_current_page < (_pages - 1) ? _page_height : HEIGHT - page_ys) * _width_bytes;
  if (!_second_phase)
  {
    for (uint16_t idx = 0; idx < bytes; idx++)
    {
      uint8_t data = (idx < sizeof(_black_buffer)) ? _black_buffer[idx] : 0x00;
      _writeData(data);
    }
    _current_page++;
    if (_current_page < _pages)
    {
      fillScreen(GxEPD_WHITE);
      return true;
    }
    fillScreen(GxEPD_WHITE);
    _second_phase = true;
    _current_page = 0;
    _writeCommand(0x13);
    return true;
  }
  for (uint16_t idx = 0; idx < bytes; idx++)
  {
    uint8_t data = (idx < sizeof(_red_buffer)) ? _red_buffer[idx] : 0x00;
    _writeData(data);
  }
  _current_page++;
  if (_current_page < _pages)
  {
    fillScreen(GxEPD_WHITE);
    return true;
  }
  _Update_Full();
  _PowerOff();
  _current_page = -1;
  return false;
}

bool GxEPD2_32_3C::_nextPagePart27()
{
  uint16_t page_ys = _current_page * _page_height;
  uint16_t page_ye = _current_page < (_pages - 1) ? page_ys + _page_height : HEIGHT;
  uint16_t dest_ys = gx_uint16_max(_pw_y, page_ys);
  uint16_t dest_ye = gx_uint16_min(_pw_y + _pw_h, page_ye);
  if (dest_ye > dest_ys)
  {
    uint16_t xs_d8 = _pw_x / 8; // byte boundary
    uint16_t xe_d8 = ((_pw_x + _pw_w - 1) | 0x0007) / 8; // byte boundary inclusive (last byte)
    uint16_t ys = dest_ys - page_ys;
    uint16_t ye = dest_ye - page_ys; // exclusive
    uint8_t* buffer = _second_phase ? _red_buffer : _black_buffer;
    for (uint16_t y = ys; y < ye; y++)
    {
      for (uint16_t x = xs_d8; x <= xe_d8; x++)
      {
        uint16_t idx = x + y * _width_bytes;
        uint8_t data = (idx < sizeof(_black_buffer)) ? buffer[idx] : 0x00;
        _writeData(data);
      }
    }
  }
  _current_page++;
  if (_current_page < _pages)
  {
    fillScreen(GxEPD_WHITE);
    return true;
  }
  if (!_second_phase)
  {
    fillScreen(GxEPD_WHITE);
    _second_phase = true;
    _current_page = 0;
    _setPartialRamArea27(0x15, _pw_x, _pw_y, _pw_w, _pw_h);
    return true;
  }
  _refreshWindow(_pw_x, _pw_y, _pw_w, _pw_h);
  _waitWhileBusy("_nextPage27");
  delay(500); // don't stress this display
  _current_page = -1;
  return false;
}

bool GxEPD2_32_3C::_nextPageFull75()
{
  uint16_t page_ys = _current_page * _page_height;
  uint16_t bytes = (_current_page < (_pages - 1) ? _page_height : HEIGHT - page_ys) * _width_bytes;
  for (uint16_t idx = 0; idx < bytes; idx++)
  {
    _send8pixel(_black_buffer[idx], _red_buffer[idx]);
  }
  _current_page++;
  if (_current_page < _pages)
  {
    return true;
  }
  _Update_Full();
  delay(200);
  _PowerOff();
  _current_page = -1;
  return false;
}

bool GxEPD2_32_3C::_nextPagePart75()
{
  uint16_t page_ys = _current_page * _page_height;
  uint16_t page_ye = _current_page < (_pages - 1) ? page_ys + _page_height : HEIGHT;
  uint16_t dest_ys = gx_uint16_max(_pw_y, page_ys);
  uint16_t dest_ye = gx_uint16_min(_pw_y + _pw_h, page_ye);
  if (dest_ye > dest_ys)
  {
    uint16_t xe_d8 = ((_pw_x + _pw_w - 1) | 0x0007) / 8; // byte boundary inclusive (last byte)
    uint16_t xs_d8 = _pw_x / 8; // byte boundary
    uint16_t ys = dest_ys - page_ys;
    uint16_t ye = dest_ye - page_ys; // exclusive
    for (uint16_t y = ys; y < ye; y++)
    {
      for (uint16_t x = xs_d8; x <= xe_d8; x++)
      {
        uint16_t idx = x + y * _width_bytes;
        _send8pixel(_black_buffer[idx], _red_buffer[idx]);
      }
    }
  }
  _current_page++;
  if (_current_page < _pages)
  {
    fillScreen(GxEPD_WHITE);
    return true;
  }
  _Update_Part();
  _writeCommand(0x92); // partial out
  _current_page = -1;
  return false;
}

void GxEPD2_32_3C::_send8pixel(uint8_t black_data, uint8_t red_data)
{
  for (uint8_t j = 0; j < 8; j++)
  {
    uint8_t t = 0x00; // black
    if (black_data & 0x80); // keep black
    else if (red_data & 0x80) t = 0x04; //red
    else t = 0x03; // white
    t <<= 4;
    black_data <<= 1;
    red_data <<= 1;
    j++;
    if (black_data & 0x80); // keep black
    else if (red_data & 0x80) t |= 0x04; //red
    else t |= 0x03; // white
    black_data <<= 1;
    red_data <<= 1;
    _writeData(t);
  }
}

void GxEPD2_32_3C::_writeCommand(uint8_t c)
{
  if (_dc >= 0) digitalWrite(_dc, LOW);
  if (_cs >= 0) digitalWrite(_cs, LOW);
  SPI.transfer(c);
  if (_cs >= 0) digitalWrite(_cs, HIGH);
  if (_dc >= 0) digitalWrite(_dc, HIGH);
}

void GxEPD2_32_3C::_writeData(uint8_t d)
{
  if (_cs >= 0) digitalWrite(_cs, LOW);
  SPI.transfer(d);
  if (_cs >= 0) digitalWrite(_cs, HIGH);
}

void GxEPD2_32_3C::_writeData(const uint8_t* data, uint16_t n)
{
  if (_cs >= 0) digitalWrite(_cs, LOW);
  for (uint8_t i = 0; i < n; i++)
  {
    SPI.transfer(*data++);
  }
  if (_cs >= 0) digitalWrite(_cs, HIGH);
}

void GxEPD2_32_3C::_writeData_nCS(const uint8_t* data, uint16_t n)
{
  for (uint8_t i = 0; i < n; i++)
  {
    if (_cs >= 0) digitalWrite(_cs, LOW);
    SPI.transfer(*data++);
    if (_cs >= 0) digitalWrite(_cs, HIGH);
  }
}

void GxEPD2_32_3C::_waitWhileBusy(const char* comment)
{
  unsigned long start = micros();
  while (1)
  {
    if (digitalRead(_busy) != _busy_active_level) break;
    delay(1);
    if (micros() - start > (_panel == GxEPD2::GDEW075Z09 ? 40000000 : 20000000)) // > ? : >14.9s !
    {
      Serial.println("Busy Timeout!");
      break;
    }
  }
  if (comment)
  {
#if !defined(DISABLE_DIAGNOSTIC_OUTPUT)
    unsigned long elapsed = micros() - start;
    Serial.print(comment);
    Serial.print(" : ");
    Serial.println(elapsed);
#endif
  }
  (void) start;
}

void GxEPD2_32_3C::_setPartialRamArea(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
  uint16_t xe = (x + w - 1) | 0x0007; // byte boundary inclusive (last byte)
  uint16_t ye = y + h - 1;
  switch (_panel)
  {
    case GxEPD2::GDEW0213Z16:
    case GxEPD2::GDEW029Z10:
      x &= 0xFFF8; // byte boundary
      _writeCommand(0x90); // partial window
      //_writeData(x / 256);
      _writeData(x % 256);
      //_writeData(xe / 256);
      _writeData(xe % 256);
      _writeData(y / 256);
      _writeData(y % 256);
      _writeData(ye / 256);
      _writeData(ye % 256);
      _writeData(0x01); // don't see any difference
      //_writeData(0x00); // don't see any difference
      break;
    case GxEPD2::GDEW042Z15:
    case GxEPD2::GDEW075Z09:
      x &= 0xFFF8; // byte boundary
      xe |= 0x0007; // byte boundary
      _writeCommand(0x90); // partial window
      _writeData(x / 256);
      _writeData(x % 256);
      _writeData(xe / 256);
      _writeData(xe % 256);
      _writeData(y / 256);
      _writeData(y % 256);
      _writeData(ye / 256);
      _writeData(ye % 256);
      //_writeData(0x01); // distortion on full right half
      _writeData(0x00); // distortion on right half
      break;
  }
}

void GxEPD2_32_3C::_setPartialRamArea27(uint8_t command, uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
  w = (w + 7 + (x % 8)) & 0xfff8; // byte boundary exclusive (round up)
  _writeCommand(command);
  _writeData(x >> 8);
  _writeData(x & 0xf8);
  _writeData(y >> 8);
  _writeData(y & 0xff);
  _writeData(w >> 8);
  _writeData(w & 0xf8);
  _writeData(h >> 8);
  _writeData(h & 0xff);
}

void GxEPD2_32_3C::_refreshWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
  w = (w + 7 + (x % 8)) & 0xfff8; // byte boundary exclusive (round up)
  h = gx_uint16_min(h, 256); // strange controller error
  _writeCommand(0x16);
  _writeData(x >> 8);
  _writeData(x & 0xf8);
  _writeData(y >> 8);
  _writeData(y & 0xff);
  _writeData(w >> 8);
  _writeData(w & 0xf8);
  _writeData(h >> 8);
  _writeData(h & 0xff);
}

void GxEPD2_32_3C::_PowerOn(void)
{
  if (!_power_is_on)
  {
    _writeCommand(0x04);
    _waitWhileBusy("_PowerOn");
  }
  _power_is_on = true;
}

void GxEPD2_32_3C::_PowerOff(void)
{
  switch (_panel)
  {
    case GxEPD2::GDEW0154Z04:
      return;
      _writeCommand(0X50);
      _writeData(0x17);    //BD floating
      _writeCommand(0x82); //to solve Vcom drop
      _writeData(0x00);
      _writeCommand(0x01); //power setting
      _writeData(0x02);    //gate switch to external
      _writeData(0x00);
      _writeData(0x00);
      _writeData(0x00);
      delay(1500);         //delay 1.5S
      break;
  }
  _writeCommand(0x02); // power off
  _waitWhileBusy("_PowerOff");
  _power_is_on = false;
}

void GxEPD2_32_3C::_InitDisplay()
{
  // reset required for wakeup
  if (!_power_is_on && (_rst >= 0))
  {
    digitalWrite(_rst, 0);
    delay(10);
    digitalWrite(_rst, 1);
    delay(10);
  }
  switch (_panel)
  {
    case GxEPD2::GDEW0154Z04:
      _writeCommand(0x01);
      _writeData(0x07);
      _writeData(0x00);
      _writeData(0x08);
      _writeData(0x00);
      _writeCommand(0x06);
      _writeData(0x07);
      _writeData(0x07);
      _writeData(0x07);
      _writeCommand(0x04);
      // power on needed here!
      _waitWhileBusy("Power On");
      _writeCommand(0X00);
      _writeData(0xcf);
      _writeCommand(0X50);
      _writeData(0x37);
      _writeCommand(0x30);
      _writeData(0x39);
      _writeCommand(0x61);
      _writeData(0xC8);
      _writeData(0x00);
      _writeData(0xC8);
      _writeCommand(0x82);
      _writeData(0x0E);
      break;
    case GxEPD2::GDEW0213Z16:
      _writeCommand(0x06);
      _writeData (0x17);
      _writeData (0x17);
      _writeData (0x17);
      //_writeCommand(0x04);
      //_waitWhileBusy("_wakeUp Power On");
      _writeCommand(0X00);
      _writeData(0x8f);
      _writeCommand(0X50);
      _writeData(0x37); //VCOM AND DATA INTERVAL SETTING
      _writeCommand(0x61); //resolution setting
      _writeData (0x68);   //source 104
      _writeData (0x00);
      _writeData (0xd4);   //gate 212
      break;
    case GxEPD2::GDEW029Z10:
      _writeCommand(0x06);
      _writeData (0x17);
      _writeData (0x17);
      _writeData (0x17);
      //_writeCommand(0x04);
      //_waitWhileBusy("_wakeUp Power On");
      _writeCommand(0X00);
      _writeData(0x8f);
      _writeCommand(0X50);
      _writeData(0x77);
      _writeCommand(0x61);
      _writeData (0x80);
      _writeData (0x01);
      _writeData (0x28);
      break;
    case GxEPD2::GDEW027C44:
      _writeCommand(0x01);
      _writeData (0x03);
      _writeData (0x00);
      _writeData (0x2b);
      _writeData (0x2b);
      _writeData (0x09);
      _writeCommand(0x06);
      _writeData (0x07);
      _writeData (0x07);
      _writeData (0x17);
      _writeCommand(0xF8);
      _writeData (0x60);
      _writeData (0xA5);
      _writeCommand(0xF8);
      _writeData (0x89);
      _writeData (0xA5);
      _writeCommand(0xF8);
      _writeData (0x90);
      _writeData (0x00);
      _writeCommand(0xF8);
      _writeData (0x93);
      _writeData (0x2A);
      _writeCommand(0xF8);
      _writeData (0x73);
      _writeData (0x41);
      _writeCommand(0x16);
      _writeData(0x00);
      //_writeCommand(0x04);
      //_waitWhileBusy("_wakeUp Power On");
      _writeCommand(0x00);
      _writeData(0xaf); // by register LUT
      _writeCommand(0x30);
      _writeData (0x3a);
      _writeCommand(0x61);
      _writeData (0x00);
      _writeData (0xb0); //176
      _writeData (0x01);
      _writeData (0x08); //264
      _writeCommand(0x82);
      _writeData (0x12);
      _writeCommand(0X50);
      _writeData(0x87);
      break;
    case GxEPD2::GDEW042Z15:
      _writeCommand(0x06); //boost
      _writeData(0x17);
      _writeData(0x17);
      _writeData(0x17);
      //_writeData(0x04);
      //_waitWhileBusy("Power On");
      _writeCommand(0x00);
      _writeData(0x0f); // LUT from OTP Pixel with B/W/R.
      break;
    case GxEPD2::GDEW075Z09:
      /**********************************release flash sleep**********************************/
      _writeCommand(0X65);     //FLASH CONTROL
      _writeData(0x01);
      _writeCommand(0xAB);
      _writeCommand(0X65);     //FLASH CONTROL
      _writeData(0x00);
      /**********************************release flash sleep**********************************/
      _writeCommand(0x01);
      _writeData (0x37);       //POWER SETTING
      _writeData (0x00);
      _writeCommand(0x04);     //POWER ON
      _waitWhileBusy("POWER");
      _writeCommand(0X00);     //PANNEL SETTING
      _writeData(0xCF);
      _writeData(0x08);
      _writeCommand(0x06);     //boost
      _writeData (0xc7);
      _writeData (0xcc);
      _writeData (0x28);
      _writeCommand(0x30);     //PLL setting
      _writeData (0x3c);
      _writeCommand(0X41);     //TEMPERATURE SETTING
      _writeData(0x00);
      _writeCommand(0X50);     //VCOM AND DATA INTERVAL SETTING
      _writeData(0x77);
      _writeCommand(0X60);     //TCON SETTING
      _writeData(0x22);
      _writeCommand(0x61);     //tres 640*384
      _writeData (0x02);       //source 640
      _writeData (0x80);
      _writeData (0x01);       //gate 384
      _writeData (0x80);
      _writeCommand(0X82);     //VDCS SETTING
      _writeData(0x1E);        //decide by LUT file
      _writeCommand(0xe5);     //FLASH MODE
      _writeData(0x03);
      break;
  }
}

void GxEPD2_32_3C::_Init_Full()
{
  _InitDisplay();
  switch (_panel)
  {
    case GxEPD2::GDEW0154Z04:
      _writeCommand(0x20);
      _writeData(GxGDEW0154Z04_lut_20_vcom0, sizeof(GxGDEW0154Z04_lut_20_vcom0));
      _writeCommand(0x21);
      _writeData(GxGDEW0154Z04_lut_21_w, sizeof(GxGDEW0154Z04_lut_21_w));
      _writeCommand(0x22);
      _writeData(GxGDEW0154Z04_lut_22_b, sizeof(GxGDEW0154Z04_lut_22_b));
      _writeCommand(0x23);
      _writeData(GxGDEW0154Z04_lut_23_g1, sizeof(GxGDEW0154Z04_lut_23_g1));
      _writeCommand(0x24);
      _writeData(GxGDEW0154Z04_lut_24_g2, sizeof(GxGDEW0154Z04_lut_24_g2));
      _writeCommand(0x25);
      _writeData(GxGDEW0154Z04_lut_25_vcom1, sizeof(GxGDEW0154Z04_lut_25_vcom1));
      _writeCommand(0x26);
      _writeData(GxGDEW0154Z04_lut_26_red0, sizeof(GxGDEW0154Z04_lut_26_red0));
      _writeCommand(0x27);
      _writeData(GxGDEW0154Z04_lut_27_red1, sizeof(GxGDEW0154Z04_lut_27_red1));
      break;
    case GxEPD2::GDEW027C44:
      _writeCommand(0x20); //vcom
      _writeData_nCS(GxGDEW027C44_lut_20_vcomDC, sizeof(GxGDEW027C44_lut_20_vcomDC));
      _writeCommand(0x21); //ww --
      _writeData_nCS(GxGDEW027C44_lut_21, sizeof(GxGDEW027C44_lut_21));
      _writeCommand(0x22); //bw r
      _writeData_nCS(GxGDEW027C44_lut_22_red, sizeof(GxGDEW027C44_lut_22_red));
      _writeCommand(0x23); //wb w
      _writeData_nCS(GxGDEW027C44_lut_23_white, sizeof(GxGDEW027C44_lut_23_white));
      _writeCommand(0x24); //bb b
      _writeData_nCS(GxGDEW027C44_lut_24_black, sizeof(GxGDEW027C44_lut_24_black));
      break;
  }
  _PowerOn();
}

void GxEPD2_32_3C::_Init_Part()
{
  _InitDisplay();
  switch (_panel)
  {
    case GxEPD2::GDEW027C44:
      _writeCommand(0x20); //vcom
      _writeData_nCS(GxGDEW027C44_lut_20_vcomDC, sizeof(GxGDEW027C44_lut_20_vcomDC));
      _writeCommand(0x21); //ww --
      _writeData_nCS(GxGDEW027C44_lut_21, sizeof(GxGDEW027C44_lut_21));
      _writeCommand(0x22); //bw r
      _writeData_nCS(GxGDEW027C44_lut_22_red, sizeof(GxGDEW027C44_lut_22_red));
      _writeCommand(0x23); //wb w
      _writeData_nCS(GxGDEW027C44_lut_23_white, sizeof(GxGDEW027C44_lut_23_white));
      _writeCommand(0x24); //bb b
      _writeData_nCS(GxGDEW027C44_lut_24_black, sizeof(GxGDEW027C44_lut_24_black));
      break;
  }
  _PowerOn();
}

void GxEPD2_32_3C::_Update_Full(void)
{
  _writeCommand(0x12); //display refresh
  _waitWhileBusy("_Update_Full");
}

void GxEPD2_32_3C::_Update_Part(void)
{
  _writeCommand(0x12); //display refresh
  _waitWhileBusy("_Update_Part");
}

void GxEPD2_32_3C::_rotate(uint16_t& x, uint16_t& y, uint16_t& w, uint16_t& h)
{
  switch (getRotation())
  {
    case 1:
      swap(x, y);
      swap(w, h);
      x = WIDTH - x - w;
      break;
    case 2:
      x = WIDTH - x - w;
      y = HEIGHT - y - h;
      break;
    case 3:
      swap(x, y);
      swap(w, h);
      y = HEIGHT - y - h;
      break;
  }
}


