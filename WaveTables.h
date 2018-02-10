#ifndef _WaveTables_H_
#define _WaveTables_H_

const uint8_t GDEP015OC1_LUTDefault_full[] =
{
  0x32,  // command
  0x50, 0xAA, 0x55, 0xAA, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x1F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

const uint8_t GDEP015OC1_LUTDefault_part[] =
{
  0x32,  // command
  0x10, 0x18, 0x18, 0x08, 0x18, 0x18, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x13, 0x14, 0x44, 0x12, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

const uint8_t GxGDE0213B1_LUTDefault_full[] =
{
  0x32,  // command
  0x22, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x01, 0x00, 0x00, 0x00, 0x00
};

const uint8_t GxGDE0213B1_LUTDefault_part[] =
{
  0x32,  // command
  0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x0F, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

const uint8_t GxGDEH029A1_LUTDefault_full[] =
{
  0x32,  // command
  0x50, 0xAA, 0x55, 0xAA, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x1F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

const uint8_t GxGDEH029A1_LUTDefault_part[] =
{
  0x32,  // command
  0x10, 0x18, 0x18, 0x08, 0x18, 0x18, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x13, 0x14, 0x44, 0x12, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

const uint8_t GxGDEW027W3_lut_20_vcomDC[] =
{
0x00  ,0x00 ,
0x00  ,0x0F ,0x0F ,0x00 ,0x00 ,0x05,    
0x00  ,0x32 ,0x32 ,0x00 ,0x00 ,0x02,    
0x00  ,0x0F ,0x0F ,0x00 ,0x00 ,0x05,    
0x00  ,0x00 ,0x00 ,0x00 ,0x00 ,0x00,    
0x00  ,0x00 ,0x00 ,0x00 ,0x00 ,0x00,    
0x00  ,0x00 ,0x00 ,0x00 ,0x00 ,0x00,    
0x00  ,0x00 ,0x00 ,0x00 ,0x00 ,0x00,
};
//R21H
const uint8_t GxGDEW027W3_lut_21_ww[] =
{
0x50  ,0x0F ,0x0F ,0x00 ,0x00 ,0x05,
0x60  ,0x32 ,0x32 ,0x00 ,0x00 ,0x02,
0xA0  ,0x0F ,0x0F ,0x00 ,0x00 ,0x05,
0x00  ,0x00 ,0x00 ,0x00 ,0x00 ,0x00,
0x00  ,0x00 ,0x00 ,0x00 ,0x00 ,0x00,
0x00  ,0x00 ,0x00 ,0x00 ,0x00 ,0x00,
0x00  ,0x00 ,0x00 ,0x00 ,0x00 ,0x00,
};
//R22H  r
const uint8_t GxGDEW027W3_lut_22_bw[] =
{
0x50  ,0x0F ,0x0F ,0x00 ,0x00 ,0x05,
0x60  ,0x32 ,0x32 ,0x00 ,0x00 ,0x02,
0xA0  ,0x0F ,0x0F ,0x00 ,0x00 ,0x05,
0x00  ,0x00 ,0x00 ,0x00 ,0x00 ,0x00,
0x00  ,0x00 ,0x00 ,0x00 ,0x00 ,0x00,
0x00  ,0x00 ,0x00 ,0x00 ,0x00 ,0x00,
0x00  ,0x00 ,0x00 ,0x00 ,0x00 ,0x00,
};
//R23H  w
const uint8_t GxGDEW027W3_lut_23_wb[] =
{
0xA0  ,0x0F ,0x0F ,0x00 ,0x00 ,0x05,
0x60  ,0x32 ,0x32 ,0x00 ,0x00 ,0x02,
0x50  ,0x0F ,0x0F ,0x00 ,0x00 ,0x05,
0x00  ,0x00 ,0x00 ,0x00 ,0x00 ,0x00,
0x00  ,0x00 ,0x00 ,0x00 ,0x00 ,0x00,
0x00  ,0x00 ,0x00 ,0x00 ,0x00 ,0x00,
0x00  ,0x00 ,0x00 ,0x00 ,0x00 ,0x00,
};
//R24H  b
const uint8_t GxGDEW027W3_lut_24_bb[] =
{
0xA0  ,0x0F ,0x0F ,0x00 ,0x00 ,0x05,
0x60  ,0x32 ,0x32 ,0x00 ,0x00 ,0x02,
0x50  ,0x0F ,0x0F ,0x00 ,0x00 ,0x05,
0x00  ,0x00 ,0x00 ,0x00 ,0x00 ,0x00,
0x00  ,0x00 ,0x00 ,0x00 ,0x00 ,0x00,
0x00  ,0x00 ,0x00 ,0x00 ,0x00 ,0x00,
0x00  ,0x00 ,0x00 ,0x00 ,0x00 ,0x00,
};

#endif

