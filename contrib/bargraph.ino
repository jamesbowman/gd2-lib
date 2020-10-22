#include <EEPROM.h>
#include <SPI.h>
#include <GD2.h>

void setup() 
{
  Serial.begin(1000000);
  GD.begin(~GD_STORAGE);

  GD.BitmapHandle(0);
  GD.cmd_setbitmap(0, BARGRAPH, 1280, 256);
  for (int i = 0; i < 1280; i++)
    GD.wr(i, random(256));
}

void loop()
{
  GD.VertexFormat(0);

  GD.ClearColorRGB(0x000050);
  GD.Clear();


  GD.Begin(BITMAPS);
  GD.BitmapHandle(0);

  // First, draw the bargraph with no scaling
  GD.Vertex2f(0, 50);

  // Now scale in y by 0.5
  GD.cmd_loadidentity();
  GD.cmd_scale(F16(1), F16(0.5));
  GD.cmd_setmatrix();

  GD.Vertex2f(0, 500);
  GD.swap();
}
