#include <EEPROM.h>
#include <SPI.h>
#include <GD2.h>

void setup()
{
  Serial.begin(1000000); // JCB
  GD.begin();
}

void loop() //' a{
{
  GD.Clear();
  GD.Begin(POINTS);
  for (int i = 0; i < 100; i++) {
    GD.PointSize(GD.random(16 * 50));
    GD.ColorRGB(GD.random(256),
                GD.random(256),
                GD.random(256));
    GD.ColorA(GD.random(256));
    GD.Vertex2f(GD.random(16 * GD.w), GD.random(16 * GD.h));
  }
  GD.swap();
} //' }a
