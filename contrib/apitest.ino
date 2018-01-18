#include <EEPROM.h>
#include <SPI.h>
#include <GD2.h>

#include "apitest_assets.h"

static int y = 0;

static int null()
{
  return 0;
}

static int inflate()
{
  GD.cmd_inflate(0);
  GD.copy(compressed, sizeof(compressed));
  GD.finish();
  return 0;
}

static int setfont()
{
  uint32_t base = 76000;
  GD.cmd_inflate(base);
  GD.copy(compressed, sizeof(compressed));

  GD.BitmapHandle(12);
  GD.BitmapSource(base + 148);
  GD.BitmapLayout(L4, 8, 17);
  GD.BitmapSize(NEAREST, BORDER, BORDER, 16, 17);
  GD.cmd_setfont(12, base);

  GD.cmd_text(200, y, 12, 0, "abcdefghijklmnopqrstuvwxyz");
  GD.finish();
  return 0;
}

/* atan2(): check for hangs in atan2() */
static int atan2()
{
  int16_t tricky[6] = {-32768, -32767, -1, 0, 1, 32767};
  byte i, j;
  for (i = 0; i < 6; i++)
    for (j = 0; j < 6; j++)
      GD.atan2(tricky[i], tricky[j]);
  return 0;
}

static int randrange()
{
  const size_t N = 21;
  uint16_t slots[N];

  memset(slots, 0, sizeof(slots));
  for (int i = 0; i < N * 100; i++)
    slots[GD.random(N)]++;
  GD.Begin(POINTS);

  uint32_t total = 0;
  for (int i = 0; i < N; i++) {
    GD.PointSize(slots[i]);
    GD.Vertex2f(16 * 200 + i * (16 * 280UL) / N, 16 * (y + 7));

    if (slots[i] < 20)
      return 1;
    if (slots[i] > 180)
      return 1;
    total += slots[i];
  }

  return (total == (N * 100)) ? 0 : 1;
}

static int textsize()
{
  int w, h;

  GD.textsize(w, h, 31, "");
  if ((w != 0) || (h != 49))
    return 1;
  GD.textsize(w, h, 26, "hello world");
  if ((w != 68) || (h != 16))
    return 1;

  return 0;
}

#define DOTEST(NAME) \
  do { \
  GD.cmd_text(110, y, 20, OPT_RIGHTX, #NAME); \
  if (NAME()) \
    GD.cmd_text(125, y, 21, 0, "FAIL"); \
  else \
    GD.cmd_text(125, y, 20, 0, "pass"); \
  y += 14; \
  } while (0)

void setup()
{
  Serial.begin(1000000); // JCB
  GD.begin(0);
  GD.cmd_memset(0, 0x40000UL, 0);
  GD.Clear();

  DOTEST(null);
  DOTEST(inflate);
  DOTEST(setfont);
  DOTEST(atan2);
  DOTEST(randrange);
  DOTEST(textsize);

  GD.swap();
}

void loop()
{
}
