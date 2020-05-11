#include <EEPROM.h>
#include <SPI.h>
#include <GD2.h>

#define GD3 ft8xx_model

#define UART_SPEED 115200

#include "selftest_assets.h"

#define SCREEN_ADDR 0x30000UL

byte x, y;

static void log(const char*s)
{
  while (*s) {
    char c = *s++;
#ifndef RASPBERRY_PI
    Serial.write(c);
#endif
    if (c == '\n') {
      x = 0;
      y++;
    } else {
      uint32_t dst = SCREEN_ADDR + (((x + (y * 48)) << 1));
      GD.wr16(dst, 0x0f00 | c);
      x++;
    }
  };
}

void setup()
{
  Serial.begin(UART_SPEED);
  Serial.println("---- GAMEDUINO 2/3 SELFTEST ----");
}

static void ramp(int y, uint32_t color)
{
  GD.ScissorSize(400, 8);
  GD.ScissorXY(40, y);
  GD.cmd_gradient(40, 0, 0x000000, 440, 0, color);
}

void testcard(int pass, const char *message)
{
  // GD.ClearColorRGB(0x204060);

  GD.get_inputs();
  GD.Clear();
  GD.cmd_text(GD.w / 2, 12, 28, OPT_CENTER,
    GD3 ? "Gameduino3 Self test" :
          "Gameduino2 Self test");
  GD.cmd_text(GD.w - 2, 12, 27, OPT_CENTERY | OPT_RIGHTX,
    GD2_VERSION);
  int y;

  y = PIXELS(50);
  GD.Begin(POINTS);
  for (int i = 0; i < 6; i++) {
    byte l = 4 << i;
    int x = map(1 + i, 0, 7, 0, PIXELS(GD.w));
    GD.PointSize(280);
    GD.ColorRGB(0xffffff);
    GD.Vertex2f(x, y);

    GD.PointSize(240);
    GD.ColorRGB(l, l, l);
    GD.Vertex2f(x, y);
  }
  y += 30;

  ramp(y, 0xff0000);  y += 12;
  ramp(y, 0x00ff00);  y += 12;
  ramp(y, 0x0000ff);  y += 12;
  ramp(y, 0xffffff);  y += 12;
  GD.RestoreContext();

  // GD.Begin(BITMAPS);
  // GD.Vertex2ii(0, 272 - (8 * 16), 1, 0);
  // GD.Vertex2ii(480 - LENA_WIDTH, 272 - LENA_WIDTH, 0, 0);

  if (pass == -1)
    GD.ColorRGB(0x808000);
  else
    GD.ColorRGB(pass ? 0x40ff40 : 0xff4040);
  GD.cmd_text(GD.w / 2, 180, 31, OPT_CENTER, message);

  GD.ColorRGB(0xffffff);
  GD.Begin(LINES);

  GD.Vertex2f(PIXELS(GD.inputs.x), PIXELS(0));
  GD.Vertex2f(PIXELS(GD.inputs.x), PIXELS(GD.h));

  GD.Vertex2f(PIXELS(0),    PIXELS(GD.inputs.y));
  GD.Vertex2f(PIXELS(GD.w), PIXELS(GD.inputs.y));

  GD.swap();
  GD.finish();
}

#define SCREENTEST(NAME) \
  do { \
  Serial.println(#NAME); \
  testcard(-1, #NAME); \
  r = test_##NAME(); \
  const char* msg = r ? (#NAME ": pass") : (#NAME ": FAIL"); \
  Serial.println(msg); \
  testcard(r, msg); \
  while (!r) ; \
  } while (0)

int test_ident()
{
  byte id = GD.rd(REG_ID);
  if (id != 0x7c) {
    Serial.println(id, HEX);
    return 0;
  }
  return 1;
}

int test_clock()
{
  int SPEEDUP = 8;

  GD.rd32(REG_CLOCK); // warm-up
  delay(10);
  long t1 = GD.rd32(REG_CLOCK);
  delay(1000 / SPEEDUP);
  long t2 = GD.rd32(REG_CLOCK);

  float measured = float(t2 - t1);
  // measured should be 48e6, within 2%

  float expected = (GD3 ? 60e6 : 48e6) / SPEEDUP;
  Serial.println(measured, DEC);
  Serial.println(expected, DEC);
  float diff = measured - expected;
  float percent = fabs(100 * (diff / expected));
  return percent < 5.0;
}

int test_tune()
{
  GD.tune();
  return 1;
}

static byte test_RAM(void)
{
  uint32_t a;
  for (a = 0; a < 0x40000U; a += 947)
    GD.wr(a, a);
  for (a = 0; a < 0x40000U; a += 947) {
    if (GD.rd(a) != (a & 0xff))
      return 0;
  }
  return 1;
}

static byte test_PWM(void)
{
  for (int i = 128; i >= 0; i--) {
    GD.wr(REG_PWM_DUTY, i);
    delay(2);
  }
  GD.wr(REG_PWM_DUTY, 128);
  return 1;
}

static byte test_storage(void)
{
  GD.storage();
  return test_ident();
}

static byte test_SDcard(void)
{
  for (byte i = 0; i < 2; i++) {
    GD.safeload("selftest.gd2");
    uint32_t pcrc = GD.cmd_memcrc(0, ASSETS_END);
    GD.finish();
    uint32_t crc = GD.rd32(pcrc);
    if (crc != KITTEN_CRC)
      return 0;
  }
  return 1;
}

static void collect(int &rx, int &ry, int &rz)
{
  uint16_t ax = 0, ay = 0, az = 0;
  for (byte i = 32; i; i--) {
    int x = 0, y = 0, z = 0;
#ifdef A2
    x = analogRead(A2);
#endif
#ifdef A1
    y = analogRead(A1);
#endif
#ifdef A0
    z = analogRead(A0);
#endif
    ax += x;
    ay += y;
    az += z;
  }
  rx = ax >> 5;
  ry = ay >> 5;
  rz = az >> 5;
}

static byte test_accel2(void)
{
  while (1) {
    GD.finish();
    if ((millis() % 2000) < 1000)
      GD.wr(REG_GPIO, 0x80);
    else
      GD.wr(REG_GPIO, 0x81);
    int x, y, z;
    collect(x, y, z);
    GD.Clear();
    GD.cmd_number(0,  40, 26, 3, x);
    GD.cmd_slider(50,  40, 400, 10, 0, x, 512);

    GD.cmd_number(0,   70, 26, 3, y);
    GD.cmd_slider(50,  70, 400, 10, 0, y, 512);

    GD.cmd_number(0,  100, 26, 3, GD.rd(REG_GPIO));
    GD.cmd_slider(50, 100, 400, 10, 0, z, 512);
    GD.swap();
  }
  return 1;
}

static byte test_accel(void)
{
  int x0, y0, z0;
  int x1, y1, z1;

  GD.wr(REG_GPIO, 0x80);
  collect(x0, y0, z0);
  delay(100);
  GD.wr(REG_GPIO, 0x81);
  delay(100);
  collect(x1, y1, z1);

  Serial.print(x0); Serial.print(" "); Serial.print(y0); Serial.print(" "); Serial.println(z0);
  Serial.print(x1); Serial.print(" "); Serial.print(y1); Serial.print(" "); Serial.println(z1);

  // if ((x0 > x1) || (y0 > y1) || (z0 > z1)) return 0;

  int d;
  d = abs(x0 - x1);
  if ((d < 30) || (120 < d))
    return 0;

  d = abs(y0 - y1);
  if ((d < 30) || (120 < d))
    return 0;

  d = abs(z0 - z1);
  if ((d < 50) || (200 < d))
    return 0;
  
  z0 %= 37;
  while (z0--)
    GD.random();

  return 1;
}

static void play(uint16_t n)
{
  GD.wr16(REG_SOUND, n);
  GD.wr(REG_PLAY, 1);
}

static void play_wait(uint16_t n)
{
  play(n);
  while (GD.rd(REG_PLAY))
    ;
}

static byte test_touch(void)
{
  if (!GD3) {
    GD.self_calibrate();
    // write the new calibration back to EEPROM

#if !defined(RASPBERRY_PI) && !defined(__DUE__)
    for (int i = 0; i < 24; i++)
      EEPROM.write(1 + i, GD.rd(REG_TOUCH_TRANSFORM_A + i));
#endif
  }

  byte hit = 0;
  while (hit != 0x0f) {
    GD.finish();
    byte tag = GD.rd(REG_TOUCH_TAG);
    if ((1 <= tag) && (tag <= 4)) {
      play(0x50);
      hit |= (1 << (tag - 1));
    }
    if (tag == 77)
      return 0;
    GD.ClearTag(77);
    GD.Clear();
    GD.PointSize(20 * 16);
    GD.Begin(POINTS);
    for (byte i = 1; i <= 4; i++) {
      if (hit & (1 << (i - 1))) {
        GD.ColorRGB(0x00ff00);
        GD.Tag(0xff);
      } else {
        GD.ColorRGB(0x808080);
        GD.Tag(i);
      }
      switch (i) {
        case 1: GD.Vertex2ii(20,  20, 0, 0);  break;
        case 2: GD.Vertex2ii(460, 20, 0, 0);  break;
        case 3: GD.Vertex2ii(20,  250, 0, 0);  break;
        case 4: GD.Vertex2ii(460, 250, 0, 0);  break;
      }
    }
    GD.random();  // scramble PRN state for later
    GD.swap();
  }
  return 1;
}

static const PROGMEM uint32_t digits[11] = {
  DIGIT_0,
  DIGIT_1,
  DIGIT_2,
  DIGIT_3,
  DIGIT_4,
  DIGIT_5,
  DIGIT_6,
  DIGIT_7,
  DIGIT_8,
  DIGIT_9,
  DIGIT_9 + DIGIT_9_LENGTH
};

static void saydigit(byte n)
{
  GD.wr32(REG_PLAYBACK_FREQ, 8000);
  GD.wr32(REG_PLAYBACK_FORMAT, ADPCM_SAMPLES);

  uint32_t dstart = pgm_read_dword(digits + n);
  uint32_t dend = pgm_read_dword(digits + n + 1);
  GD.wr32(REG_PLAYBACK_START, dstart);
  GD.wr32(REG_PLAYBACK_LENGTH, dend - dstart);

  GD.wr(REG_PLAYBACK_PLAY, 1);
}

static void blank(int n)
{
  for (int i = 0; i < n; i++) {
    GD.get_inputs();
    GD.cmd_gradient(0, 0, 0xb0b0a0, 0, 272, 0x404040);
    GD.swap();
  }
}

static byte getkey()
{
  byte prev_tag;
  do {
    prev_tag = GD.inputs.tag;
    GD.get_inputs();
    if (GD.inputs.x & 1)
      GD.random();
    GD.cmd_gradient(0, 0, 0xb0b0a0, 0, 272, 0x404040);
    for (int i = 0; i < 9; i++) {
      byte digit = i + 1;
      int x = 120 + 80 * (i % 3);
      int y = 20 + 80 * (i / 3);
      GD.Tag(digit);
      char msg[2] = { '0' + digit, 0 };
      GD.cmd_fgcolor((digit == GD.inputs.tag) ? 0xc08000 : 0x003870);
      GD.cmd_button(x, y, 70, 70, 31, 0, msg);
    }
    GD.swap();
  } while (!((GD.inputs.tag == 0) && (1 <= prev_tag) && (prev_tag <= 9)));
  return prev_tag;
}

static byte test_audio(void)
{
  // Stir up the PRN
  for (int i = micros() % 97; i; i--)
    GD.random();
  blank(20);

  for (int i = 0; i < 3; i++) {
    byte d = 1 + GD.random(9);
    saydigit(d);
    blank(12);
    if (getkey() != d)
      return 0;
  }

  return 1;
}

static struct {
  byte t, note;
} pacman[]  = {
  { 0, 71 },
  { 2, 83 },
  { 4, 78 },
  { 6, 75 },
  { 8, 83 },
  { 9, 78 },
  { 12, 75 },
  { 16, 72 },
  { 18, 84 },
  { 20, 79 },
  { 22, 76 },
  { 24, 84 },
  { 25, 79 },
  { 28, 76 },
  { 32, 71 },
  { 34, 83 },
  { 36, 78 },
  { 38, 75 },
  { 40, 83 },
  { 41, 78 },
  { 44, 75 },
  { 48, 75 },
  { 49, 76 },
  { 50, 77 },
  { 52, 77 },
  { 53, 78 },
  { 54, 79 },
  { 56, 79 },
  { 57, 80 },
  { 58, 81 },
  { 60, 83 },
  { 255, 255 }
};

void loop()
{
  if (EEPROM.read(0) == 0x7c)
    EEPROM.write(0, 0xff);
  GD.begin(0);
  GD.self_calibrate();

  x = y = 0;
  testcard(1, "Starting tests");
  GD.finish();

  Serial.println("Starting self-test");

  byte r, pass = 1;

  {
    SCREENTEST(ident);
    if (!GD3)
      SCREENTEST(tune);
    SCREENTEST(clock);
    SCREENTEST(RAM);
    SCREENTEST(PWM);
    SCREENTEST(storage);
    SCREENTEST(SDcard);
    if (0)
      SCREENTEST(accel);
    if (1) {
      SCREENTEST(touch);
      SCREENTEST(audio);
    }
    {
      int i = 0, t = 0;
      for (;;) {
        testcard(1, "* ALL PASS *");
        if (t == 4 * pacman[i].t)
          GD.play(HARP, pacman[i++].note - 12);
        if (++t == 256) {
          t = 0;
          i = 0;
        }
      }
    }
  }

  if (pass) {
    char msg[60];
    log("All tests passed\n");
    long seconds = millis() / 1000;
    int minutes = seconds / 60;
    sprintf(msg, "%d minutes", minutes);
    log(msg);
  } else {
    for (;;)
      ;
  }
  delay(5000);
}
