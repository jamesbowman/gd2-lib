#include <EEPROM.h>
#include <SPI.h>
#include <GD2.h>

MoviePlayer mp;

class Dirsearch {
  struct dirent de;
  int index;
  
public:
  char name[13];
  void begin() {
    index = 0;
  }
  int get(const char *ext) {
    byte i;

    GD.__end();
    char e3[3];

    do {
      GD.SD.rdn((byte*)&de, GD.SD.o_root + index++ * 32, sizeof(de));
      for (i = 0; i < 3; i++)
        e3[i] = tolower(de.ext[i]);
    } while (de.name[0] &&
             ((de.name[0] & 0x80) || (memcmp(ext, e3, 3) != 0)));

    GD.resume();

    char *pc = name;
    for (i = 0; i < 8 && de.name[i] != ' '; i++)
      *pc++ = tolower(de.name[i]);
    *pc++ = '.';
    for (i = 0; i < 3 && de.ext[i] != ' '; i++)
      *pc++ = tolower(de.ext[i]);
    *pc++ = 0;
    Serial.println(de.name[0], HEX);
    Serial.println(name);

    return de.name[0];
  }
};
  
Dirsearch ds;

char *pickfile(char *ext)
{
  GD.Clear();
  ds.begin();
  for (byte i = 0; ds.get(ext); i++) {
    int x = (i % 3) * 160;
    int y = (i / 3) * 68;
    GD.Tag(i + 1);
    GD.cmd_button(x, y, 150, 60, 27, OPT_FLAT, ds.name);
  }
  GD.swap();

  do {
    GD.get_inputs();
  } while (!GD.inputs.tag);

  ds.begin();
  for (byte i = 0; i <= (GD.inputs.tag - 1); i++)
    ds.get(ext);
  Serial.println(GD.inputs.tag);
  Serial.println(ds.name);
  return ds.name;
}

void setup()
{
  Serial.begin(1000000); // JCB
  GD.begin();
}

void loop()
{
  char *avi = pickfile("avi");

  GD.Clear();
  GD.cmd_text(GD.w / 2, GD.h / 2 - 30, 31, OPT_CENTER, "playing");
  GD.cmd_text(GD.w / 2, GD.h / 2 + 30, 31, OPT_CENTER, avi);
  GD.swap();

  mp.begin(avi);
  mp.play();
}
