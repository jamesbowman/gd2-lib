#include <SPI.h>

int CS = 8;

static void hostcmd(byte a)
{
  digitalWrite(CS, LOW);
  SPI.transfer(a);
  SPI.transfer(0x00);
  SPI.transfer(0x00);
  digitalWrite(CS, HIGH);
  delay(200);
}

void setup()
{
  Serial.begin(115200);
  pinMode(CS, OUTPUT);
  digitalWrite(CS, HIGH);
  SPI.begin();
  SPI.setClockDivider(SPI_CLOCK_DIV128);

  Serial.println("SPI INIT DONE");

  hostcmd(0x00);  // wake up
  hostcmd(0x68);  // reset GPU
}

void loop()
{
  digitalWrite(CS, LOW);
  Serial.println();
  Serial.println(SPI.transfer(0x10), HEX);
  Serial.println(SPI.transfer(0x24), HEX);
  Serial.println(SPI.transfer(0x00), HEX);
  Serial.println(SPI.transfer(0xff), HEX);
  Serial.println(SPI.transfer(0xff), HEX);
  Serial.println(SPI.transfer(0xff), HEX);
  digitalWrite(CS, HIGH);
  delay(1000);
}
