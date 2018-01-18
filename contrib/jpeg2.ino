#include <EEPROM.h>
#include <SPI.h>
#include <GD2.h>

void setup() //' A{
{
  Serial.begin(1000000); // JCB
  GD.begin();

  GD.BitmapHandle(0);
  GD.cmd_loadimage(0, 0);
  GD.load("sunrise.jpg");

  GD.BitmapHandle(1);
  GD.cmd_loadimage(-1, 0);
  GD.load("healsky3.jpg");
}

void loop()
{
  GD.Clear();
  GD.Begin(BITMAPS);
  GD.Vertex2ii(0,   0,   0);    // handle 0: sunrise
  GD.Vertex2ii(200, 100, 1);    // handle 1: healsky3
  GD.swap();
} //' }A
