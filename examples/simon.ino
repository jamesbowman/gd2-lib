#include <EEPROM.h>
#include <SPI.h>
#include <GD2.h>

void setup()
{
  Serial.begin(1000000); // JCB
  GD.begin(~GD_STORAGE);
}

#define DARK_GREEN      0x007000  //' a{
#define LIGHT_GREEN     0x33ff33
#define DARK_RED        0x700000
#define LIGHT_RED       0xff3333
#define DARK_YELLOW     0x707000
#define LIGHT_YELLOW    0xffff33
#define DARK_BLUE       0x007070
#define LIGHT_BLUE      0x33ffff  //' }a

void drawscreen(int pressed)  //' b{
{
  GD.get_inputs();  //' touch{
  GD.Clear();       //' }touch

  GD.PointSize(16 * 60);  // 60-pixel radius points
  GD.Begin(POINTS);
  GD.Tag(1);
  if (pressed == 1)
    GD.ColorRGB(LIGHT_GREEN);
  else
    GD.ColorRGB(DARK_GREEN);
  GD.Vertex2ii(240 - 70, 136 - 70);

  GD.Tag(2);
  if (pressed == 2)
    GD.ColorRGB(LIGHT_RED);
  else
    GD.ColorRGB(DARK_RED);
  GD.Vertex2ii(240 + 70, 136 - 70);

  GD.Tag(3);
  if (pressed == 3)
    GD.ColorRGB(LIGHT_YELLOW);
  else
    GD.ColorRGB(DARK_YELLOW);
  GD.Vertex2ii(240 - 70, 136 + 70);

  GD.Tag(4);
  if (pressed == 4)
    GD.ColorRGB(LIGHT_BLUE);
  else
    GD.ColorRGB(DARK_BLUE);
  GD.Vertex2ii(240 + 70, 136 + 70);

  GD.swap();
} //' }b

void play(int pressed) //' play{
{
  //                 G   R   Y   B
  //                 E3  A4  C#4 E4
  byte note[] = { 0, 52, 69, 61, 64 };
  GD.play(BELL, note[pressed]);
  for (int i = 0; i < 30; i++)
    drawscreen(pressed);
  for (int i = 0; i < 15; i++)
    drawscreen(0);
} //' }play

static int get_note() //' getnote{
{
  byte pressed = 0;
  while (pressed == 0) {
    GD.random();
    drawscreen(0);
    if ((1 <= GD.inputs.tag) && (GD.inputs.tag <= 4))
      pressed = GD.inputs.tag;
#ifdef DUMPDEV    // JCB{
    pressed = 1;
#endif          // }JCB
  }
  play(pressed);
  return pressed;
} //' }getnote

static int random_note()
{
  return 1 + GD.random(4);
}

void loop() //' loop{
{
  int sequence[100];
  int length = 0;

  while (1) {
    delay(500);

    sequence[length++] = random_note();

    for (int i = 0; i < length; i++)
      play(sequence[i]);

    for (int i = 0; i < length; i++) {
      int pressed = get_note();
      if (pressed != sequence[i]) {
        for (int i = 69; i > 49; i--) {
          GD.play(BELL, i);
          delay(50);
        }
        return;
      }
    }
  }
} //' }loop
