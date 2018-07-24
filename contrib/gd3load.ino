#include <EEPROM.h>
#include <SPI.h>
#include <GD2.h>

#define SD_PIN        9
#include "transports/wiring.h"

#include "gd3load_assets.h"

GDTransport GDTR;

uint8_t gpio, gpio_dir;

void set_SDA(byte n)
{
  GDTR.__wr16(REG_GPIO_DIR, gpio_dir | (0x03 - n));    // Drive SCL, SDA low
}

void set_SCL(byte n)
{
  GDTR.__wr16(REG_GPIO, gpio | (n << 1));
}

int get_SDA(void)
{
  return GDTR.__rd16(REG_GPIO) & 1;
}

void i2c_start(void) 
{
  set_SDA(1);
  set_SCL(1);
  set_SDA(0);
  set_SCL(0);
}

void i2c_stop(void) 
{
  set_SDA(0);
  set_SCL(1);
  set_SDA(1);
  set_SCL(1);
}

int i2c_rx1()
{
  set_SDA(1);
  set_SCL(1);
  byte r = get_SDA();
  set_SCL(0);
  return r;
}

void i2c_tx1(byte b)
{
  set_SDA(b);
  set_SCL(1);
  set_SCL(0);
}

int i2c_tx(byte x)
{
  for (int i = 7; i >= 0; i--)
    i2c_tx1(1 & (x >> i));
  return i2c_rx1();
}

int i2c_rx(int nak)
{
  byte r = 0;
  for (byte i = 0; i < 8; i++)
    r = (r << 1) | i2c_rx1();
  i2c_tx1(nak);
  return r;
}

void i2c_begin(void)
{
  gpio = GDTR.__rd16(REG_GPIO) & ~3;
  gpio_dir = GDTR.__rd16(REG_GPIO_DIR) & ~3;

  // 2-wire software reset
  i2c_start();
  i2c_rx(1);
  i2c_start();
  i2c_stop();
}

#define ADDR  0xa0

void ram_write(const uint8_t *v)
{
  for (byte i = 0; i < 128; i += 8) {
    i2c_start();
    i2c_tx(ADDR);
    i2c_tx(i);
    for (byte j = 0; j < 8; j++)
      i2c_tx(*v++);
    i2c_stop();
    delay(6);
  }
}

byte ram_read(byte a)
{
  i2c_start();
  i2c_tx(ADDR);
  i2c_tx(a);

  i2c_start();
  i2c_tx(ADDR | 1);
  byte r = i2c_rx(1);
  i2c_stop();
  return r;
}

void ramdump(void)
{
  for (int i = 0; i < 128; i++) {
    byte v = ram_read(i);
    Serial.print(i, HEX);
    Serial.print(" ");
    Serial.println(v, HEX);
  }
}

void ram_get(byte *v)
{
  i2c_start();
  i2c_tx(ADDR);
  i2c_tx(0);

  i2c_start();
  i2c_tx(ADDR | 1);
  for (int i = 0; i < 128; i++) {
    *v++ = i2c_rx(i == 127);
    // Serial.println(v[-1], DEC);
  }
  i2c_stop();
}

static void load_flash(uint8_t *config)
{
    byte b[128];

    i2c_begin();
    ram_write(config);
    ram_get(b);
    int diff = memcmp(config, b, 128);
    if (diff != 0) {
      Serial.println("Flash fault");
      GD.Clear();
      GD.cmd_text(GD.w / 2, GD.h / 2, 30, OPT_CENTERX, "Flash fault");
      GD.swap();
      for (;;);
    }
    Serial.println("Flash verified OK");
    GD.begin(0);
}

void demo_sketch()
{
  GD.begin(~GD_STORAGE);

  GD.cmd_memset(0, 0x00, long(GD.w) * GD.h);     // clear the bitmap

  GD.Clear();                                 // draw the bitmap

  GD.ColorRGB(0x202030);
  GD.cmd_text(GD.w / 2, GD.h / 2, 31, OPT_CENTER, "sketch demo");

  GD.BitmapLayout(L8, GD.w, GD.h);
  GD.BitmapSize(NEAREST, BORDER, BORDER, GD.w, GD.h);

  GD.Begin(BITMAPS);
  GD.ColorRGB(0xffffff);
  GD.Vertex2ii(0, 0);
  GD.swap();
  GD.cmd_sketch(0, 0, GD.w, GD.h, 0, L8);     // start sketching
  GD.finish();                                // flush all commands
}

void gd3load(uint8_t *config)
{
  uint8_t stage[128];
  memcpy(stage, config, 128);

  GDTR.begin0();
  GDTR.begin1();
  GDTR.wr(REG_GPIO, GDTR.rd(REG_GPIO) | 0x80);
  GD.copyram(config + 4, 124);

  GD.Clear(); GD.swap();
  GD.Clear(); GD.swap();
  GD.Clear(); GD.swap();
  GD.cmd_regwrite(REG_PWM_DUTY, 128);

  GD.finish();


  GD.w = GD.rd16(REG_HSIZE);
  GD.h = GD.rd16(REG_VSIZE);
  GD.self_calibrate();
  GD.finish();

  for (int i = 0; i < 24; i++)
    stage[CALIBRATION_OFFSET + i] = GD.rd(REG_TOUCH_TRANSFORM_A + i);

  i2c_begin();
  ram_write(stage);
  ramdump();

  demo_sketch();
}

void setup()
{
  Serial.begin(1000000);
  gd3load(GD3_7__init);
}

void loop()
{
  // Serial.println(GD.rd16(REG_TOUCH_RAW_XY));
}
