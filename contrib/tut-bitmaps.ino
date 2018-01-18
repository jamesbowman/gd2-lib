#include <EEPROM.h>
#include <SPI.h>
#include <GD2.h>


Bitmap hello;
Bitmap world;

void setup()
{
  GD.begin();

  hello.fromtext(31, "hello");
  world.fromtext(31, "world");
}

int angle = DEGREES(0);

void loop()
{

  GD.Clear();

  int x = GD.w / 2;
  int y = GD.h / 2;

  GD.ColorRGB(0x008080);
  hello.draw(x, y, angle);

  GD.ColorRGB(0xffa500);
  world.draw(x, y, -angle);

  angle += DEGREES(1);

  GD.swap();
}
