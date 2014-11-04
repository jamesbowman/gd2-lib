#include <EEPROM.h>
#include <SPI.h>
#include <GD2.h>

void setup() //' a{
{
  Serial.begin(115200); // JCB
  GD.begin();
  GD.cmd_memset(0, 0, 480UL * 272UL);   // clear the bitmap
  GD.Clear();                           // draw the bitmap
  GD.BitmapLayout(L8, 480, 272);
  GD.BitmapSize(NEAREST, BORDER, BORDER, 480, 272);
  GD.Begin(BITMAPS);
  GD.Vertex2ii(0, 0);
  GD.swap();
  GD.cmd_sketch(0, 0, 480, 272, 0, L8); // start sketching
  GD.finish();                          // flush all commands
}

void loop() { } //' }a
/* JCB{
  delay(60000);
  GD.cmd_stop();
  GD.dumpscreen();
} }JCB */ 
