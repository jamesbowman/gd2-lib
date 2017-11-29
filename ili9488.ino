#include <EEPROM.h> //' A{
#include <SPI.h>
#include <GD2.h>

#define CSX 6   // chip select, active low
#define DCX 2   // data/command select
#define SCL 3   // clock, rising edge
#define SDA 4   // data in to ILI9488
#define SDO 5   // data out from ILI9488

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

void loop()
{
  digitalWrite(DCX, LOW);
  out_8(0xda);  // RDID1

  Serial.println();
  digitalWrite(DCX, HIGH);
  digitalWrite(CSX, LOW);
  for (int i = 0; i < 8; i++) {
    digitalWrite(SCL, LOW);
    digitalWrite(SCL, HIGH);
    Serial.println(digitalRead(SDO), DEC);
  }
  digitalWrite(CSX, HIGH);

  delay(3000);
}
