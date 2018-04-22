#include <EEPROM.h>
#include <SPI.h>
#include <GD2.h>

static int a[256]; //' A{

#include "walk_assets.h" //' load{

void setup() 
{
  Serial.begin(1000000); // JCB
  GD.begin(~GD_STORAGE);
  LOAD_ASSETS();  //' }load
  for (int i = 0; i < 256; i++)
    a[i] = GD.random(512);
}

void loop()
{
  GD.ClearColorRGB(0x000050);
  GD.Clear();
  GD.Begin(BITMAPS);
  for (int i = 0; i < 256; i++) {
    GD.ColorRGB(i, i, i);
    GD.Vertex2ii(a[i], i, WALK_HANDLE, (a[i] >> 2) & 7);
    a[i] = (a[i] + 1) & 511;
  }
  GD.swap();
} //' }A

/*
    GD.Vertex2ii(x, y, WALK_HANDLE);   //' asset{ }asset
*/
