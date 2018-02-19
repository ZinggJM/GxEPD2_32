// Display Library for SPI e-paper panels from Dalian Good Display and boards from Waveshare.
// Requires HW SPI and Adafruit_GFX. Caution: these e-papers require 3.3V supply AND data lines!
//
// based on Demo Example from Good Display: http://www.good-display.com/download_list/downloadcategoryid=34&isMode=false.html
//
// Author: Jean-Marc Zingg
//
// Version: see library.properties
//
// Library: https://github.com/ZinggJM/GxEPD_AVR

// NOTE: FOR EXPERIMENTAL TEST WITH 7.5 3-COLOR E-PAPER, untested, as I do not have this disply

#ifndef _GxEPD2_32_3C_X_H_
#define _GxEPD2_32_3C_X_H_

#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>

#ifndef GxEPD_BLACK
#define GxEPD_BLACK     0x0000
#define GxEPD_DARKGREY  0x7BEF      /* 128, 128, 128 */
#define GxEPD_LIGHTGREY 0xC618      /* 192, 192, 192 */
#define GxEPD_WHITE     0xFFFF
#define GxEPD_RED       0xF800      /* 255,   0,   0 */
#endif

struct GxEPD2_32_3C_X_ScreenDimensionType
{
  uint16_t width;
  uint16_t height;
};

static const GxEPD2_32_3C_X_ScreenDimensionType GxEPD2_32_3C_X_ScreenDimensions[]
{
  // note: width must be multiple of 8
  {200, 200}, // GDEW0154Z04
  {104, 212}, // GDEW0213Z16
  {128, 296}, // GDEW029Z10
  {176, 264}, // GDEW027C44
  {400, 300}, // GDEW042Z15
  {640, 384}  // GDEW075Z09
};

class GxEPD2_32_3C_X : public Adafruit_GFX
{
  private:
    // 2 * ~5k full screen buffer for GDEW042Z15 is optimal (black/white + color/white)
    static const uint16_t buffer_size = 400 * 300 / 8; // 2 * 5'000 bytes
    // 2 * ~2.5k half screen buffer for GDEW042Z15 is a good compromise
    // static const uint16_t buffer_size = 400 * 300 / 8 / 2; // 2 * 2'500 bytes
  public:
    enum GxEPD_Panel
    {
      GDEW0154Z04, Waveshare_1_54_bwr = GDEW0154Z04,
      GDEW0213Z16, Waveshare_2_13_bwr = GDEW0213Z16,
      GDEW029Z10,  Waveshare_2_9_bwr = GDEW029Z10,
      GDEW027C44,  Waveshare_2_7_bwr = GDEW027C44,
      GDEW042Z15,  Waveshare_4_2_bwr = GDEW042Z15,
      GDEW075Z09,  Waveshare_7_5_bwr = GDEW075Z09
    };
    GxEPD2_32_3C_X(GxEPD_Panel panel, int8_t cs, int8_t dc, int8_t rst, int8_t busy);
    void drawPixel(int16_t x, int16_t y, uint16_t color);
    GxEPD_Panel panel()
    {
      return _panel;
    }
    bool hasColor()
    {
      return true;
    };
    bool hasPartialUpdate()
    {
      return (_panel != GDEW0154Z04);
    };
    bool hasFastPartialUpdate()
    {
      return false;
    }
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
    bool _nextPageFull154();
    bool _nextPageFull27();
    bool _nextPagePart27();
    bool _nextPageFull75();
    bool _nextPagePart75();
    void _send8pixel(uint8_t black_data, uint8_t red_data);
    void _writeCommand(uint8_t c);
    void _writeData(uint8_t d);
    void _writeData(const uint8_t* data, uint16_t n);
    void _writeData_nCS(const uint8_t* data, uint16_t n);
    void _setPartialRamArea(uint16_t x, uint16_t y, uint16_t w, uint16_t h);
    void _setPartialRamArea27(uint8_t command, uint16_t x, uint16_t y, uint16_t w, uint16_t h);
    void _setRamEntryPartialWindow(uint8_t em);
    void _refreshWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h);
    void _PowerOn(void);
    void _PowerOff(void);
    void _waitWhileBusy(const char* comment = 0);
    void _InitDisplay();
    void _Init_Full();
    void _Init_Part();
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
    uint8_t _busy_active_level;
    uint16_t _width_bytes, _pixel_bytes;
    int16_t _current_page;
    uint16_t _pages, _page_height;
    bool _initial, _power_is_on, _using_partial_mode, _second_phase;
    uint16_t _pw_x, _pw_y, _pw_w, _pw_h;
    uint8_t _black_buffer[buffer_size];
    uint8_t _red_buffer[buffer_size];
    static const uint8_t bw2grey[];
};

#endif
