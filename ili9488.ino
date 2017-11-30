#include <EEPROM.h>
#include <SPI.h>
#include <GD2.h>

#define CSX 6   // chip select, active low
#define DCX 5   // data/command select
#define SCL 13  // clock, rising edge
#define SDA 11  // data in to ILI9488
#define SDO 12  // data out from ILI9488

#define ILI9488_CMD_READ_ID1                    0xDA
#define ILI9488_CMD_READ_ID2                    0xDB
#define ILI9488_CMD_READ_ID3                    0xDC
#define ILI9488_CMD_READ_DISPLAY_IDENTIFICATION 0x04
#define ILI9488_CMD_READ_DISPLAY_POWERMODE      0x0A
#define ILI9488_CMD_READ_MADCTRL                0x0B
#define ILI9488_CMD_READ_PIXEL_FORMAT           0x0C
#define ILI9488_CMD_READ_DISPLAY_SIGNALMODE     0x0E

void setup()
{
  Serial.begin(1000000);

  pinMode(CSX, OUTPUT);
  digitalWrite(CSX, HIGH);

  Serial.println("GD init");
  gd_ili9488_init();

  if (1) {
    GD.__end();
    SPI.end();

    pinMode(DCX, OUTPUT);
    pinMode(SCL, OUTPUT);
    pinMode(SDA, OUTPUT);
    pinMode(SDO, INPUT);

    delay(100);

    ili9488_report();
    ili9488_rgb_mode();
    ili9488_report();
    SPI.begin();
    GD.resume();
  }

}

unsigned long t;

void loop()
{
  GD.ClearColorRGB(0x103000);
  GD.Clear();
  GD.cmd_text(GD.w / 2, GD.h / 2, 31, OPT_CENTER, "Hello world");
  GD.cmd_number(GD.w / 2, 3 * GD.h / 4, 31, OPT_CENTER, t++);
  GD.swap();

  /*
  uint32_t f0 = GD.rd32(REG_FRAMES);
  delay(1000);
  uint32_t f1 = GD.rd32(REG_FRAMES);
  Serial.println(f1 - f0);
  GD.finish();
  GD.__end();
  SPI.end();
  pinMode(DCX, OUTPUT);
  pinMode(SCL, OUTPUT);
  pinMode(SDA, OUTPUT);
  pinMode(SDO, INPUT);
  ili9488_report();
  SPI.begin();
  GD.resume();
  */
}

// Initialize GD for the ILI9488 
void gd_ili9488_init()
{
  GD.begin(~GD_STORAGE);
  Serial.println("Setting video timing for ILI9488");
  GD.wr16(REG_HCYCLE,   400 );
  GD.wr16(REG_HOFFSET,  40  );
  GD.wr16(REG_HSYNC0,   0   );
  GD.wr16(REG_HSYNC1,   10  );
  GD.wr16(REG_VCYCLE,   500 );
  GD.wr16(REG_VOFFSET,  10  );
  GD.wr16(REG_VSYNC0,   0   );
  GD.wr16(REG_VSYNC1,   5   );
  GD.wr16(REG_SWIZZLE,  2   );
  GD.wr16(REG_PCLK_POL, 1   );
  GD.wr16(REG_HSIZE,    320 );
  GD.wr16(REG_VSIZE,    480 );
  GD.wr16(REG_CSPREAD,  1   );
  GD.wr16(REG_DITHER,   1   );
  GD.wr16(REG_PCLK,     5   );
  GD.w = 320;
  GD.h = 480;
  Serial.print("ID: ");
  Serial.println(GD.rd(REG_ID), HEX);
}

void out_8(uint8_t b)
{
  digitalWrite(CSX, LOW);
  for (int i = 0; i < 8; i++) {
    digitalWrite(SCL, LOW);
    digitalWrite(SDA, (b >> 7) & 1);
    digitalWrite(SCL, HIGH);
    b <<= 1;
  }
  digitalWrite(CSX, HIGH);
}

void write_command(uint8_t b)
{
  digitalWrite(DCX, LOW);
  out_8(b);
}

void write_data(uint8_t b)
{
  digitalWrite(DCX, HIGH);
  out_8(b);
}

uint8_t rdreg(uint8_t reg)
{
  uint8_t r;

  write_command(reg);

  digitalWrite(DCX, HIGH);
  digitalWrite(CSX, LOW);
  for (int i = 0; i < 8; i++) {
    digitalWrite(SCL, LOW);
    digitalWrite(SCL, HIGH);
    r = (r << 1) | digitalRead(SDO);
  }
  digitalWrite(CSX, HIGH);
  return r;
}

static void ili9488_rgb_mode()
{
  write_command(0xE0);     //  positive gamma control
  write_data(0x00);
  write_data(0x04);
  write_data(0x0E);
  write_data(0x08);
  write_data(0x17);
  write_data(0x0A);
  write_data(0x40);
  write_data(0x79);
  write_data(0x4D);
  write_data(0x07);
  write_data(0x0E);
  write_data(0x0A);
  write_data(0x1A);
  write_data(0x1D);
  write_data(0x0F);

  write_command(0xE1);     //  Negative gamma control
  write_data(0x00);
  write_data(0x1B);
  write_data(0x1F);
  write_data(0x02);
  write_data(0x10);
  write_data(0x05);
  write_data(0x32);
  write_data(0x34);
  write_data(0x43);
  write_data(0x02);
  write_data(0x0A);
  write_data(0x09);
  write_data(0x33);
  write_data(0x37);
  write_data(0x0F);

  write_command(0xC0);     // power control 1
  write_data(0x18);
  write_data(0x16);

  write_command(0xC1);     // power control 2
  write_data(0x41);

  write_command(0xC5);     // vcom control
  write_data(0x00);
  write_data(0x1E);       // VCOM
  write_data(0x80);

  write_command(0x36);     // madctrl - memory access control
  write_data(0x48);       // bgr connection and colomn address order

  write_command(0x3A);     // Interface Mode Control
  write_data(0x66);       // 18BIT

  write_command(0xB1);     // Frame rate 60HZ
  write_data(0xB0);

  write_command(0xE9);     // set image function
  write_data(0x00);       // DB_EN off - 24 bit is off

  write_command(0xF7);     // adjust control 3
  write_data(0xA9);
  write_data(0x51);
  write_data(0x2C);
  write_data(0x82);

  write_command(0xB0);     // Interface Mode Control
  write_data(0x02);       // set DE,HS,VS,PCLK polarity

  write_command(0xB6);

  write_data(0x30);       // 30 set rgb
  write_data(0x02);       // GS,SS 02£¬42
  write_data(0x3B);

  write_command(0x2A);     // colomn address set
  write_data(0x00);
  write_data(0x00);
  write_data(0x01);
  write_data(0x3F);

  write_command(0x2B);     // Display function control
  write_data(0x00);
  write_data(0x00);
  write_data(0x01);
  write_data(0xDF);

  write_command(0x11);     // sleep out

  delay(120);

  write_command(0x29);     // display on
}

void ili9488_report()
{
  Serial.println();

  Serial.print("ID1: ");
  Serial.println(rdreg(ILI9488_CMD_READ_ID1), HEX);

  Serial.print("ID2: ");
  Serial.println(rdreg(ILI9488_CMD_READ_ID2), HEX);

  Serial.print("ID3: ");
  Serial.println(rdreg(ILI9488_CMD_READ_ID3), HEX);

  Serial.print("DISPLAY_IDENTIFICATION: ");
  Serial.println(rdreg(ILI9488_CMD_READ_DISPLAY_IDENTIFICATION), HEX);

  Serial.print("DISPLAY_POWERMODE: ");
  Serial.println(rdreg(ILI9488_CMD_READ_DISPLAY_POWERMODE), HEX);

  Serial.print("MADCTRL: ");
  Serial.println(rdreg(ILI9488_CMD_READ_MADCTRL), HEX);

  Serial.print("PIXEL_FORMAT: ");
  Serial.println(rdreg(ILI9488_CMD_READ_PIXEL_FORMAT), HEX);

  Serial.print("DISPLAY_SIGNALMODE: ");
  Serial.println(rdreg(ILI9488_CMD_READ_DISPLAY_SIGNALMODE), HEX);
}
