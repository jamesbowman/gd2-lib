#include <EEPROM.h>
#include <SPI.h>
#include <GD2.h>

void setup()
{
  GD.begin();
}

void loop()
{
  MoviePlayer mp;

  mp.begin("fun-1500.avi");
  mp.play();
}
