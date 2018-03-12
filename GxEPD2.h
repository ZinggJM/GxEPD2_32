#ifndef _GxEPD2_H_
#define _GxEPD2_H_

#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>

#define GxEPD_BLACK     0x0000
#define GxEPD_DARKGREY  0x7BEF      /* 128, 128, 128 */
#define GxEPD_LIGHTGREY 0xC618      /* 192, 192, 192 */
#define GxEPD_WHITE     0xFFFF
#define GxEPD_RED       0xF800      /* 255,   0,   0 */

class GxEPD2
{
  public:
    enum Panel
    {
      GDEP015OC1, WS_1_54_bw = GDEP015OC1,
      GDE0213B1,  WS_2_13_bw = GDE0213B1,
      GDEH029A1,  WS_2_9_bw = GDEH029A1,
      GDEW027W3,  WS_2_7_bw = GDEW027W3,
      GDEW042T2,  WS_4_2_bw = GDEW042T2,
      GDEW075T8,  WS_7_5_bw = GDEW075T8,
      // 3-color
      GDEW0154Z04, Waveshare_1_54_bwr = GDEW0154Z04,
      GDEW0213Z16, Waveshare_2_13_bwr = GDEW0213Z16,
      GDEW029Z10,  Waveshare_2_9_bwr = GDEW029Z10,
      GDEW027C44,  Waveshare_2_7_bwr = GDEW027C44,
      GDEW042Z15,  Waveshare_4_2_bwr = GDEW042Z15
    };
    struct ScreenDimensionType
    {
      uint16_t width;
      uint16_t height;
    };
    static const ScreenDimensionType ScreenDimensions[];
};
#endif

