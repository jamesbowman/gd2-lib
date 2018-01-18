#include <EEPROM.h>
#include <SPI.h>
#include <GD2.h>

#define DHTPIN  A0

#define RISE()   do {} while (digitalRead(DHTPIN) == 0)
#define FALL()   do {} while (digitalRead(DHTPIN) == 1)

void setup()
{
  Serial.begin(1000000);

  pinMode(DHTPIN, OUTPUT);
  digitalWrite(DHTPIN, HIGH);
  delay(1000);
}

int getbit()
{
  RISE();
  uint32_t t0 = micros();
  FALL();
  return (micros() - t0) > 48;
}

void loop()
{
  pinMode(DHTPIN, OUTPUT);
  digitalWrite(DHTPIN, LOW);
  delay(18);
  digitalWrite(DHTPIN, HIGH);
  delayMicroseconds(40);
  pinMode(DHTPIN, INPUT_PULLUP);

  FALL();
  getbit();

  int bits[40];
  for (int i = 0; i < 40; i++)
    bits[i] = getbit();
  for (int i = 0; i < 40; i++) {
    Serial.print(i);
    Serial.print(' ');
    Serial.println(bits[i]);
  }
  int bytes[5] = {0};
  for (int i = 0; i < 40; i++)
    bytes[i / 8] = (bytes[i / 8] << 1) | bits[i];
  for (int i = 0; i < 5; i++) {
    Serial.print(bytes[i]);
    Serial.print(' ');
  }
  Serial.println();

  delay(6000);
}
