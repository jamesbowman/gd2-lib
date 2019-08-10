#include <EEPROM.h>
#include <SPI.h>
#include <GD2.h>

// >>> list(open("jet.png","rb").read())
static PROGMEM uint8_t jet[] = {
137, 80, 78, 71, 13, 10, 26, 10, 0, 0, 0, 13, 73, 72, 68, 82, 0, 0, 1, 0, 0, 0, 0, 1, 8, 2, 0, 0, 0, 190, 59, 231, 32, 0, 0, 0, 9, 112, 72, 89, 115, 0, 0, 46, 35, 0, 0, 46, 35, 1, 120, 165, 63, 118, 0, 0, 0, 7, 116, 73, 77, 69, 7, 227, 7, 30, 2, 34, 17, 144, 100, 141, 140, 0, 0, 0, 25, 116, 69, 88, 116, 67, 111, 109, 109, 101, 110, 116, 0, 67, 114, 101, 97, 116, 101, 100, 32, 119, 105, 116, 104, 32, 71, 73, 77, 80, 87, 129, 14, 23, 0, 0, 0, 227, 73, 68, 65, 84, 40, 207, 141, 144, 193, 110, 3, 49, 8, 68, 159, 13, 139, 155, 67, 255, 186, 63, 220, 83, 179, 54, 208, 30, 54, 222, 58, 105, 34, 213, 7, 235, 49, 26, 6, 68, 129, 15, 48, 80, 216, 158, 253, 107, 105, 80, 161, 77, 229, 47, 31, 54, 225, 159, 207, 160, 193, 59, 52, 48, 16, 184, 204, 60, 89, 178, 79, 214, 101, 230, 49, 199, 166, 168, 83, 127, 96, 91, 22, 180, 37, 228, 81, 255, 70, 147, 154, 88, 71, 3, 73, 52, 176, 142, 4, 36, 4, 12, 184, 113, 161, 55, 162, 145, 111, 196, 5, 55, 188, 18, 70, 52, 92, 241, 74, 54, 134, 18, 133, 84, 252, 100, 33, 140, 174, 68, 37, 5, 55, 134, 226, 133, 84, 98, 163, 11, 94, 239, 89, 112, 163, 11, 94, 8, 197, 55, 122, 33, 149, 174, 140, 141, 107, 33, 132, 47, 229, 186, 241, 89, 25, 50, 79, 95, 96, 229, 243, 126, 245, 208, 3, 9, 106, 162, 142, 12, 72, 112, 112, 88, 185, 63, 211, 99, 234, 177, 120, 14, 78, 24, 175, 27, 87, 30, 47, 2, 199, 61, 231, 116, 30, 225, 251, 111, 99, 94, 113, 191, 89, 118, 112, 216, 151, 141, 250, 228, 113, 207, 103, 134, 63, 227, 31, 121, 109, 158, 132, 179, 166, 13, 3, 0, 0, 0, 0, 73, 69, 78, 68, 174, 66, 96, 130
};

class SpecGram
{
  uint32_t base;
  int y;
  int width, history;

public:
  void setup(int _width = 256, int _history = 256) {
    // width is the number of freqency buckets
    // history is how many samples to keep

    GD.cmd_loadimage(0, 0);
    GD.copy(jet, sizeof(jet));
    base = 512UL;
    width = _width;
    history = _history;
    y = 0;

    GD.cmd_memset(base, 0, width * history);
    GD.BitmapSource(base);
    GD.BitmapLayout(14 /*PALETTED565*/, width, history);
    GD.BitmapSize(NEAREST, BORDER, REPEAT, width, history - 1);
  }
  void draw() {
    GD.Begin(BITMAPS);
    GD.SaveContext();
    GD.BitmapTransformF(((y + 2) & (history - 1)) << 8);
    GD.Vertex2f(16 * 112, 16 * 10);
    GD.RestoreContext();
  }
  void update(byte newdata[]) {
    y = (y + 1) % history;
    GD.cmd_memwrite(base + width * y, width);
    GD.copyram(newdata, width);
  }
};

class SpecGram spectrogram;

void setup()
{
  GD.begin();

  spectrogram.setup();
}

int t;

void loop()
{
  byte buf[256];
  for (int i = 0; i < 256; i++) {
    int phase = GD.rcos(29509, t * 397);
    int freq = 800 + GD.rcos(600, 0x3333 + t * 443);
    buf[i] = 128 + GD.rsin(127, phase + i * freq);
  }
  spectrogram.update(buf);

  GD.Clear();
  spectrogram.draw();
  GD.swap();
  t++;
}
