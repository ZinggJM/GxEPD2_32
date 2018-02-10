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

#ifndef _GxEPD2_32_BW_H_
#define _GxEPD2_32_BW_H_

#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>

#define GxEPD_BLACK     0x0000
#define GxEPD_DARKGREY  0x7BEF      /* 128, 128, 128 */
#define GxEPD_LIGHTGREY 0xC618      /* 192, 192, 192 */
#define GxEPD_WHITE     0xFFFF
#define GxEPD_RED       0xF800      /* 255,   0,   0 */

struct GxEPD2_ScreenDimensionType
{
  uint16_t width;
  uint16_t height;
};

static const GxEPD2_ScreenDimensionType GxEPD2_ScreenDimensions[]
{
  // note: width must be multiple of 8
  {200, 200}, // GDEP015OC1
  {128, 250}, // GDE0213B1
  {128, 296}, // GDEH029A1
  {176, 264}, // GDEW027W3
  {400, 300}, // GDEW042T2
  {640, 384}  // GDEW075T8
};

class GxEPD2_32_BW : public Adafruit_GFX
{
  private:
    // ~5k full screen buffer for GDEW042T2 is a good compromise
    static const uint16_t buffer_size = 400 * 300 / 8; // 5'000 bytes
    // 30k full screen buffer for GDEW075T8 will nearly fill ESP8266
    //static const uint16_t buffer_size = 640 * 384 / 8; // 30'720 bytes
  public:
    enum GxEPD_Panel
    {
      GDEP015OC1, WS_1_54_bw = GDEP015OC1,
      GDE0213B1,  WS_2_13_bw = GDE0213B1,
      GDEH029A1,  WS_2_9_bw = GDEH029A1,
      GDEW027W3,  WS_2_7_bw = GDEW027W3,
      GDEW042T2,  WS_4_2_bw = GDEW042T2,
      GDEW075T8,  WS_7_5_bw = GDEW075T8
    };
    GxEPD2_32_BW(GxEPD_Panel panel, int8_t cs, int8_t dc, int8_t rst, int8_t busy);
    void drawPixel(int16_t x, int16_t y, uint16_t color);
    GxEPD_Panel panel();
    void init();
    void fillScreen(uint16_t color); // 0x0 black, >0x0 white, to buffer
    void setFullWindow();
    void setPartialWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h);
    void firstPage();
    bool nextPage();
    // partial update keeps power on
    void powerOff(void);
  private:
    template <typename T> static inline void
    swap(T& a, T& b)
    {
      T t = a;
      a = b;
      b = t;
    }
    bool _nextPageFull();
    bool _nextPagePart();
    bool _nextPageFull27();
    bool _nextPagePart27();
    bool _nextPageFull42();
    bool _nextPagePart42();
    bool _nextPageFull75();
    bool _nextPagePart75();
    void _send8pixel(uint8_t data);
    void _writeCommand(uint8_t c);
    void _writeData(uint8_t d);
    void _writeData(const uint8_t* data, uint16_t n);
    void _writeCommandData(const uint8_t* pCommandData, uint8_t datalen);
    void _setRamEntryWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint8_t em);
    void _setRamArea(uint16_t xs, uint16_t xe, uint16_t ys, uint16_t ye);
    void _setPartialRamArea(uint16_t x, uint16_t y, uint16_t w, uint16_t h);
    void _setRamPointer(uint16_t x, uint16_t y);
    void _setRamEntryPartialWindow(uint8_t em);
    void _refreshWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h);
    void _PowerOn(void);
    void _PowerOff(void);
    void _waitWhileBusy(const char* comment = 0);
    void _InitDisplay(uint8_t em);
    void _Init_Full(uint8_t em);
    void _Init_Part(uint8_t em);
    void _Update_Full(void);
    void _Update_Part(void);
    void _rotate(uint16_t& x, uint16_t& y, uint16_t& w, uint16_t& h);
    static inline uint16_t gx_uint16_min(uint16_t a, uint16_t b)
    {
      return (a < b ? a : b);
    };
    static inline uint16_t gx_uint16_max(uint16_t a, uint16_t b)
    {
      return (a > b ? a : b);
    };
  protected:
    GxEPD_Panel _panel;
    int8_t _cs, _dc, _rst, _busy;
    uint8_t _ram_data_entry_mode, _busy_active_level;
    uint16_t _width_bytes, _pixel_bytes;
    int16_t _current_page;
    uint16_t _pages, _page_height;
    bool _initial, _power_is_on, _using_partial_mode, _second_phase, _reverse;
    uint16_t _pw_x, _pw_y, _pw_w, _pw_h;
    uint8_t _buffer[buffer_size];
};

#endif
