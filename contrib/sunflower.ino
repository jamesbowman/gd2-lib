#include <EEPROM.h>
#include <SPI.h>
#include <GD2.h>

void setup()
{
  pinMode(3, OUTPUT);
  digitalWrite(3, HIGH);

  GD.begin(~GD_STORAGE);
  GD.cmd_romfont(31, 34);
}

// in 1/4 degrees C. See MAX31855 datasheet
int read_temperature(bool &fault)
{
  union {
    int16_t t;
    uint8_t b[4];
  };
  digitalWrite(3, LOW);
  b[1] = SPI.transfer(0xff);
  b[0] = SPI.transfer(0xff);
  digitalWrite(3, HIGH);
  fault = (t & 1) != 0;
  return t >> 2;
}

int prev_touching;
int celsius;

void loop()
{
  bool fault;
  int temp;

  GD.__end();
  int t4 = read_temperature(fault);
  GD.resume();

  if (celsius)
    temp = t4 / 4;
  else
    temp = t4 * 9 / (5 * 4) + 32;
  GD.get_inputs();

  GD.ClearColorRGB(0x103000);
  GD.Clear();
  GD.cmd_number(170, GD.h / 2, 31, OPT_CENTERY | OPT_RIGHTX, temp);
  GD.cmd_text  (170, GD.h / 2, 31, OPT_CENTERY, celsius ? " C" : " F");
  GD.swap();
  GD.finish();

  if (GD.inputs.touching && !prev_touching)
    celsius ^= 1;
  prev_touching = GD.inputs.touching;
}
