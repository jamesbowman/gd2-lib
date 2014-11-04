#include <EEPROM.h> //' A{
#include <SPI.h>
#include <GD2.h>

#include "fonts_assets.h"

void setup()
{
  Serial.begin(1000000); // JCB
  GD.begin();
  LOAD_ASSETS();
}

void loop()
{
  GD.ClearColorRGB(0x103000);
  GD.Clear();
  for (int i = 0; i < 16; i++) {
    int x = (i & 8) ? 260 : 20;
    int y = 12 + 33 * (i % 8);
    GD.BlendFunc(SRC_ALPHA, ONE_MINUS_SRC_ALPHA);
    GD.cmd_number(x, y, 26, OPT_CENTERY, 16 + i);
    // GD.BlendFunc(SRC_ALPHA, ZERO);
    GD.cmd_text(x + 20, y, 16 + i, OPT_CENTERY, "This font");
  }
  GD.swap();

  GD.ClearColorRGB(0x103000);
  GD.Clear();
  byte font = NIGHTFONT_HANDLE;  //' night{
  GD.cmd_text(240, 40, font, OPT_CENTER, "abcdefghijklm");
  GD.cmd_text(240, 100, font, OPT_CENTER, "nopqrstuvwxyz");
  GD.cmd_text(240, 160, font, OPT_CENTER, "ABCDEFGHIJKLM");
  GD.cmd_text(240, 220, font, OPT_CENTER, "NOPQRSTUVWXYZ"); //' }night
  GD.swap();
  
  for (int i = 0; i < 10; i++) {
    GD.ClearColorRGB(0x301000);
    GD.Clear();
    GD.cmd_text(240, 136, 31, OPT_CENTER, "Night font");
    GD.swap();
  }

} //' }A
