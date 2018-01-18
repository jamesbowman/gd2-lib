#include <EEPROM.h>
#include <SPI.h>
#include <GD2.h>

void setup() //' A{
{
  Serial.begin(1000000); // JCB
  GD.begin();
  GD.cmd_loadimage(0, 0);
  GD.load("healsky3.jpg");
}

void loop()
{
  GD.Clear();
  GD.Begin(BITMAPS);
  GD.Vertex2ii(0, 0);
  GD.swap();
} //' }A
