#include <EEPROM.h> //' A{
#include <SPI.h>
#include <GD2.h>

static int x0[17], yy0[17];
static int x1[17], yy1[17];

void setup()
{
  Serial.begin(115200); // JCB
  GD.begin();
  for (byte th = 0; th < 17; th++) {
    int x, y;
    GD.polar(x, y, 16 * 128, th << 12);
    x0[th] = 16 * 240 + x;
    yy0[th] = 16 * 136 + y;
    GD.polar(x, y, 16 * 16, th << 12);
    x1[th] = 16 * 240 + x;
    yy1[th] = 16 * 190 + y;
  }
}

static void polar(int r, uint16_t th)
{
  int x, y;
  GD.polar(x, y, 16 * r, th);
  GD.Vertex2f(16 * 240 + x, 16 * 136 + y);
}

static void drawgame()
{
  GD.ColorRGB(0x2020ff);
  GD.Begin(LINE_STRIP);
  for (byte i = 0; i < 17; i++)
    GD.Vertex2f(x0[i], yy0[i]);
  GD.Begin(LINE_STRIP);
  for (byte i = 0; i < 17; i++)
    GD.Vertex2f(x1[i], yy1[i]);
  GD.Begin(LINES);
  for (byte i = 0; i < 17; i++) {
    GD.Vertex2f(x0[i], yy0[i]);
    GD.Vertex2f(x1[i], yy1[i]);
  }
  GD.ColorRGB(0xffff00);
  GD.Begin(LINE_STRIP);
  polar(128, 0x3000);
  polar(142, 0x3900);
  polar(128, 0x4000);
  polar(118, 0x3c00);
  polar(128, 0x3e00);
  polar(134, 0x3900);
  polar(128, 0x3300);
  polar(118, 0x3400);
  polar(128, 0x3000);
}

void loop()
{
  GD.Clear();                   // Clear to black //' draw{

  GD.ColorA(0x30);              // Draw background glows
  GD.LineWidth(48);
  drawgame();

  GD.ColorA(0xff);              // Draw foreground vectors
  GD.LineWidth(10);
  GD.BlendFunc(SRC_ALPHA, ONE); // additive blending
  drawgame(); //' }draw

  GD.swap();  
  // GD.dumpscreen();    // JCB
}
