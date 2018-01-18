#include <EEPROM.h>
#include <SPI.h>
#include <GD2.h>

void setup()
{
  Serial.begin(1000000); // JCB
  GD.begin();
}

static void zigzag(int x) //' a{
{
  GD.Vertex2ii(x - 10,   10); GD.Vertex2ii(x + 10,   60);
  GD.Vertex2ii(x - 10,  110); GD.Vertex2ii(x + 10,  160);
  GD.Vertex2ii(x - 10,  210); GD.Vertex2ii(x + 10,  260);
}

void loop()
{
  GD.Clear();
  GD.Begin(LINES);
  zigzag(140);
  GD.Begin(LINE_STRIP);
  zigzag(240);
  GD.LineWidth(16 * 10);
  GD.Begin(LINE_STRIP);
  zigzag(340);
  GD.swap();
} //' }a
