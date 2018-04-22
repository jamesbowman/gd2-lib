#include <EEPROM.h>
#include <SPI.h>
#include <GD2.h>

void setup()
{
  Serial.begin(1000000); // JCB
  GD.begin(~GD_STORAGE);
}

void loop() //' a{
{
  GD.Clear();
  GD.Begin(POINTS);
  for (int i = 0; i < 100; i++) {
    GD.PointSize(GD.random(2 * GD.w));
    GD.ColorRGB(GD.random(), GD.random(), GD.random());
    GD.ColorA(GD.random());
    GD.Vertex2f(GD.random(PIXELS(GD.w)), GD.random(PIXELS(GD.h)));
  }
  GD.swap();
} //' }a
