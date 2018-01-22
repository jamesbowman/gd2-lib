#include <EEPROM.h>
#include <SPI.h>
#include <GD2.h>

#define DHTPIN  A0

#define UNITS(C)  (((C) * 9) / 5 + 32)    // Convert from Celsius

#define RISE()   do {} while (digitalRead(DHTPIN) == 0)
#define FALL()   do {} while (digitalRead(DHTPIN) == 1)

void setup()
{
  GD.begin(~GD_STORAGE);
  GD.cmd_setrotate(3);

  dht_begin();
  // demo_data();
}

int cycle;

void loop()
{
  int T, H;

  dht_sense(T, H);
  update_graphs(T, H);

  for (int i = 0; i < 720; i++) {
    draw_graphs();
    delay(1000);
  }
}

///////////////////////////////////////////////////////////////////////

void dht_begin()
{
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

void dht_sense(int &temp, int &humidity)
{
  pinMode(DHTPIN, OUTPUT);
  digitalWrite(DHTPIN, LOW);
  delay(18);
  digitalWrite(DHTPIN, HIGH);
  delayMicroseconds(40);
  pinMode(DHTPIN, INPUT_PULLUP);

  FALL();
  getbit();

  int bytes[5] = {0};
  for (int i = 0; i < 40; i++)
    bytes[i / 8] = (bytes[i / 8] << 1) | getbit();
  humidity = bytes[0];
  temp = UNITS(bytes[2]);
}

///////////////////////////////////////////////////////////////////////

class Graph
{
public:
  int y;
  const char *name;
  int lo, hi;
  uint32_t c0, c1;
  int n;
  unsigned data[120];
  void update(int d) {
    for (int i = 0; i < 119; i++)
      data[i] = data[i + 1];
    data[119] = d;
    n = min(120, n + 1);
  }
  void draw() {
    int w = GD.w - 40;
    int h = 190;
    GD.SaveContext();

    GD.ScissorXY(0, y);
    GD.ScissorSize(w, h);
    GD.cmd_gradient(0, y, c0,
                    0, y + h, c1);
    GD.ColorA(128);
    GD.ColorRGB(0x000000);
    GD.cmd_text(5, y + 5, 30, 0, name);
    GD.RestoreContext();

    GD.LineWidth(PIXELS(0.875));
    GD.Begin(LINE_STRIP);
    xy p;
    for (int i = 120 - n; i < 120; i++) {
      p.set(map(i, 0, 120, 0, PIXELS(w)),
            map(data[i], lo, hi, PIXELS(y + h), PIXELS(y)));
      p.draw();
    }
    GD.PointSize(PIXELS(5));
    GD.Begin(POINTS);
    p.set(PIXELS(w),
          map(data[119], lo, hi, PIXELS(y + h), PIXELS(y)));
    p.draw();
    GD.cmd_number((w + GD.w) / 2, y + h / 2, 30, OPT_CENTER, data[119]);
  }
};

Graph Tg = { 80, "Temperature", UNITS(5), UNITS(45), 0x721817, 0x2b4162},
      Hg = {280, "Humidity",    0,        100,       0x0b6e4f, 0xfa9f42};

void update_graphs(int T, int H)
{
  Tg.update(T);
  Hg.update(H);
}

void demo_data()
{
  for (int i = 0; i < 120; i++) {
    Tg.data[i] = GD.random(UNITS(18), UNITS(23)) + GD.rsin(25, i * 300);
    Hg.data[i] = GD.random(30, 40) - GD.rsin(10, i * 300);
  }
  Tg.n = 120;
  Hg.n = 120;
}

void draw_graphs()
{
  GD.Clear();
  GD.cmd_gradient(0, 0, 0x101010, GD.w, GD.h, 0x202060);

  Tg.draw();
  Hg.draw();

  uint32_t s = millis() / 1000; //  + 91817UL;
  int m = s / 60;
  int h = m / 60;

  GD.ColorRGB(0xe0e0e2);
  GD.cmd_text(0, 20, 28, OPT_CENTERY, "Running");

  GD.cmd_number(130, 20, 30, OPT_CENTERY | OPT_RIGHTX, h);
  GD.cmd_text(  135, 20, 28, OPT_CENTERY, "h");

  GD.cmd_number(188, 20, 30, OPT_CENTERY | OPT_RIGHTX, m % 60);
  GD.cmd_text(  193, 20, 28, OPT_CENTERY, "m");

  GD.cmd_number(250, 20, 30, OPT_CENTERY | OPT_RIGHTX, s % 60);
  GD.cmd_text(  255, 20, 28, OPT_CENTERY, "s");

  GD.swap();
}
