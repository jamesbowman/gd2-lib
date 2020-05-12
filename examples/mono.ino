#include <EEPROM.h> //' a{
#include <SPI.h>
#include <GD2.h>

#include "mono_assets.h"

void setup()
{
  Serial.begin(1000000); // JCB
  GD.begin();
  LOAD_ASSETS();
}

void loop()
{
  GD.ClearColorRGB(0x375e03);
  GD.Clear();
  // GD.ClearColorRGB(GD.random(255), GD.random(255), GD.random(255)); // JCB
  GD.Begin(BITMAPS);
  GD.ColorRGB(0x68b203);
  // GD.ColorRGB(GD.random(255), GD.random(255), GD.random(255)); // JCB
  GD.BitmapSize(NEAREST, REPEAT, REPEAT, GD.w, GD.h);
  GD.Vertex2f(0, 0);
  GD.swap();
} //' }a
