#include <EEPROM.h>
#include <SPI.h>
#include <GD2.h>

static uint16_t value = 15000;      // every widget is hooked to this value
static char message[41];            // 40 character text entry field
static uint16_t options = OPT_FLAT;
static byte prevkey;

void setup()
{
  memset(message, 7, 40);
  GD.begin(~GD_STORAGE);
}

#define TAG_DIAL      200
#define TAG_SLIDER    201
#define TAG_TOGGLE    202
#define TAG_BUTTON1   203
#define TAG_BUTTON2   204

void loop()
{

  GD.get_inputs();

  switch (GD.inputs.track_tag & 0xff) {   //' track{
  case TAG_DIAL:
  case TAG_SLIDER:
  case TAG_TOGGLE:
    value = GD.inputs.track_val;
  } //' }track
  switch (GD.inputs.tag) {
  case TAG_BUTTON1:
    options = OPT_FLAT;
    break;
  case TAG_BUTTON2:
    options = 0;
    break;
  }
  byte key = GD.inputs.tag;
  if ((prevkey == 0x00) && (' ' <= key) && (key < 0x7f)) {
    memmove(message, message + 1, 39);
    message[39] = key;
  }
  prevkey = key;

  GD.cmd_gradient(0, 0,   0x404044,
                  480, 480, 0x606068);
  GD.ColorRGB(0x707070);

  GD.LineWidth(4 * 16);
  GD.Begin(RECTS);

  GD.Vertex2ii(8, 8);
  GD.Vertex2ii(128, 128);

  GD.Vertex2ii(8, 136 + 8);
  GD.Vertex2ii(128, 136 + 128);

  GD.Vertex2ii(144, 136 + 8);
  GD.Vertex2ii(472, 136 + 128);
  GD.ColorRGB(0xffffff);

  GD.Tag(TAG_DIAL); //' dial{
  GD.cmd_dial(68, 68, 50, options, value);
  GD.cmd_track(68, 68, 1, 1, TAG_DIAL);   //' }dial

  GD.Tag(TAG_SLIDER); //' linear{
  GD.cmd_slider(16, 199, 104, 10, options, value, 65535);
  GD.cmd_track(16, 199, 104, 10, TAG_SLIDER);

  GD.Tag(TAG_TOGGLE);
  GD.cmd_toggle(360, 62, 80, 29, options, value,
                "that" "\xff" "this");
  GD.cmd_track(360, 62, 80, 20, TAG_TOGGLE); //' }linear

  GD.Tag(255);
  GD.cmd_number(68, 136, 30, OPT_CENTER | 5, value);

  GD.cmd_clock(184, 48, 40, options | OPT_NOSECS, 0, 0, value, 0);
  GD.cmd_gauge(280, 48, 40, options, 4, 3, value, 65535);

  GD.Tag(TAG_BUTTON1);
  GD.cmd_button(352, 12, 40, 30, 28, options,  "2D");
  GD.Tag(TAG_BUTTON2);
  GD.cmd_button(400, 12, 40, 30, 28, options,  "3D");

  GD.Tag(255);
  GD.cmd_progress(144, 100, 320, 10, options, value, 65535);
  GD.cmd_scrollbar(144, 120, 320, 10, options, value / 2, 32768, 65535);

  GD.cmd_keys(144, 168,      320, 24, 28, options | OPT_CENTER | key, "qwertyuiop");
  GD.cmd_keys(144, 168 + 26, 320, 24, 28, options | OPT_CENTER | key,   "asdfghjkl");
  GD.cmd_keys(144, 168 + 52, 320, 24, 28, options | OPT_CENTER | key,   "zxcvbnm,.");
  GD.Tag(' ');
  GD.cmd_button(308 - 60, 172 + 74, 120, 20, 28, options,   "");

  GD.BlendFunc(SRC_ALPHA, ZERO);
  GD.cmd_text(149, 146, 18, 0, message);

  GD.swap();
}
