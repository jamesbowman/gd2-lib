#include <EEPROM.h>
#include <SPI.h>
#include <GD2.h>

#include "formats_assets.h" //' A{

void setup() 
{
  Serial.begin(1000000); // JCB
  GD.begin();
  LOAD_ASSETS();
}

void loop()
{
  GD.ClearColorRGB(0x000050);
  GD.Clear();
  GD.Begin(BITMAPS);
  GD.Vertex2ii(10, 10, PHOTO_RGB565_HANDLE);
  GD.Vertex2ii(10, 60, PHOTO_ARGB1555_HANDLE);
  GD.Vertex2ii(10, 110, PHOTO_ARGB4_HANDLE);
  GD.Vertex2ii(10, 160, PHOTO_RGB332_HANDLE);
  GD.Vertex2ii(10, 210, PHOTO_ARGB2_HANDLE);

  GD.Vertex2ii(240, 10, PHOTO_L1_HANDLE);
  GD.swap();
} //' }A
