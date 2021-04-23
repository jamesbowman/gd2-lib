#include <EEPROM.h>
#include <SPI.h>
#include <GD2.h>

void setup()
{
  Serial.begin(1000000);
  GD.begin(~GD_STORAGE);

  GD.cmd_flashdetach();
  GD.cmd_flashspidesel();
  GD.cmd_flashspitx(1);
  GD.cmd32(0x9f);
  GD.cmd_flashspirx(0,3);
  GD.finish();
  for (int i = 0; i < 3; i++)
    Serial.println(GD.rd(i), HEX);
}

void loop()
{
  GD.Clear();
  GD.swap();
}
