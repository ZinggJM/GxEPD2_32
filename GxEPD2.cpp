#include "GxEPD2.h"

const GxEPD2::ScreenDimensionType GxEPD2::ScreenDimensions[]
{
  // note: width must be multiple of 8
  {200, 200}, // GDEP015OC1
  {128, 250}, // GDE0213B1
  {128, 296}, // GDEH029A1
  {176, 264}, // GDEW027W3
  {400, 300}, // GDEW042T2
  {640, 384}, // GDEW075T8
  // 3-color
  {200, 200}, // GDEW0154Z04
  {104, 212}, // GDEW0213Z16
  {128, 296}, // GDEW029Z10
  {176, 264}, // GDEW027C44
  {400, 300}, // GDEW042Z15
  {640, 384}  // GDEW075Z09
};

