#include <EEPROM.h> //' A{
#include <SPI.h>
#include <GD2.h>

#define CSX 6   // chip select, active low
#define DCX 2   // data/command select
#define SCL 3   // clock, rising edge
#define SDA 4   // data in to ILI9488
#define SDO 5   // data out from ILI9488

#define ILI9488_CMD_READ_ID1  0xDA
#define ILI9488_CMD_READ_ID2  0xDB
#define ILI9488_CMD_READ_ID3  0xDC

void setup()
{
  Serial.begin(1000000); // JCB
  // GD.begin(0);

  pinMode(CSX, OUTPUT);
  pinMode(DCX, OUTPUT);
  pinMode(SCL, OUTPUT);
  pinMode(SDA, OUTPUT);
  pinMode(SDO, INPUT);

  digitalWrite(CSX, HIGH);
}

void loop2()
{
  GD.ClearColorRGB(0x103000);
  GD.Clear();
  GD.cmd_text(GD.w / 2, GD.h / 2, 31, OPT_CENTER, "Hello world");
  GD.swap();
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

uint8_t rdreg(uint8_t reg)
{
  uint8_t r;

  digitalWrite(DCX, LOW);
  out_8(reg);

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

uint8_t wrreg(uint8_t reg, uint8_t val)
{
  digitalWrite(DCX, LOW);
  out_8(reg);
  digitalWrite(DCX, HIGH);
  out_8(val);
}

void loop()
{
  Serial.println();
  Serial.print("ID1: ");
  Serial.println(rdreg(ILI9488_CMD_READ_ID1), HEX);
  Serial.print("ID2: ");
  Serial.println(rdreg(ILI9488_CMD_READ_ID2), HEX);
  Serial.print("ID3: ");
  Serial.println(rdreg(ILI9488_CMD_READ_ID3), HEX);

  delay(3000);
}
