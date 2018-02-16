// Display Library for SPI e-paper panels from Dalian Good Display and boards from Waveshare.
// Requires HW SPI and Adafruit_GFX. Caution: these e-papers require 3.3V supply AND data lines!
//
// based on Demo Example from Good Display: http://www.good-display.com/download_list/downloadcategoryid=34&isMode=false.html
//
// Author: Jean-Marc Zingg
//
// Version: see library.properties
//
// Library: https://github.com/ZinggJM/GxEPD2_32

#include "GxEPD2_32_BW.h"
#include "WaveTables.h"

GxEPD2_32_BW::GxEPD2_32_BW(GxEPD_Panel panel, int8_t cs, int8_t dc, int8_t rst, int8_t busy) :
  Adafruit_GFX(GxEPD2_ScreenDimensions[panel].width, GxEPD2_ScreenDimensions[panel].height),
  _panel(panel), _cs(cs), _dc(dc), _rst(rst), _busy(busy),
  _current_page(-1), _using_partial_mode(false)
{
  _initial = true;
  _power_is_on = false;
  _width_bytes = uint16_t(WIDTH) / 8; // just discard any (WIDTH % 8) pixels
  _pixel_bytes = _width_bytes * uint16_t(HEIGHT); // save uint16_t range
  _page_height = buffer_size / _width_bytes;
  _pages = (HEIGHT / _page_height) + ((HEIGHT % _page_height) > 0);
  _ram_data_entry_mode  = (_panel == GDE0213B1) ? 0x01 : 0x03;
  _reverse = (_panel == GDE0213B1);
  _busy_active_level = (_panel < GDEW027W3) ? HIGH : LOW;
}

void GxEPD2_32_BW::drawPixel(int16_t x, int16_t y, uint16_t color)
{
  if ((x < 0) || (x >= width()) || (y < 0) || (y >= height())) return;

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
  if (_reverse)
  {
    // flip y for y-decrement mode
    y = HEIGHT - y - 1;
  }
  uint16_t i = x / 8 + y * _width_bytes;
  if (_current_page < 1)
  {
    if (i >= sizeof(_buffer)) return;
  }
  else
  {
    y -= _current_page * _page_height;
    if ((y < 0) || (y >= _page_height)) return;
    i = x / 8 + y * _width_bytes;
  }

  if (!color)
    _buffer[i] = (_buffer[i] | (1 << (7 - x % 8)));
  else
    _buffer[i] = (_buffer[i] & (0xFF ^ (1 << (7 - x % 8))));
}

void GxEPD2_32_BW::init()
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

void GxEPD2_32_BW::fillScreen(uint16_t color)
{
  uint8_t data = (color == GxEPD_BLACK) ? 0xFF : 0x00;
  for (uint16_t x = 0; x < sizeof(_buffer); x++)
  {
    _buffer[x] = data;
  }
}

void GxEPD2_32_BW::setFullWindow()
{
  _using_partial_mode = false;
}

void GxEPD2_32_BW::setPartialWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
  _rotate(x, y, w, h);
  _using_partial_mode = true;
  _pw_x = gx_uint16_min(x, WIDTH);
  _pw_y = gx_uint16_min(y, HEIGHT);
  _pw_w = gx_uint16_min(w, WIDTH - _pw_x);
  _pw_h = gx_uint16_min(h, HEIGHT - _pw_y);
}

void GxEPD2_32_BW::firstPage()
{
  _current_page = 0;
  _second_phase = false;
  if (!_using_partial_mode)
  {
    switch (_panel)
    {
      case GDEP015OC1:
      case GDE0213B1:
      case GDEH029A1:
        _Init_Full(_ram_data_entry_mode);
        _writeCommand(0x24);
        break;
      case GDEW027W3:
      case GDEW042T2:
        _Init_Full(_ram_data_entry_mode);
        _writeCommand(0x13);
        break;
      case GDEW075T8:
        _Init_Full(_ram_data_entry_mode);
        _writeCommand(0x10);
        break;
    }
  }
  else
  {
    if ((_pw_w > 0) && (_pw_h > 0))
    {
      switch (_panel)
      {
        case GDEP015OC1:
        case GDE0213B1:
        case GDEH029A1:
          _Init_Part(_ram_data_entry_mode);
          _setRamEntryWindow(_pw_x, _pw_y, _pw_w, _pw_h, _ram_data_entry_mode);
          break;
        case GDEW027W3:
          _Init_Part(_ram_data_entry_mode);
          _setPartialRamArea(_pw_x, _pw_y, _pw_w, _pw_h);
          break;
        case GDEW042T2:
          _Init_Part(_ram_data_entry_mode);
          _writeCommand(0x91); // partial in
          _setPartialRamArea(_pw_x, _pw_y, _pw_w, _pw_h);
          _writeCommand(0x13);
          break;
        case GDEW075T8:
          _Init_Part(_ram_data_entry_mode);
          _writeCommand(0x91); // partial in
          _setPartialRamArea(_pw_x, _pw_y, _pw_w, _pw_h);
          _writeCommand(0x10);
      }
    }
  }
}

bool GxEPD2_32_BW::nextPage()
{
  if (!_using_partial_mode)
  {
    switch (_panel)
    {
      case GDEP015OC1:
      case GDE0213B1:
      case GDEH029A1:
        return _nextPageFull();
      case GDEW027W3:
        return _nextPageFull27();
      case GDEW042T2:
        return _nextPageFull42();
      case GDEW075T8:
        return _nextPageFull75();
    }
  }
  else if ((_pw_w > 0) && (_pw_h > 0))
  {
    switch (_panel)
    {
      case GDEP015OC1:
      case GDE0213B1:
      case GDEH029A1:
        return _nextPagePart();
      case GDEW027W3:
        return _nextPagePart27();
      case GDEW042T2:
        return _nextPagePart42();
      case GDEW075T8:
        return _nextPagePart75();
    }
  }
  return false;
}

void GxEPD2_32_BW::powerOff(void)
{
  _PowerOff();
}

bool GxEPD2_32_BW::_nextPageFull()
{
  uint16_t page_ys = _current_page * _page_height;
  uint16_t bytes = (_current_page < (_pages - 1) ? _page_height : HEIGHT - page_ys) * _width_bytes;
  for (uint16_t idx = 0; idx < bytes; idx++)
  {
    uint8_t data = (idx < sizeof(_buffer)) ? _buffer[idx] : 0x00;
    _writeData(~data);
  }
  _current_page++;
  if (_current_page < _pages)
  {
    return true;
  }
  if (!_second_phase)
  {
    _Update_Full();
    delay(200);
    // second phase needed for subsequent partial updates
    _second_phase = true;
    _current_page = 0;
    _Init_Part(_ram_data_entry_mode);
    _writeCommand(0x24);
    return true;
  }
  _PowerOff();
  _current_page = -1;
  return false;
}

bool GxEPD2_32_BW::_nextPagePart()
{
  uint16_t page_ys = _current_page * _page_height;
  uint16_t page_ye = _current_page < (_pages - 1) ? page_ys + _page_height : HEIGHT;
  uint16_t dest_ys = gx_uint16_max(_pw_y, page_ys);
  uint16_t dest_ye = gx_uint16_min(_pw_y + _pw_h, page_ye);
  if (_reverse)
  {
    dest_ys = gx_uint16_max(HEIGHT - _pw_y - _pw_h, page_ys);
    dest_ye = gx_uint16_min(HEIGHT - _pw_y, page_ye);
  }
  if (dest_ye > dest_ys)
  {
    uint16_t xs_d8 = _pw_x / 8;
    uint16_t xe_d8 = (_pw_x + _pw_w + 7) / 8;
    uint16_t ys = dest_ys - page_ys;
    uint16_t ye = dest_ye - page_ys;
    for (uint16_t y = ys; y < ye; y++)
    {
      for (uint16_t x = xs_d8; x < xe_d8; x++)
      {
        uint16_t idx = x + y * _width_bytes;
        uint8_t data = (idx < sizeof(_buffer)) ? _buffer[idx] : 0x00;
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
    _Update_Part();
    delay(200);
    _second_phase = true;
    _current_page = 0;
    _setRamEntryWindow(_pw_x, _pw_y, _pw_w, _pw_h, _ram_data_entry_mode); // needed!
    return true;
  }
  delay(200);
  //_PowerOff();
  _current_page = -1;
  return false;
}

bool GxEPD2_32_BW::_nextPageFull27()
{
  uint16_t page_ys = _current_page * _page_height;
  uint16_t bytes = (_current_page < (_pages - 1) ? _page_height : HEIGHT - page_ys) * _width_bytes;
  for (uint16_t idx = 0; idx < bytes; idx++)
  {
    uint8_t data = (idx < sizeof(_buffer)) ? _buffer[idx] : 0x00;
    _writeData(~data);
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

bool GxEPD2_32_BW::_nextPagePart27()
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
    for (uint16_t y = ys; y < ye; y++)
    {
      for (uint16_t x = xs_d8; x <= xe_d8; x++)
      {
        uint16_t idx = x + y * _width_bytes;
        uint8_t data = (idx < sizeof(_buffer)) ? _buffer[idx] : 0x00;
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
  _refreshWindow(_pw_x, _pw_y, _pw_w, _pw_h);
  _waitWhileBusy("_nextPage27");
  delay(500); // don't stress this display
  //_PowerOff();
  _current_page = -1;
  return false;
}

bool GxEPD2_32_BW::_nextPageFull42()
{
  uint16_t page_ys = _current_page * _page_height;
  uint16_t bytes = (_current_page < (_pages - 1) ? _page_height : HEIGHT - page_ys) * _width_bytes;
  for (uint16_t idx = 0; idx < bytes; idx++)
  {
    uint8_t data = (idx < sizeof(_buffer)) ? _buffer[idx] : 0x00;
    _writeData(~data);
  }
  _current_page++;
  if (_current_page < _pages)
  {
    return true;
  }
  if (_initial)
  {
    _Update_Full();
    delay(200);
    _initial = false;
    _current_page = 0;
    _writeCommand(0x13);
    return true;
  }
  _Update_Full();
  delay(200);
  _PowerOff();
  _current_page = -1;
  return false;
}

bool GxEPD2_32_BW::_nextPagePart42()
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
        uint8_t data = (idx < sizeof(_buffer)) ? _buffer[idx] : 0x00;
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
  _Update_Part();
  _writeCommand(0x92); // partial out
  _current_page = -1;
  return false;
}

bool GxEPD2_32_BW::_nextPageFull75()
{
  uint16_t page_ys = _current_page * _page_height;
  uint16_t bytes = (_current_page < (_pages - 1) ? _page_height : HEIGHT - page_ys) * _width_bytes;
  for (uint16_t idx = 0; idx < bytes; idx++)
  {
    uint8_t data = (idx < sizeof(_buffer)) ? _buffer[idx] : 0x00;
    _send8pixel(data);
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

bool GxEPD2_32_BW::_nextPagePart75()
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
        uint8_t data = (idx < sizeof(_buffer)) ? _buffer[idx] : 0x00;
        _send8pixel(data);
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

void GxEPD2_32_BW::_send8pixel(uint8_t data)
{
  for (uint8_t j = 0; j < 8; j++)
  {
    uint8_t t = data & 0x80 ? 0x00 : 0x03;
    t <<= 4;
    data <<= 1;
    j++;
    t |= data & 0x80 ? 0x00 : 0x03;
    data <<= 1;
    _writeData(t);
  }
}

void GxEPD2_32_BW::_writeCommand(uint8_t c)
{
  if (_dc >= 0) digitalWrite(_dc, LOW);
  if (_cs >= 0) digitalWrite(_cs, LOW);
  SPI.transfer(c);
  if (_cs >= 0) digitalWrite(_cs, HIGH);
  if (_dc >= 0) digitalWrite(_dc, HIGH);
}

void GxEPD2_32_BW::_writeData(uint8_t d)
{
  if (_cs >= 0) digitalWrite(_cs, LOW);
  SPI.transfer(d);
  if (_cs >= 0) digitalWrite(_cs, HIGH);
}

void GxEPD2_32_BW::_writeData(const uint8_t* data, uint16_t n)
{
  if (_cs >= 0) digitalWrite(_cs, LOW);
  for (uint8_t i = 0; i < n; i++)
  {
    SPI.transfer(*data++);
  }
  if (_cs >= 0) digitalWrite(_cs, HIGH);
}

void GxEPD2_32_BW::_writeCommandData(const uint8_t* pCommandData, uint8_t datalen)
{
  if (_dc >= 0) digitalWrite(_dc, LOW);
  if (_cs >= 0) digitalWrite(_cs, LOW);
  SPI.transfer(*pCommandData++);
  if (_dc >= 0) digitalWrite(_dc, HIGH);
  for (uint8_t i = 0; i < datalen - 1; i++)  // sub the command
  {
    SPI.transfer(*pCommandData++);
  }
  if (_cs >= 0) digitalWrite(_cs, HIGH);
}

void GxEPD2_32_BW::_waitWhileBusy(const char* comment)
{
  unsigned long start = micros();
  while (1)
  {
    if (digitalRead(_busy) != _busy_active_level) break;
    delay(1);
    if (micros() - start > 10000000)
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

void GxEPD2_32_BW::_setRamEntryWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint8_t em)
{
  em = gx_uint16_min(em, 0x03);
  _writeCommand(0x11);
  _writeData(em);
  switch (em)
  {
    case 0x00: // x decrease, y decrease
      _setRamArea((x + w - 1), x, (y + h - 1), y);
      _setRamPointer((x + w - 1), (y + h - 1));
      break;
    case 0x01: // x increase, y decrease : as in demo code
      _setRamArea(x, (x + w - 1), (y + h - 1), y);
      _setRamPointer(x, (y + h - 1));
      break;
    case 0x02: // x decrease, y increase
      _setRamArea((x + w - 1), x, y, (y + h - 1));
      _setRamPointer((x + w - 1), y);
      break;
    case 0x03: // x increase, y increase : normal mode
      _setRamArea(x, (x + w - 1), y, (y + h - 1));
      _setRamPointer(x, y);
      break;
  }
  _writeCommand(0x24);
}

void GxEPD2_32_BW::_setRamArea(uint16_t xs, uint16_t xe, uint16_t ys, uint16_t ye)
{
  _writeCommand(0x44);
  _writeData(xs / 8);
  _writeData(xe / 8);
  _writeCommand(0x45);
  _writeData(ys % 256);
  _writeData(ys / 256);
  _writeData(ye % 256);
  _writeData(ye / 256);
}

void GxEPD2_32_BW::_setRamPointer(uint16_t x, uint16_t y)
{
  _writeCommand(0x4e);
  _writeData(x / 8);
  _writeCommand(0x4f);
  _writeData(y % 256);
  _writeData(y / 256);
}

void GxEPD2_32_BW::_setPartialRamArea(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
  if (_panel == GDEW027W3)
  {
    w = (w + 7 + (x % 8)) & 0xfff8; // byte boundary exclusive (round up)
    _writeCommand(0x15);
    _writeData(x >> 8);
    _writeData(x & 0xf8);
    _writeData(y >> 8);
    _writeData(y & 0xff);
    _writeData(w >> 8);
    _writeData(w & 0xf8);
    _writeData(h >> 8);
    _writeData(h & 0xff);
  }
  else
  {
    uint16_t xe = (x + w - 1) | 0x0007; // byte boundary inclusive (last byte)
    x &= 0xFFF8; // byte boundary
    uint16_t ye = y + h - 1;
    _writeCommand(0x90); // partial window
    _writeData(x / 256);
    _writeData(x % 256);
    _writeData(xe / 256);
    _writeData(xe % 256);
    _writeData(y / 256);
    _writeData(y % 256);
    _writeData(ye / 256);
    _writeData(ye % 256);
    _writeData(0x01); // don't see any difference
    //_writeData(0x00); // don't see any difference
  }
}

void GxEPD2_32_BW::_refreshWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
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

void GxEPD2_32_BW::_PowerOn(void)
{
  if (!_power_is_on)
  {
    if (_panel < GDEW027W3)
    {
      _writeCommand(0x22);
      _writeData(0xc0);
      _writeCommand(0x20);
    }
    else
    {
      _writeCommand(0x04);
    }
    _waitWhileBusy("_PowerOn");
  }
  _power_is_on = true;
}

void GxEPD2_32_BW::_PowerOff(void)
{
  if (_panel < GDEW027W3)
  {
    _writeCommand(0x22);
    _writeData(0xc3);
    _writeCommand(0x20);
  }
  else
  {
    _writeCommand(0x02); // power off
  }
  _waitWhileBusy("_PowerOff");
  _power_is_on = false;
}

void GxEPD2_32_BW::_InitDisplay(uint8_t em)
{
  switch (_panel)
  {
    case GDEP015OC1:
      _writeCommand(0x01); // Panel configuration, Gate selection
      _writeData((HEIGHT - 1) % 256);
      _writeData((HEIGHT - 1) / 256);
      _writeData(0x00);
      _writeCommand(0x0c); // softstart
      _writeData(0xd7);
      _writeData(0xd6);
      _writeData(0x9d);
      _writeCommand(0x2c); // VCOM setting
      _writeData(0x9b);
      _writeCommand(0x3a); // DummyLine
      _writeData(0x1a);    // 4 dummy line per gate
      _writeCommand(0x3b); // Gatetime
      _writeData(0x08);    // 2us per line
      _setRamEntryWindow(0, 0, WIDTH, HEIGHT, em);
      break;
    case GDE0213B1:
      _writeCommand(0x01); // Panel configuration, Gate selection
      _writeData((HEIGHT - 1) % 256);
      _writeData((HEIGHT - 1) / 256);
      _writeData(0x00);
      _writeCommand(0x0c); // softstart
      _writeData(0xd7);
      _writeData(0xd6);
      _writeData(0x9d);
      _writeCommand(0x2c); // VCOM setting
      _writeData(0xa8);    // * different
      _writeCommand(0x3a); // DummyLine
      _writeData(0x1a);    // 4 dummy line per gate
      _writeCommand(0x3b); // Gatetime
      _writeData(0x08);    // 2us per line
      _setRamEntryWindow(0, 0, WIDTH, HEIGHT, em);
      break;
    case GDEH029A1:
      _writeCommand(0x01); // Panel configuration, Gate selection
      _writeData((HEIGHT - 1) % 256);
      _writeData((HEIGHT - 1) / 256);
      _writeData(0x00);
      _writeCommand(0x0c); // softstart
      _writeData(0xd7);
      _writeData(0xd6);
      _writeData(0x9d);
      _writeCommand(0x2c); // VCOM setting
      _writeData(0xa8);    // * different
      _writeCommand(0x3a); // DummyLine
      _writeData(0x1a);    // 4 dummy line per gate
      _writeCommand(0x3b); // Gatetime
      _writeData(0x08);    // 2us per line
      _setRamEntryWindow(0, 0, WIDTH, HEIGHT, em);
      break;
    case GDEW027W3:
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
      _writeData (0xa0);
      _writeData (0xa5);
      _writeCommand(0xF8);
      _writeData (0xa1);
      _writeData (0x00);
      _writeCommand(0xF8);
      _writeData (0x73);
      _writeData (0x41);
      _writeCommand(0x16);
      _writeData(0x00);
      //_writeCommand(0x04);
      //_waitWhileBusy("_wakeUp Power On");
      _writeCommand(0x00);
      _writeData(0x9f); // b/w, by OTP LUT
      _writeCommand(0x30);
      _writeData (0x3a); //3A 100HZ
      _writeCommand(0x61);
      _writeData (0x00);
      _writeData (0xb0); //176
      _writeData (0x01);
      _writeData (0x08); //264
      _writeCommand(0x82);
      _writeData (0x12);
      break;
    case GDEW042T2:
      _writeCommand(0x06); // boost
      _writeData(0x17);
      _writeData(0x17);
      _writeData(0x17);
      //_writeCommand(0x04);
      //_waitWhileBusy("Power On");
      _writeCommand(0x00);
      _writeData(0x1f); // LUT from OTP Pixel with B/W.
      break;
    case GDEW075T8:
      if (!_power_is_on && (_rst >= 0))
      {
        digitalWrite(_rst, 0);
        delay(10);
        digitalWrite(_rst, 1);
        delay(10);
      }
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
      //_writeCommand(0x04);     //POWER ON
      //_waitWhileBusy();
      break;
  }
}

void GxEPD2_32_BW::_Init_Full(uint8_t em)
{
  _InitDisplay(em);
  switch (_panel)
  {
    case GDEP015OC1:
      _writeCommandData(GDEP015OC1_LUTDefault_full, sizeof(GDEP015OC1_LUTDefault_full));
      break;
    case GDE0213B1:
      _writeCommandData(GxGDE0213B1_LUTDefault_full, sizeof(GxGDE0213B1_LUTDefault_full));
      break;
    case GDEH029A1:
      _writeCommandData(GxGDEH029A1_LUTDefault_full, sizeof(GxGDEH029A1_LUTDefault_full));
      break;
    case GDEW027W3:
      _writeCommand(0x20);
      _writeData(GxGDEW027W3_lut_20_vcomDC, sizeof(GxGDEW027W3_lut_20_vcomDC));
      _writeCommand(0x21);
      _writeData(GxGDEW027W3_lut_21_ww, sizeof(GxGDEW027W3_lut_21_ww));
      _writeCommand(0x22);
      _writeData(GxGDEW027W3_lut_22_bw, sizeof(GxGDEW027W3_lut_22_bw));
      _writeCommand(0x23);
      _writeData(GxGDEW027W3_lut_23_wb, sizeof(GxGDEW027W3_lut_23_wb));
      _writeCommand(0x24);
      _writeData(GxGDEW027W3_lut_24_bb, sizeof(GxGDEW027W3_lut_24_bb));
      break;
  }
  _PowerOn();
}

void GxEPD2_32_BW::_Init_Part(uint8_t em)
{
  _InitDisplay(em);
  switch (_panel)
  {
    case GDEP015OC1:
      _writeCommandData(GDEP015OC1_LUTDefault_part, sizeof(GDEP015OC1_LUTDefault_part));
      break;
    case GDE0213B1:
      _writeCommandData(GxGDE0213B1_LUTDefault_part, sizeof(GxGDE0213B1_LUTDefault_part));
      break;
    case GDEH029A1:
      _writeCommandData(GxGDEH029A1_LUTDefault_part, sizeof(GxGDEH029A1_LUTDefault_part));
      break;
    case GDEW027W3:
      // no partial update LUT
      _writeCommand(0x20);
      _writeData(GxGDEW027W3_lut_20_vcomDC, sizeof(GxGDEW027W3_lut_20_vcomDC));
      _writeCommand(0x21);
      _writeData(GxGDEW027W3_lut_21_ww, sizeof(GxGDEW027W3_lut_21_ww));
      _writeCommand(0x22);
      _writeData(GxGDEW027W3_lut_22_bw, sizeof(GxGDEW027W3_lut_22_bw));
      _writeCommand(0x23);
      _writeData(GxGDEW027W3_lut_23_wb, sizeof(GxGDEW027W3_lut_23_wb));
      _writeCommand(0x24);
      _writeData(GxGDEW027W3_lut_24_bb, sizeof(GxGDEW027W3_lut_24_bb));
      break;
  }
  _PowerOn();
}

void GxEPD2_32_BW::_Update_Full(void)
{
  if (_panel < GDEW027W3)
  {
    _writeCommand(0x22);
    _writeData(0xc4);
    _writeCommand(0x20);
    _waitWhileBusy("_Update_Full");
    _writeCommand(0xff);
  }
  else
  {
    _writeCommand(0x12);      //display refresh
    _waitWhileBusy("_Update_Full");
  }
}

void GxEPD2_32_BW::_Update_Part(void)
{
  if (_panel < GDEW027W3)
  {
    _writeCommand(0x22);
    _writeData(0x04);
    _writeCommand(0x20);
    _waitWhileBusy("_Update_Part");
    _writeCommand(0xff);
  }
  else
  {
    _writeCommand(0x12);      //display refresh
    _waitWhileBusy("_Update_Part");
  }
}

void GxEPD2_32_BW::_rotate(uint16_t& x, uint16_t& y, uint16_t& w, uint16_t& h)
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

