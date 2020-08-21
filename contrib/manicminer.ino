#include <EEPROM.h>
#include <SPI.h>
#include <GD2.h>

int dazzler = 1;
int SF = 3;
int X0 = 112;
int Y0 = 32;

int OVER_FRAMES = 1; // 5

struct level {
  char name[33];
  uint32_t border;
  struct { byte x, y; } items[5];
  byte air;
  byte conveyordir;
  byte portalx, portaly;
  struct {
    byte a, x, y, d, x0, x1; } hguard[8];
  byte wx, wy, wd, wf;
  byte bidir;
};

struct guardian {
  byte a;
  byte x, y;
  signed char d;
  byte x0, x1;
  byte f;
};

char debug[80];

#define CHEAT_INVINCIBLE    0   // means Willy unaffected by nasties
#define START_LEVEL         0   // level to start on, 0-18
#define CHEAT_OPEN_PORTAL   0   // Portal always open

// Game has three controls: LEFT, RIGHT, JUMP.  You can map them
// to any pins by changing the definitions of PIN_L, PIN_R, PIN_J
// below.

#define CONTROL_LEFT      1
#define CONTROL_RIGHT     2
#define CONTROL_JUMP      4

int control(void) {
  int r = 0;

  if (!dazzler) {
    r = GD.inputs.tag;
  } else {
    uint16_t b = ~GD.inputs.wii[0].buttons;
    if (b & WII_LEFT)
      r |= CONTROL_LEFT;
    if (b & WII_RIGHT)
      r |= CONTROL_RIGHT;
    if (b & WII_A)
      r |= CONTROL_JUMP;
  }
  return r;
}

// The basic colors
#define BLACK   RGB(0,   0,   0)
#define RED     RGB(255, 0,   0)
#define GREEN   RGB(0,   255, 0)
#define YELLOW  RGB(255, 255, 0)
#define BLUE    RGB(0,   0,   255)
#define MAGENTA RGB(255, 0,   255)
#define CYAN    RGB(0,   255, 255)
#define WHITE   RGB(255, 255, 255)

// Convert a spectrum attribute byte to a 24-bit RGB
static uint32_t attr(byte a)
{
  // bit 6 indicates bright version
  byte l = (a & 64) ? 0xff : 0xaa;
  return RGB(
    ((a & 2) ? l : 0),
    ((a & 4) ? l : 0),
    ((a & 1) ? l : 0));
}

#define HANDLE_WILLY    0
#define HANDLE_SPECIAL  6

// The map's blocks have fixed meanings:
#define ELEM_AIR      0
#define ELEM_FLOOR    1
#define ELEM_CRUMBLE  2
#define ELEM_WALL     3
#define ELEM_CONVEYOR 4
#define ELEM_NASTY1   5
#define ELEM_NASTY2   6

#include "manicminer_assets.h"

#define MAXRAY 40

static struct state_t {
  byte level;
  byte lives;
  byte alive;
  byte t;
  uint32_t score;
  uint32_t hiscore;

  byte bidir;
  byte air;
  byte conveyordir;
  byte conveyor0[16];
  byte conveyor1[16];
  byte portalattr;
  byte nitems;
  byte items[5];
  byte wx, wy;    // Willy x,y
  byte wd, wf;    // Willy dir and frame
  byte lastdx;    // Willy last x movement
  byte convey;    // Willy caught on conveyor
  byte jumping;
  signed char wyv;
  byte conveyor[2];
  PROGMEM prog_uchar *guardian;
  uint16_t prevray[MAXRAY];
  byte switch1, switch2;
} state;

static struct guardian guards[8];

static void screen2ii(byte x, byte y, byte handle = 0, byte cell = 0)
{
  GD.BitmapHandle(handle);
  GD.Cell(cell);
  GD.Vertex2f(8 * x, 8 * y);
}

static void spectrum(byte x, byte y)
{
  GD.Vertex2f(8 * x, 8 * y);
}

static void qvertex(byte x, byte y, byte handle = 0, byte cell = 0)
{
  if ((x & ~511) | (y & ~511)) {
    GD.Cell(cell);
    GD.Vertex2f(8 * x, 8 * y);
  } else {
    GD.Vertex2ii(x, y, handle, cell);
  }
}

static void bump_score(byte n)
{
  uint32_t old_score = state.score;
  state.score += n;

  // Extra life on crossing 10K
  if ((old_score < 10000) && (10000 <= state.score))
    state.lives++;
}

static void load_level(void)
{
  const PROGMEM level *l = &levels[state.level];

  // the items are 5 sprites, using colors 16 up
  state.nitems = 0;
  for (byte i = 0; i < 5; i++) {
    byte x = pgm_read_byte_near(&l->items[i].x);
    byte y = pgm_read_byte_near(&l->items[i].y);
    byte isitem = (x || y);
    state.items[i] = isitem;
    state.nitems += isitem;
  }

  state.air = pgm_read_byte_near(&l->air);

  // Conveyor direction
  state.conveyordir = pgm_read_byte_near(&l->conveyordir);

  uint32_t level_chars = TILES_MEM + 15 * 64 * state.level;
  GD.rd_n(state.conveyor0, level_chars + 4 * 64, 8);
  memcpy(state.conveyor0 + 8, state.conveyor0, 8);
  GD.rd_n(state.conveyor1, level_chars + 4 * 64 + 3 * 8 , 8);
  memcpy(state.conveyor1 + 8, state.conveyor1, 8);

  // the hguardians
  state.bidir = pgm_read_byte_near(&l->bidir);
  guardian *pg;
  for (byte i = 0; i < 8; i++) {
    byte a = pgm_read_byte_near(&l->hguard[i].a);
    guards[i].a = a;
    if (a) {
      byte x = pgm_read_byte_near(&l->hguard[i].x);
      byte y = pgm_read_byte_near(&l->hguard[i].y);
      guards[i].x = x;
      guards[i].y = y;
      guards[i].d = pgm_read_byte_near(&l->hguard[i].d);
      guards[i].x0 = pgm_read_byte_near(&l->hguard[i].x0);
      guards[i].x1 = pgm_read_byte_near(&l->hguard[i].x1);
      guards[i].f = 0;
    }
  }

  pg = &guards[4];  // Use slot 4 for special guardians
  switch (state.level) { // Special level handling
  case 4: // Eugene's lair
    pg->a = 0;  // prevent normal guardian logic
    pg->x = 120;
    pg->y = 0;
    pg->d = -1;
    pg->x0 = 0;
    pg->x1 = 88;
    // loadspr16(IMG_GUARD + 4, eugene, 255, 15);
    break;
  case 7:   // Miner Willy meets the Kong Beast
  case 11:  // Return of the Alien Kong Beast
    pg->a = 0;
    pg->x = 120;
    pg->y = 0;
    state.switch1 = 0;
    // loadspr8(IMG_SWITCH1, lightswitch, 0, 6);
    // sprite(IMG_SWITCH1, 48, 0, IMG_SWITCH1);
    state.switch2 = 0;
    // loadspr8(IMG_SWITCH2, lightswitch, 0, 6);
    // sprite(IMG_SWITCH2, 144, 0, IMG_SWITCH2);
    break;
  }

  for (byte i = 0; i < MAXRAY; i++)
    state.prevray[i] = 4095;

  // Willy
  state.wx = pgm_read_byte_near(&l->wx);
  state.wy = pgm_read_byte_near(&l->wy);
  state.wf = pgm_read_byte_near(&l->wf);
  state.wd = pgm_read_byte_near(&l->wd);
  state.jumping = 0;
  state.convey = 0;
  state.wyv = 0;
}

static uint32_t level_base()
{
  return MANICMINER_ASSET_MAPS + (state.level << 9);
}

static void draw_level(void)
{
  const PROGMEM level *l = &levels[state.level];

  GD.ClearColorRGB(pgm_read_dword(&(l->border)));
  GD.Clear();
  GD.VertexTranslateX(16 * X0);
  GD.VertexTranslateY(16 * 32);
  GD.VertexFormat(3);

  // GD.cmd_clock(240, 50, 30, 0, 10, 0, 33, 100);
  // GD.cmd_number(240, 136, 31U, OPT_CENTER, 47U);
  // GD.cmd_slider(10, 250, 400, 10, 0, 66, 100);
  // GD.cmd_regwrite(0, 0x55aa);

  GD.Tag(CONTROL_JUMP);

  GD.Begin(BITMAPS);
  GD.cmd_loadidentity();
  GD.cmd_scale(F16(sf), F16(sf));
  GD.cmd_setmatrix();

  {
    GD.BitmapHandle(TILES_HANDLE);
    GD.BitmapSource(TILES_MEM + 15 * 64 * state.level);
    uint32_t pmap = level_base();
    for (byte y = 0; y < 16; y++) {
      byte x;
      byte line[32];

      GD.rd_n(line, pmap, 32);
      pmap += 32;

      for (x = 0; x < 32; x++)
        if (line[x])
          qvertex(8 * x, 8 * y, TILES_HANDLE, line[x]);
    }
  }

  // the portal is a sprite
  byte portalx = pgm_read_byte_near(&l->portalx);
  byte portaly = pgm_read_byte_near(&l->portaly);
  // flash it when it is open
  if (CHEAT_OPEN_PORTAL || state.nitems == 0) {
    byte lum = state.t << 5;
    GD.ColorRGB(lum, lum, lum);
  }
  screen2ii(portalx, portaly, PORTALS_HANDLE, state.level);

  // the items are 5 sprites
  uint32_t colors[4] = { MAGENTA, YELLOW, CYAN, GREEN };
  for (byte i = 0; i < 5; i++) 
    if (state.items[i]) {
      byte x = pgm_read_byte_near(&l->items[i].x);
      byte y = pgm_read_byte_near(&l->items[i].y);
      GD.ColorRGB(colors[(i + state.t) & 3]);
      screen2ii(x, y, 4, state.level);
    }
}

static void draw_status(byte playing)
{
  const level *l = &levels[state.level];

  uint16_t x = 0;
  uint16_t y = 128;

  GD.Begin(RECTS);
  GD.ColorRGB(BLACK);
  GD.Vertex2ii(x, y);
  GD.Vertex2ii(x + 256, y + 84);

  GD.ColorRGB(0xaaaa00);
  GD.Vertex2ii(x, y);
  GD.Vertex2ii(x + 256, y + 15);
  GD.ColorRGB(0xa00000);
  GD.Vertex2ii(x, y + 16);
  GD.Vertex2ii(x + 64, y + 30);
  GD.ColorRGB(0x00a000);
  GD.Vertex2ii(x + 64, y + 16);
  GD.Vertex2ii(x + 256, y + 30);

  if (playing) {
    GD.ColorRGB(BLACK);
    char nm[33];
    strcpy_P(nm, l->name);
    GD.cmd_text(128, y + 7, 26, OPT_CENTER, nm);

    GD.ColorRGB(WHITE);
    GD.cmd_text(x + 26, y + 23, 26, OPT_CENTERY | OPT_RIGHTX, "AIR");

    uint16_t air_x0 = x + 4 * 8;
    uint16_t air_x1 = x + 4 * 8 + state.air;
    uint16_t air_y = y + 23;

    GD.Begin(LINES);
    GD.LineWidth(64);
    GD.ColorRGB(BLACK);
    GD.ColorA(128);
    GD.Vertex2ii(air_x0, air_y);
    GD.Vertex2ii(air_x1, air_y);

    GD.LineWidth(32);
    GD.ColorRGB(WHITE);
    GD.ColorA(255);
    GD.Vertex2ii(air_x0, air_y);
    GD.Vertex2ii(air_x1, air_y);
  }

  GD.ColorRGB(YELLOW);
  GD.cmd_text(x, y + 48, 26, OPT_CENTERY, "High Score");
  GD.cmd_text(x + 165, y + 48, 26, OPT_CENTERY, "Score");
  GD.cmd_number(x + 75, y + 48, 26, OPT_CENTERY | 6, state.hiscore);
  GD.cmd_number(x + 255, y + 48, 26, OPT_RIGHTX | OPT_CENTERY | 6, state.score);

  if (playing) {
    GD.ColorRGB(CYAN);
    GD.Begin(BITMAPS);
    for (int i = 0; i < (state.lives - 1); i++) {
      screen2ii(2 + i * 16, 190, HANDLE_WILLY, 3 & (state.t >> 2));
    }
  }
}

static int qsin(byte r, byte a)
{
  byte t;
  switch (a & 3) {
  default:
  case 0:
    t = 0;
    break;
  case 1:
  case 3:
    t = (45 * r) >> 6;
    break;
  case 2:
    t = r;
  }
  return (a & 4) ? -t : t;
}

static void polar(int x, int y, byte r, byte a)
{
  GD.Vertex2ii(x + qsin(r, a), y + qsin(r, a + 2));
}

static void draw_button(int x, int y, byte angle)
{
  int r = 30;
  GD.PointSize(42 * 16);
  GD.ColorRGB(0x808080);
  GD.Begin(POINTS);
  GD.Vertex2ii(x, y);
  GD.ColorRGB(WHITE);
  GD.LineWidth(6 * 16);
  GD.Begin(LINES);
  polar(x, y, r, angle + 2);
  polar(x, y, r, angle + 6);
  GD.Begin(LINE_STRIP);
  polar(x, y, r, angle + 0);
  polar(x, y, r, angle + 2);
  polar(x, y, r, angle + 4);
}

static void draw_controls()
{
  if (dazzler)
    return;
  GD.Tag(CONTROL_LEFT);
  draw_button(      45, 272 - 45,  4);

  GD.Tag(CONTROL_LEFT | CONTROL_JUMP);
  draw_button(      45, 272 - 140, 3);

  GD.Tag(CONTROL_RIGHT);
  draw_button(480 - 45, 272 - 45,  0);

  GD.Tag(CONTROL_RIGHT | CONTROL_JUMP);
  draw_button(480 - 45, 272 - 140, 1);
}

static void draw_willy()
{
  byte frame = state.wf ^ (state.wd ? 7 : 0);
  GD.ColorRGB(WHITE);
  GD.Begin(BITMAPS);
  screen2ii(state.wx, state.wy, HANDLE_WILLY, frame);
}

void setup()
{
  GD.begin(~GD_STORAGE);

  LOAD_ASSETS();

  // Handle     Graphic
  //   0        Miner Willy
  //   1        Guardians
  //   2        Background tiles
  //   3        Portals
  //   4        Items
  //   5        Title screen
  //   6        Specials: Eugene, Plinth and Boot

  GD.Clear();
  GD.swap();

  sf = 2;
}

static void draw_guardians(void)
{
  GD.BitmapHandle(GUARDIANS_HANDLE);
  GD.BitmapSource(GUARDIANS_MEM + 8 * 32 * state.level);

  guardian *pg;
  for (byte i = 0; i < 8; i++) {
    pg = &guards[i];
    if (pg->a) {
      GD.ColorRGB(attr(pg->a));
      screen2ii(pg->x, pg->y, GUARDIANS_HANDLE, pg->f);
    }
  }
  pg = &guards[4];
  switch (state.level) { // Special level handling
  case 4: // Eugene's lair
    GD.ColorRGB(WHITE);
    screen2ii(pg->x, pg->y, 6, 0);
  }
}

static void move_guardians(void)
{
  guardian *pg;
  byte lt;  // local time, for slow hguardians
  for (byte i = 0; i < 8; i++) {
    pg = &guards[i];
    if (pg->a) {
      byte vertical = (i >= 4);
      byte frame;
      switch (state.level) {
      case 13:  // Skylabs
        if (pg->y != pg->x1) {
          pg->f = 0;
          pg->y += pg->d;
        } else {
          pg->f++;
        }
        if (pg->f == 8) {
          pg->f = 0;
          pg->x += 64;
          pg->y = pg->x0;
        }
        // Color is pg->a
        screen2ii(pg->x, pg->y, 1, pg->f);
        break;
      default:
        lt = state.t;
        if (!vertical && (pg->a & 0x80)) {
          if (state.t & 1)
            lt = state.t >> 1;
          else
            break;
        }

        if (!vertical) {
          if ((lt & 3) == 0) {
            if (pg->x == pg->x0 && pg->d)
              pg->d = 0;
            else if (pg->x == pg->x1 && !pg->d)
              pg->d = 1;
            else
              pg->x += pg->d ? -8 : 8;
          }
        } else {
          if (pg->y <= pg->x0 && pg->d < 0)
            pg->d = -pg->d;
          else if (pg->y >= pg->x1 && pg->d > 0)
            pg->d = -pg->d;
          else
            pg->y += pg->d;
        }

        if (state.bidir)
          frame = (lt & 3) ^ (pg->d ? 7 : 0);
        else
          if (vertical)
            frame = lt & 3;
          else
            frame = 4 ^ (lt & 3) ^ (pg->d ? 3 : 0);
        pg->f = frame;
      }
    }
  }
  pg = &guards[4];
  switch (state.level) { // Special level handling
  case 4: // Eugene's lair
    // sprite(IMG_GUARD + 4, pg->x, pg->y, IMG_GUARD + 4);
    if (pg->y == pg->x0 && pg->d < 0)
      pg->d = 1;
    if (pg->y == pg->x1 && pg->d > 0)
      pg->d = -1;
    if (state.nitems == 0) {  // all collected -> descend and camp
      if (pg->d == -1)
        pg->d = 1;
      if (pg->y == pg->x1)
        pg->d = 0;
    }
    pg->y += pg->d;
    break;
  case 7: // Miner Willy meets the Kong Beast
  case 11: //  Return of the Alien Kong Beast
    byte frame, color;
    if (!state.switch2) {
      frame = (state.t >> 3) & 1;
      color = 8 + 4;
    } else {
      frame = 2 + ((state.t >> 1) & 1);
      color = 8 + 6;
      if (pg->y < 104) {
        pg->y += 4;
        bump_score(100);
      }
    }
    // if (pg->y != 104) {
    //   loadspr16(IMG_GUARD + 4, state.guardian + (frame << 5), 255, color);
    //   sprite(IMG_GUARD + 4, pg->x, pg->y, IMG_GUARD + 4);
    // } else {
    //   hide(IMG_GUARD + 4);
    // }
  }
}

uint32_t atxy(byte x, byte y)
{
  return level_base() + (y << 5) + x;
}

// crumble block at s, which is the sequence
// 2 -> 8 -> 9 -> 10 -> 11 -> 12 -> 13 -> 14 -> AIR

static void crumble(uint32_t s)
{
  byte r = GD.rd(s);
  signed char nexts[] = {
    -1, -1,  8, -1, -1, -1, -1, -1,
    9,  10, 11, 12, 13, 14, ELEM_AIR };
  if (r < sizeof(nexts) && nexts[r] != -1)
    GD.wr(s, nexts[r]);
}

// is it legal for willy to be at (x,y)
static int canbe(byte x, byte y)
{
  uint32_t addr = atxy(x / 8, y / 8);
  return
    (GD.rd(addr) != ELEM_WALL) &&
    (GD.rd(addr+1) != ELEM_WALL) &&
    (GD.rd(addr+32) != ELEM_WALL) &&
    (GD.rd(addr+33) != ELEM_WALL);
}

// is Willy standing in a solar ray?
static int inray(byte x, byte y)
{
  uint32_t addr = atxy(x / 8, y / 8);
  return
    (GD.rd(addr) > 0x80 ) ||
    (GD.rd(addr+1) > 0x80) ||
    (GD.rd(addr+32) > 0x80) ||
    (GD.rd(addr+33) > 0x80);
}

static void under_willy(byte &a, byte &b, byte &c, byte &d)
{
  uint32_t addr = atxy(state.wx / 8, state.wy / 8);
  a = GD.rd(addr);
  b = GD.rd(addr+1);
  c = GD.rd(addr+32);
  d = GD.rd(addr+33);
}

static byte any(byte code, byte a, byte b, byte c, byte d)
{
  return ((a == code) |
          (b == code) |
          (c == code) |
          (d == code));
}

static byte collide_16x16(byte x, byte y)
{
  return (abs(state.wx - x) < 7) &&
         (abs(state.wy - y) < 15);
}

static void move_all(void)
{
  state.t++;
  move_guardians();

  // Animate conveyors: 
  byte conveyor_offset = (7 & state.t) ^ (state.conveyordir ? 0 : 7);
  uint32_t level_chars = TILES_MEM + 15 * 64 * state.level;
  GD.wr_n(level_chars + 4 * 64, state.conveyor0 + conveyor_offset, 8);
  GD.wr_n(level_chars + 4 * 64 + 3 * 8, state.conveyor1 + (7 ^ conveyor_offset), 8);

  // Willy movement
  // See http://www.seasip.demon.co.uk/Jsw/manic.mac
  // and http://jetsetwilly.jodi.org/poke.html

  byte con = control();
  byte ychanged = 0;  // if Y block changes must check for landing later
  if (state.jumping) {
#define JUMP_APEX 9   
#define JUMP_FALL 11  //  1   2   3   4   5   6   7   8   9  10 11
    signed char moves[] = {  -4, -4, -3, -3, -2, -2, -1, -1, 0, 0, 1, 1, 2, 2, 3, 3, 4 };
    byte index = min(sizeof(moves) - 1, state.jumping - 1);
    byte newy = state.wy + moves[index];
    state.jumping++;
    if (canbe(state.wx, newy)) {
      ychanged = (state.wy >> 3) != (newy >> 3);
      state.wy = newy;
    } else {
      state.jumping = max(state.jumping, JUMP_FALL);   // halt ascent
      ychanged = 1;
    }
    // sprintf(debug, "jumping=%d index=%d (%d,%d)", state.jumping, index, state.wx, state.wy);
  }
  uint32_t feet_addr = atxy(state.wx >> 3, (state.wy + 16) >> 3);
  byte elem0 = GD.rd(feet_addr);
  byte elem1 = GD.rd(feet_addr + 1);
  byte elem = ((1 <= elem0) && (elem0 <= 31)) ? elem0 : elem1;

  byte dx = 0xff;
  byte first_jump = (con & CONTROL_JUMP) && state.lastdx == 0xff;
  if (state.jumping) {
    dx = state.lastdx;
  } else if (!first_jump && (elem == ELEM_CONVEYOR) && (state.wd == state.conveyordir)) {
    dx = state.conveyordir;
  } else {
    if (con & CONTROL_RIGHT) {
      if (state.wd != 0) {
        state.wf ^= 3;
        state.wd = 0;
      } else {
        dx = 0;
      }
    } else if (con & CONTROL_LEFT) {
      if (state.wd == 0) {
        state.wf ^= 3;
        state.wd = 1;
      } else {
        dx = 1;
      }
    }
  }
  if (dx != 0xff) {
    if (state.wf != 3)
      state.wf++;
    else {
      byte newx = state.wx + (dx ? -8 : 8);
      if (canbe(newx, state.wy)) {
        state.wf = 0;
        state.wx = newx;
      }
    }
  }
  state.lastdx = dx;

  if ((elem == ELEM_CONVEYOR) && (dx == 0xff) && !state.jumping)
    state.wd = state.conveyordir;

  if (!state.jumping && (con & CONTROL_JUMP)) {
    if (canbe(state.wx, state.wy - 3)) {
      state.jumping = 1;
      state.wy -= 0;
    }
  }

  byte onground = ((1 <= elem) && (elem <= 4)) || ((7 <= elem) && (elem <= 16));
  // sprintf(debug, "jumping=%d elem=%d onground=%d", state.jumping, elem, onground);
  if (state.jumping) {
    if ((JUMP_APEX <= state.jumping) && ychanged && onground) {
      state.jumping = 0;
      state.wy &= ~7;
    }
    if (state.jumping >= 19)    // stop x movement late in the jump
      state.lastdx = 0xff;
  } else {
    if (!onground) {    // nothing to stand on, start falling
      state.jumping = JUMP_FALL;
      state.lastdx = 0xff;
    }
  }
  if (!state.jumping) {
    crumble(feet_addr);
    crumble(feet_addr + 1);
  }

  if (((state.t & 7) == 0) ||
     ((state.level == 18) && inray(state.wx, state.wy))) {
    state.air--;
    if (state.air == 0) {
      state.alive = 0;
    }
  }

  byte a, b, c, d;
  under_willy(a, b, c, d);
  if (any(ELEM_NASTY1, a, b, c, d) | any(ELEM_NASTY2, a, b, c, d))
    state.alive = 0;

  guardian *pg;
  for (byte i = 0; i < 8; i++) {
    pg = &guards[i];
    if (pg->a && collide_16x16(pg->x, pg->y))
      state.alive = 0;
  }

  state.alive |= CHEAT_INVINCIBLE;

  byte wx = state.wx / 8;
  byte wy = state.wy / 8;

  const PROGMEM level *l = &levels[state.level];
  for (byte i = 0; i < 5; i++) 
    if (state.items[i]) {
      byte x = pgm_read_byte_near(&l->items[i].x) / 8;
      byte y = pgm_read_byte_near(&l->items[i].y) / 8;
      if (((x == wx) || (x == (wx + 1))) &&
          ((y == wy) || (y == (wy + 1)))) {
        bump_score(100);
        state.items[i] = 0;
        --state.nitems;
      }
    }

  byte portalx = pgm_read_byte_near(&l->portalx);
  byte portaly = pgm_read_byte_near(&l->portaly);
  if (((CHEAT_OPEN_PORTAL || state.nitems == 0)) &&
      collide_16x16(portalx, portaly)) {
    while (state.air) {
      // squarewave(0, 800 + 2 * state.air, 100);
      state.air--;
      bump_score(7);
      draw_level();
      draw_willy();
      draw_guardians();
      draw_status(1);
      draw_controls();
      GD.swap();
    }
    state.level = (state.level + 1) % 18;
    load_level();
  }
}

static void draw_all(void)
{
  byte midi = pgm_read_byte_near(manicminer_tune + ((state.t >> 1) & 63));
  GD.cmd_regwrite(REG_SOUND, 0x01 | (midi << 8));
  GD.cmd_regwrite(REG_PLAY, 1);
  GD.get_inputs();
  for (int j = 0; j < OVER_FRAMES; j++) {
    draw_level();
    draw_willy();
    draw_guardians();
    draw_status(1);
    draw_controls();
    GD.swap();
    GD.cmd_regwrite(REG_SOUND, 0);
    GD.cmd_regwrite(REG_PLAY, 1);
  }
}

static void game_over()
{
  for (byte i = 0; i <= 96; i++) {
    if (i & 1) {
      GD.cmd_regwrite(REG_SOUND, 0x01 | ((40 + (i / 2)) << 8));
    } else {
      GD.cmd_regwrite(REG_SOUND, 0);
    }
    GD.cmd_regwrite(REG_PLAY, 1);

    GD.Clear();
    GD.ClearColorRGB(attr(9 + GD.random(7)));
    GD.ScissorSize(256, 128);
    GD.ScissorXY(X0, Y0);
    GD.Clear();

    if (i == 96)
      GD.cmd_text(122, 64, 29, OPT_CENTER, "GAME    OVER");

    GD.LineWidth(16 * 4);                           // Leg
    GD.Begin(LINES);
    screen2ii(125, 0);
    screen2ii(125, i);

    GD.Begin(BITMAPS);
    screen2ii(120, 128 - 16, HANDLE_SPECIAL, 1);    // Plinth

    screen2ii(120, i, HANDLE_SPECIAL, 2);           // Boot

    // Scissor so that boot covers up Willy
    GD.ScissorSize(480, 511);
    GD.ScissorXY(0, i + 16 + Y0);
    screen2ii(124, 128 - Y0, HANDLE_WILLY, 0);      // Willy
    GD.RestoreContext();

    draw_status(1);
    GD.swap();
  }
}

static const char marquee[] =
"MANIC MINER . . "
"(C) BUG-BYTE ltd. 1983 . . "
"By Matthew Smith . . "
"Gameduino2 conversion by James Bowman . . "
"Guide Miner Willy through 19 lethal caverns";

int touch(void)
{
  if (dazzler)
    return (GD.inputs.wii[0].buttons != 0xffff);
  else
    return GD.inputs.touching;
}

static void title_screen(void)
{
  for (;;) {
    for (uint16_t i = 0; i < 1500; i++) {
      GD.get_inputs();
      GD.ClearColorRGB(attr(2));
      GD.Clear();
      GD.Begin(BITMAPS);
      screen2ii(0, 0, 5, 0);
      draw_status(0);

      // Text crawl across bottom
      GD.ScissorSize(256, 272);
      GD.ScissorXY(X0, 0);
      GD.ColorRGB(WHITE);
      GD.cmd_text(256 - i, 222, 27, 0, marquee);

      GD.swap();
      if (touch())
        return;
    }

    for (state.level = 0; state.level < 19; state.level++) {
      load_level();
      for (byte t = 0; t < 30; t++) {
        draw_all();
        move_all();
        if (touch())
          return;
      }
    }
  }
}

void loop()
{
  title_screen();
  do {
    GD.get_inputs();
    GD.Clear();
    GD.swap();
  } while (touch());

  state.level = START_LEVEL;
  state.score = 0;

  for (state.lives = 3; state.lives; state.lives--) {
    LOAD_ASSETS();
    load_level();
    state.alive = 1;
    while (state.alive) {
uint32_t t0 = GD.rd32(REG_FRAMES);
      draw_all();
      move_all();
uint32_t t1 = GD.rd32(REG_FRAMES);
      printf("Took %d frames\n", (t1 - t0) & 0xff);
    }
  }
  game_over();
}
