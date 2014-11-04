#include <EEPROM.h> //' A{
#include <SPI.h>
#include <GD2.h>

void setup()
{
  Serial.begin(1000000); // JCB
  GD.begin(0);
}

void loop()
{
  GD.ClearColorRGB(0x103000);
  GD.Clear();
  GD.cmd_text(240, 136, 31, OPT_CENTER, "Hello world");
  GD.swap();
} //' }A
