#include <EEPROM.h>
#include <SPI.h>
#include <GD2.h>

void setup() //' a{
{
  Serial.begin(115200); // JCB
  GD.begin(~GD_STORAGE);

  GD.cmd_memset(0, 0x00, long(GD.w) * GD.h);     // clear the bitmap

  GD.Clear();                                 // draw the bitmap

  GD.ColorRGB(0x202030);
  GD.cmd_text(GD.w / 2, GD.h / 2, 31, OPT_CENTER, "sketch demo");

  GD.BitmapLayout(L8, GD.w, GD.h);
  GD.BitmapSize(NEAREST, BORDER, BORDER, GD.w, GD.h);

  GD.Begin(BITMAPS);
  GD.ColorRGB(0xffffff);
  GD.Vertex2ii(0, 0);
  GD.swap();
  GD.cmd_sketch(0, 0, GD.w, GD.h, 0, L8);     // start sketching
  GD.finish();                                // flush all commands
}

void loop() { } //' }a
/* JCB{
  delay(60000);
  GD.cmd_stop();
  GD.dumpscreen();
} }JCB */ 
