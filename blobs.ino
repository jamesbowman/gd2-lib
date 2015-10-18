#include <EEPROM.h>
#include <SPI.h>
#include <GD2.h>

#define NBLOBS      128 //' a{
#define OFFSCREEN   -16384

struct xy {
  int x, y;
} blobs[NBLOBS];

void setup()
{
  GD.begin();

  for (int i = 0; i < NBLOBS; i++) {
    blobs[i].x = OFFSCREEN;
    blobs[i].y = OFFSCREEN;
  }
  Serial.begin(1000000);    // JCB
} //' }a

void loop() //' b{
{
  static byte blob_i;
  GD.get_inputs();
  if (GD.inputs.x != -32768) {
    blobs[blob_i].x = GD.inputs.x << 4; //' xy{
    blobs[blob_i].y = GD.inputs.y << 4; //' }xy
  } else {
    blobs[blob_i].x = OFFSCREEN;
    blobs[blob_i].y = OFFSCREEN;
  }
  blob_i = (blob_i + 1) & (NBLOBS - 1);

  GD.ClearColorRGB(0xd0d0c0); // JCB
  GD.ClearColorRGB(0xe0e0e0);
  GD.Clear();

  GD.ColorRGB(0xa0a0a0);
  GD.cmd_text(240, 136, 31, OPT_CENTER, "touch to draw");

  // GD.Begin(BITMAPS);      // JCB
  // GD.BitmapSize(NEAREST, REPEAT, REPEAT, 480, 272); // JCB
  // GD.Vertex2ii(0, 0); // JCB

  // Draw the blobs from oldest to youngest // JCB
  GD.Begin(POINTS);
  for (int i = 0; i < NBLOBS; i++) {
    // Blobs fade away and swell as they age
    GD.ColorA(i << 1);
    GD.PointSize((1024 + 16) - (i << 3));

    // Random color for each blob, keyed from (blob_i + i)
    uint8_t j = (blob_i + i) & (NBLOBS - 1);
    byte r = j * 17;
    byte g = j * 23;
    byte b = j * 147;
    GD.ColorRGB(r, g, b);

    // Draw it!
    GD.Vertex2f(blobs[j].x, blobs[j].y);
  }
  GD.swap();
} //' }b
