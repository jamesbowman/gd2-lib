#include <EEPROM.h> //' a{
#include <SPI.h>
#include <GD2.h>

#include "tiled_assets.h"

void setup()
{
  Serial.begin(1000000); // JCB
  GD.begin();
  LOAD_ASSETS();
}

int t;

void loop()
{
  GD.Clear();
  GD.Begin(BITMAPS);
  GD.BitmapSize(BILINEAR, REPEAT, REPEAT, GD.w, GD.h);
  GD.cmd_translate(F16(-t), 0);
  GD.cmd_rotate(3333);
  GD.cmd_setmatrix();
  GD.Vertex2f(0, 0);
  GD.swap();

  t += 1;
} //' }a
