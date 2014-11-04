#include <EEPROM.h>
#include <SPI.h>
#include <GD2.h>

#include "reflection_assets.h"

void setup()
{
  Serial.begin(1000000); // JCB
  GD.begin();
  LOAD_ASSETS();
}

static void showit()
{
  GD.ColorMask(1,1,1,1);
  GD.BlendFunc(DST_ALPHA, ZERO);
  GD.Begin(RECTS);
  GD.Vertex2ii(0, 0);
  GD.Vertex2ii(480, 272);
  GD.swap();
}

#define MASK_ALPHA()   GD.BlendFunc(ZERO, SRC_ALPHA)  //' A{

void loop()
{
  int x = 240 - GAMEDUINO_WIDTH / 2;

  GD.BitmapHandle(GRADIENT_HANDLE);
  GD.BitmapSize(NEAREST, REPEAT, BORDER, 480, 272);

  GD.Clear();
  GD.ColorMask(1, 1, 1, 0);            // don't touch A yet
  GD.cmd_gradient(0, 40, 0x505060,
                  0, 272, 0xc0c080);

  GD.Begin(BITMAPS);                   // top bitmap
  GD.Vertex2ii(x, 80, GAMEDUINO_HANDLE);

  GD.ColorMask(0, 0, 0, 1);
  GD.BlendFunc(ONE, ZERO);
  GD.Vertex2ii(0, 180, GRADIENT_HANDLE);

                                       // invert the image
  GD.cmd_translate(0, F16(GAMEDUINO_HEIGHT / 2));
  GD.cmd_scale(F16(1), F16(-1));
  GD.cmd_translate(0, -F16(GAMEDUINO_HEIGHT / 2));
  GD.cmd_setmatrix();

  MASK_ALPHA();                        // mask with gradient
  GD.Vertex2ii(x, 190, GAMEDUINO_HANDLE);

  GD.ColorMask(1, 1, 1, 0);            // draw the reflection
  GD.BlendFunc(DST_ALPHA, ONE_MINUS_DST_ALPHA);
  GD.Vertex2ii(x, 190, GAMEDUINO_HANDLE);
  GD.swap();
// JCB{
  return;
  for (int i = 0; i < 100; i++) {
    GD.Clear();
    GD.ColorMask(0,0,0,1);
    GD.BlendFunc(ONE, ZERO);
    GD.Begin(BITMAPS);
    GD.Vertex2ii(0, 180, GRADIENT_HANDLE);

    showit();

    GD.Clear();
    GD.ColorMask(0,0,0,1);
    GD.BlendFunc(ONE, ZERO);
    GD.Begin(BITMAPS);
    GD.Vertex2ii(0, 190, GRADIENT_HANDLE);
    MASK_ALPHA();                                       // mask with gradient
    GD.cmd_translate(0, F16(GAMEDUINO_HEIGHT / 2));
    GD.cmd_scale(F16(1), F16(-1));
    GD.cmd_translate(0, -F16(GAMEDUINO_HEIGHT / 2));
    GD.cmd_setmatrix();
    GD.Vertex2ii(x, 190, GAMEDUINO_HANDLE);

    showit();

    GD.Clear();
    GD.ColorMask(0,0,0,1);
    GD.BlendFunc(ONE, ZERO);
    GD.Begin(BITMAPS);
    GD.cmd_translate(0, F16(GAMEDUINO_HEIGHT / 2));
    GD.cmd_scale(F16(1), F16(-1));
    GD.cmd_translate(0, -F16(GAMEDUINO_HEIGHT / 2));
    GD.cmd_setmatrix();
    GD.Vertex2ii(x, 190, GAMEDUINO_HANDLE);

    showit();
  } // }JCB
} //' }A
