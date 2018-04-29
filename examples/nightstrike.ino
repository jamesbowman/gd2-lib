#include <EEPROM.h>
#include <SPI.h>
#include <GD2.h>

// For random events.
// PROB(1, 50)    One time in 50
// PROB(7, 8)     7 times in 8
#define PROB(num, denom) (GD.random() < (uint16_t)(65535UL * (num) / (denom)))

#include "nightstrike_welcome_assets.h"
#include "nightstrike_1_assets.h"

static uint32_t t;

void draw_dxt1(byte color_handle, byte bit_handle)  //' dxt1{
{
  GD.Begin(BITMAPS);

  GD.BlendFunc(ONE, ZERO);
  GD.ColorA(0x55);
  GD.Vertex2ii(0, 0, bit_handle, 0);

  GD.BlendFunc(ONE, ONE);
  GD.ColorA(0xaa);
  GD.Vertex2ii(0, 0, bit_handle, 1);

  GD.ColorMask(1,1,1,0);
  GD.cmd_scale(F16(4), F16(4));
  GD.cmd_setmatrix();

  GD.BlendFunc(DST_ALPHA, ZERO);
  GD.Vertex2ii(0, 0, color_handle, 1);

  GD.BlendFunc(ONE_MINUS_DST_ALPHA, ONE);
  GD.Vertex2ii(0, 0, color_handle, 0);

  GD.RestoreContext();
} //' }dxt1

struct Element {
public:
  int16_t x, y;
  const shape_t *shape;
  void set(const shape_t *_shape) {
    shape = _shape;
  }
  void setxy(int _x, int _y) {
    x = _x;
    y = _y;
  }
  void vertex(byte cell, byte scale) {
    int x0, y0;
    x0 = x - (shape->w >> 1) * scale;
    y0 = y - (shape->h >> 1) * scale;
    if (((x0 | y0) & 511) == 0) {
      GD.Vertex2ii(x0, y0, shape->handle, cell);
    } else {
      GD.BitmapHandle(shape->handle);
      GD.Cell(cell);
      GD.Vertex2f(x0 << 4, y0 << 4);
    }
  }
  void draw(byte cell, byte flip = 0, byte scale = 1) {
    if (!flip && scale == 1) {
      vertex(cell, scale);
    } else {
      GD.SaveContext();
      GD.cmd_loadidentity();
      GD.cmd_translate(F16(scale * shape->w / 2), F16(scale * shape->h / 2));
      if (flip)
        GD.cmd_scale(F16(-scale), F16(scale));
      else
        GD.cmd_scale(F16(scale), F16(scale));
      GD.cmd_translate(F16(-(shape->w / 2)), F16(-(shape->h / 2)));
      GD.cmd_setmatrix();
      vertex(cell, scale);
      GD.RestoreContext();
    }
  }
};

struct RotatingElement : public Element {
public:
  uint16_t angle;
  void set(const shape_t *_shape) {
    shape = _shape;
  }
  void setxy(int _x, int _y) {
    x = _x << 4;
    y = _y << 4;
  }
  void setxy_16ths(int _x, int _y) {
    x = _x;
    y = _y;
  }
  void draw(byte cell) {
    GD.SaveContext();
    GD.cmd_loadidentity();
    GD.cmd_translate(F16(shape->size / 2), F16(shape->size / 2));
    GD.cmd_rotate(angle);
    GD.cmd_translate(F16(-(shape->w / 2)), F16(-(shape->h / 2)));
    GD.cmd_setmatrix();

    int x0, y0;
    x0 = x - (shape->size << 3);
    y0 = y - (shape->size << 3);
    GD.BitmapHandle(shape->handle);
    GD.Cell(cell);
    GD.Vertex2f(x0, y0);

    GD.RestoreContext();
  }
};

class Stack {
  int8_t n;
  int8_t *s;  // 0 means free
public:
  void initialize(int8_t *_s, size_t _n) {
    s = _s;
    n = _n;
    memset(s, 0, n);
  }
  int8_t alloc(void) {
    for (int8_t i = 0; i < n; i++)
      if (s[i] == 0) {
        s[i] = 1;
        return i;
      }
    return -1;
  }
  void free(int8_t i) {
    s[i] = 0;
  }
  int8_t alive() {
    return next(-1);
  }
  int8_t next(int8_t i) {
    i++;
    for (; i < n; i++) {
      if (t > 148000UL) {
        REPORT(n);
        REPORT(i);
      }
      if (s[i])
        return i;
    }
    return -1;
  }
};


#define NUM_MISSILES    8
#define MISSILE_TRAIL   8
#define NUM_FIRES      16
#define NUM_EXPLOSIONS 16

static const PROGMEM uint8_t fire_a[] = {0,1,2,3,4,5,6,7,8, 9,9, 10,10, 11,11, 12,12, 13,13, 14,14, 15,15, 16,16};

class Fires {
  int8_t ss[NUM_FIRES];
  Stack stack;
  Element e[NUM_FIRES];
  byte anim[NUM_FIRES];
public:
  void initialize() {
    stack.initialize(ss, sizeof(ss));
  }
  void create(int x, int y) {
    int8_t i = stack.alloc();
    if (i >= 0) {
      e[i].set(&FIRE_SHAPE);
      e[i].setxy(x, y);
      anim[i] = 0;
    }
  }
  void draw() {
    for (int8_t i = stack.alive(); i >= 0; i = stack.next(i))
      e[i].draw(pgm_read_byte(fire_a + anim[i]));
  }
  void update(int t) {
    if ((t & 1) == 0) {
      for (int8_t i = stack.alive(); i >= 0; i = stack.next(i)) {
        if (++anim[i] == sizeof(fire_a))
          stack.free(i);
      }
    }
  }
};

static const PROGMEM uint8_t explode_a[] = {0,1,2,3,4,5,5,6,6,7,7,8,8,9,9};

class Explosions {
  int8_t ss[NUM_EXPLOSIONS];
  Stack stack;
  RotatingElement e[NUM_EXPLOSIONS];
  byte anim[NUM_EXPLOSIONS];
public:
  void initialize() {
    stack.initialize(ss, sizeof(ss));
  }
  void create(int x, int y, uint16_t angle) {
    int8_t i = stack.alloc();
    if (i >= 0) {
      e[i].set(&EXPLODE_BIG_SHAPE);
      e[i].setxy_16ths(x, y);
      e[i].angle = angle;
      anim[i] = 0;
    }
    GD.play(KICKDRUM);
  }
  void draw() {
    for (int8_t i = stack.alive(); i >= 0; i = stack.next(i))
      e[i].draw(pgm_read_byte(explode_a + anim[i]));
  }
  void update(int t) {
    if ((t & 1) == 0) {
      for (int8_t i = stack.alive(); i >= 0; i = stack.next(i)) {
        if (++anim[i] == sizeof(explode_a))
          stack.free(i);
      }
    }
  }
};

#define NUM_SOLDIERS  3

#define SOLDIER_LEFT   (-SOLDIER_RUN_WIDTH / 2)
#define SOLDIER_RIGHT  (480 + (SOLDIER_RUN_WIDTH / 2))

static const PROGMEM uint8_t soldier_a[] = {0,0,0,1,1,2,2,3,3,4,4,4,5,5,6,6,7,7};

class SoldierObject {
  Element e;
  int8_t vx;
  byte a;
public:
  void initialize(int8_t _vx) {
    a = 0;
    vx = _vx;
    e.set(&SOLDIER_RUN_SHAPE);
    e.setxy((e.x < 0) ? SOLDIER_RIGHT : SOLDIER_LEFT, 272 - SOLDIER_RUN_HEIGHT / 2);
  }
  void draw() {
    e.draw(pgm_read_byte(soldier_a + a), vx > 0);
  }
  byte update(int t);
};

class Soldiers {
  int8_t ss[NUM_SOLDIERS];
  Stack stack;
  SoldierObject soldiers[NUM_SOLDIERS];
public:
  void initialize() {
    stack.initialize(ss, sizeof(ss));
  }
  void create() {
    int8_t i = stack.alloc();
    if (i >= 0) {
      soldiers[i].initialize(PROB(1, 2) ? -1 : 1);
    }
  }
  void draw() {
    for (int8_t i = stack.alive(); i >= 0; i = stack.next(i))
      soldiers[i].draw();
  }
  void update(int t) {
    if ((t & 1) == 0) {
      for (int8_t i = stack.alive(); i >= 0; i = stack.next(i)) {
        if (soldiers[i].update(t))
          stack.free(i);
      }
    }
    if (PROB(1, 100))
      create();
  }
};


#define HOMING_SPEED  32
#define HOMING_SLEW  400

class MissileObject {
  RotatingElement e;
  union {
    struct { int16_t x, y; } trail[MISSILE_TRAIL];
    int dir;
  };
  int8_t th, ts;
  int16_t vx, vy;
public:
  void initialize(uint16_t angle, uint16_t vel) {
    e.set(&MISSILE_A_SHAPE);
    e.angle = angle;
    e.setxy_16ths(16*240 - GD.rsin(16*110, angle),
                  16*265 + GD.rcos(16*110, angle));
    vx = -GD.rsin(vel, e.angle);
    vy =  GD.rcos(vel, e.angle);
    th = 0;
    ts = 0;
  }
  void air(Element &o) {
    e.set(&MISSILE_C_SHAPE);
    e.setxy(o.x, o.y);
    ts = -1;
    trail[0].x = (vx < 0) ? -1 : 1;
  }
  void draw() {
    byte player = (0 <= ts);
    if (player) {
      GD.Begin(LINE_STRIP);
      for (int i = 0; i < ts; i++) {
        GD.ColorA(255 - (i << 5));
        GD.LineWidth(48 - (i << 2));
        int j = (th - i) & (MISSILE_TRAIL - 1);
        GD.Vertex2f(trail[j].x, trail[j].y);
      }
      GD.ColorA(255);
      GD.Begin(BITMAPS);
      e.angle = GD.atan2(vy, vx);
    }
    e.draw(0);
    // if (!player) GD.cmd_number(e.x >> 4, e.y >>4, 26, OPT_CENTER | OPT_SIGNED, trail[0].x);
  }
  byte update(int t);
  void blowup();
  byte collide(Element &other) {
    if (ts == -1)
      return 0;
    int dx = abs((e.x >> 4) - other.x);
    int dy = abs((e.y >> 4) - other.y);
    return (dx < 32) && (dy < 32);
  }
  byte hitbase() {
    if (0 <= ts)
      return 0;
    int dx = abs((e.x >> 4) - 240);
    int dy = abs((e.y >> 4) - 270);
    return (dx < 50) && (dy < 50);
  }
  void tailpipe(int &x, int &y) {
    uint16_t a = 0x8000 + e.angle;
    x = e.x - GD.rsin(22 * 16, a);
    y = e.y + GD.rcos(22 * 16, a);
  }
};

class Missiles {
  int8_t ss[NUM_MISSILES];
  Stack stack;
  MissileObject m[NUM_MISSILES];
public:
  void initialize() {
    stack.initialize(ss, sizeof(ss));
  }
  void launch(uint16_t angle, uint16_t vel) {
    int8_t i = stack.alloc();
    if (i >= 0) {
      m[i].initialize(angle, vel);
    }
  }
  void airlaunch(Element &e, uint16_t angle, uint16_t vel) {
    int8_t i = stack.alloc();
    if (i >= 0) {
      m[i].initialize(angle, 10);
      m[i].air(e);
    }
  }
  void draw() {
    for (int8_t i = stack.alive(); i >= 0; i = stack.next(i))
      m[i].draw();
  }
  byte collide(Element &e) {
    for (int8_t i = stack.alive(); i >= 0; i = stack.next(i))
      if (m[i].collide(e)) {
        stack.free(i);
        m[i].blowup();
        return 1;
      }
    return 0;
  }
  byte hitbase() {
    for (int8_t i = stack.alive(); i >= 0; i = stack.next(i))
      if (m[i].hitbase()) {
        stack.free(i);
        m[i].blowup();
        return 1;
      }
    return 0;
  }
  void update(int t) {
    for (int8_t i = stack.alive(); i >= 0; i = stack.next(i)) {
      // REPORT(i);
      if (m[i].update(t)) {
        stack.free(i);
        m[i].blowup();
      }
    }
  }
};

#define NUM_SPLATS 10

#define RED     0
#define YELLOW  1

class Sparks {
  int x[NUM_SPLATS];
  int y[NUM_SPLATS];
  int8_t xv[NUM_SPLATS];
  int8_t yv[NUM_SPLATS];
  byte age[NUM_SPLATS];
  byte kind[NUM_SPLATS];
  int8_t ss[NUM_SPLATS];
  Stack stack;
public:
  void initialize() {
    stack.initialize(ss, sizeof(ss));
  }
  void launch(byte n, byte _kind, int _x, int _y) {
    while (n--) {
      int8_t i = stack.alloc();
      if (i >= 0) {
        uint16_t angle = 0x5000 + GD.random(0x6000);
        byte v = 64 + (GD.random() & 63);
        xv[i] = -GD.rsin(v, angle);
        yv[i] =  GD.rcos(v, angle);
        x[i] = _x - (xv[i] << 2);
        y[i] = _y - (yv[i] << 2);
        kind[i] = _kind;
        age[i] = 0;
      }
    }
  }
  void draw() {
    GD.Begin(LINES);
    for (int8_t i = stack.alive(); i >= 0; i = stack.next(i)) {
      byte size;
      if (kind[i] == YELLOW) {
        GD.ColorRGB(0xffe000);
        size = 60;
      } else {
        GD.ColorRGB(0xc00000);
        size = 100;
      }
      GD.LineWidth(GD.rsin(size, age[i] << 11)); //' sparks{
      GD.Vertex2f(x[i], y[i]);
      GD.Vertex2f(x[i] + xv[i], y[i] + yv[i]);  //' }sparks
    }
    GD.Begin(BITMAPS);
  }
  void update() {
    for (int8_t i = stack.alive(); i >= 0; i = stack.next(i)) {
      x[i] += xv[i];
      y[i] += yv[i];
      yv[i] += 3;
      if (++age[i] == 16)
        stack.free(i);
    }
  }
};

#define NUM_REWARDS 3
#define REWARDS_FONT INFOFONT_HANDLE

class Rewards {
  int x[NUM_REWARDS];
  int y[NUM_REWARDS];
  const char *amount[NUM_REWARDS];
  byte age[NUM_REWARDS];
  int8_t ss[NUM_REWARDS];
  Stack stack;
public:
  void initialize() {
    stack.initialize(ss, sizeof(ss));
  }
  void create(int _x, int _y, const char *_amount) {
    int8_t i = stack.alloc();
    if (i >= 0) {
      x[i] = _x;
      y[i] = _y;
      amount[i] = _amount;
      age[i] = 0;
    }
  }
  void draw() {
    GD.PointSize(24 * 16);
    for (int8_t i = stack.alive(); i >= 0; i = stack.next(i)) {
      GD.ColorA(255 - age[i] * 4);
      GD.ColorRGB(0x000000);
      GD.cmd_text(x[i] - 1, y[i] - 1, REWARDS_FONT, OPT_CENTER, amount[i]);
      GD.ColorRGB(0xffffff);
      GD.cmd_text(x[i], y[i], REWARDS_FONT, OPT_CENTER, amount[i]);
    }
  }
  void update() {
    for (int8_t i = stack.alive(); i >= 0; i = stack.next(i)) {
      y[i]--;
      if (++age[i] == 60)
        stack.free(i);
    }
  }
};

#define THRESH  20

class BaseObject {
public:
  RotatingElement turret;
  Element front;
  byte power;
  byte prev_touching;
  uint32_t cash;
  byte life;
  byte hurting;
  void initialize() {
    front.set(&DEFENSOR_FRONT_SHAPE);
    front.setxy(240, 272 - 30);
    turret.set(&DEFENSOR_TURRET_SHAPE);
    turret.angle = 0x7000;
    power = 0;
    cash = 0;
    life = 100;
    hurting = 0;
  }
  void draw_base() {
    // turret.angle += 200;
    // if (turret.angle > 0xb800)
    //   turret.angle = 0x4800;
    turret.setxy_16ths(16*240 - GD.rsin(16*77, turret.angle),
                16*265 + GD.rcos(16*77, turret.angle));
    if (hurting && (hurting < 16)) {
      GD.ColorA(255 - hurting * 16);
      GD.ColorRGB(0xff0000);
      GD.PointSize(50 * 16 + hurting * 99);
      GD.Begin(POINTS);
      GD.Vertex2ii(240, 270);
      GD.ColorA(255);
      GD.ColorRGB(0xffffff);
      GD.Begin(BITMAPS);
    }
    turret.draw(0);
    GD.ColorRGB(0x000000);
    front.draw(0);
    if (hurting)
      GD.ColorRGB(0xff0000);
    else
      GD.ColorRGB(0xffffff);
    front.draw(1);
    GD.ColorRGB(0xffffff);
  }
  void draw_status() {
    GD.SaveContext();
    GD.Begin(LINES);

    GD.ColorA(0x70);

    GD.ColorRGB(0x000000);
    GD.LineWidth(5 * 16);
    GD.Vertex2ii(10, 10);
    GD.Vertex2ii(10 + 460, 10);

    if (power > THRESH) {
      int x0 = (240 - power);
      int x1 = (240 + power);

      GD.ColorRGB(0xff6040);
      GD.Vertex2ii(x0, 10);
      GD.Vertex2ii(x1, 10);

      GD.ColorA(0xff);
      GD.ColorRGB(0xffd0a0);
      GD.LineWidth(2 * 16);
      GD.Vertex2ii(x0, 10);
      GD.Vertex2ii(x1, 10);
    }
    GD.RestoreContext();
    GD.ColorA(0xf0);
    GD.ColorRGB(0xd7f2fd);
    GD.Begin(BITMAPS);
    GD.cmd_text(3, 16, INFOFONT_HANDLE, 0, "$");
    GD.cmd_number(15, 16, INFOFONT_HANDLE, 0, cash);

    GD.cmd_number(30, 32, INFOFONT_HANDLE, OPT_RIGHTX, life);
    GD.cmd_text(30, 32, INFOFONT_HANDLE, 0, "/100");
  }
  void reward(uint16_t amt) {
    cash += amt;
  }
  void damage() {
    life = max(0, life - 9);
    hurting = 1;
  }
  byte update(Missiles &missiles) {
    byte touching = (GD.inputs.x != -32768);
    if (touching)
      power = min(230, power + 3);
    if (!touching && prev_touching && (power > THRESH)) {
      GD.play(HIHAT);
      missiles.launch(turret.angle, power);
      power = 0;
    }
    prev_touching = touching;
    if ((GD.inputs.track_tag & 0xff) == 1)    //' track{
      turret.angle = GD.inputs.track_val;     //' }track
    if (hurting) {
      if (++hurting == 30)
        hurting = 0;
    }
    if (missiles.hitbase())
      damage();
    return (life != 0);
  }
};

#define NUM_HELIS 5

#define HELI_LEFT   (-HELI_WIDTH / 2)
#define HELI_RIGHT  (480 + (HELI_WIDTH / 2))

class HeliObject {
  Element e;
  int8_t vx, vy;
  int8_t state;  // -1 means alive, 0..n is death anim
public:
  void initialize() {
    e.set(&HELI_SHAPE);
    int x;
    if (PROB(1, 2)) {
      vx = 1 + GD.random(2);
      x = HELI_LEFT;
    } else {
      vx = -1 - GD.random(2);
      x = HELI_RIGHT;
    }
    e.setxy(x, (HELI_HEIGHT / 2) + GD.random(100));
    state = -1;
    vy = 0;
  }
  void draw(byte anim) {
    if (state == -1)
      e.draw(anim, vx > 0);
    else {
      byte aframes[] = {0x00, 0x01, 0x02, 0x81, 0x80, 0x03, 0x83};
      byte a = aframes[(state >> 1) % sizeof(aframes)];
      e.draw(a & 0x7f, a & 0x80, 2);
    }
  }
  byte update();
};

class Helis {
  int8_t ss[NUM_HELIS];
  Stack stack;
  HeliObject m[NUM_HELIS];
public:
  void initialize() {
    stack.initialize(ss, sizeof(ss));
  }
  void launch() {
    int8_t i = stack.alloc();
    if (i >= 0) {
      m[i].initialize();
    }
  }
  void draw(int t) {
    for (int8_t i = stack.alive(); i >= 0; i = stack.next(i))
      m[i].draw(((i + t) >> 2) & 1);
  }
  void update(byte level) {
    for (int8_t i = stack.alive(); i >= 0; i = stack.next(i))
      if (m[i].update())
        stack.free(i);
  }
};

class GameObject {
public:
  BaseObject base;
  Missiles missiles;
  Fires fires;
  Explosions explosions;
  Helis helis;
  Soldiers soldiers;
  Sparks sparks;
  Rewards rewards;
  byte level;
  int t;
  void load_level(byte n) {

    level = n;
    GD.Clear();
    GD.cmd_text(240, 110, 29, OPT_CENTER, "LEVEL");
    GD.cmd_number(240, 145, 31, OPT_CENTER, n + 1);
    GD.swap();
    GD.finish();

    char filename[] = "night#.gd2";
    filename[5] = '0' + (n % 5);
    GD.load(filename);

    if (n == 0)
      base.initialize();
    missiles.initialize();
    fires.initialize();
    explosions.initialize();
    helis.initialize();
    soldiers.initialize();
    sparks.initialize();
    rewards.initialize();
    t = 0;
  }
  void draw() {
    GD.Tag(1);
    draw_dxt1(BACKGROUND_COLOR_HANDLE, BACKGROUND_BITS_HANDLE);
    GD.TagMask(0);
    GD.Begin(BITMAPS);
    base.draw_base();
    helis.draw(t);
    soldiers.draw();
    fires.draw();
    missiles.draw();
    explosions.draw();
    sparks.draw();
    rewards.draw();
    base.draw_status();
  }
  byte update(byte playing) {
    byte alive = base.update(missiles);
    // if ((t % 30) == 29) missiles.launch(base.turret.angle, 120);
    uint16_t launch;
    launch = min(1024, 128 + level * 64);
    if (GD.random() < launch)
      helis.launch();
    helis.update(level);
    soldiers.update(t);
    fires.update(t);
    explosions.update(t);
    sparks.update();
    rewards.update();
    missiles.update(t);
    t++;

    byte leveltime = min(50, 10 + level * 5);
    if (playing && (t == (leveltime * 60))) {
      load_level(level + 1);
    }
    return alive;
  }
};

static GameObject game;

void MissileObject::blowup() {
  game.explosions.create(e.x, e.y /* - 16 * 40 */, e.angle);
}

byte MissileObject::update(int t)
{
  if (0 <= ts) {
    if ((t & 1) == 0) {
      th = (th + 1) & (MISSILE_TRAIL - 1);
      trail[th].x = e.x;
      trail[th].y = e.y;
      ts = min(MISSILE_TRAIL, ts + 1);
    }
    vy += 4;
  } else {
    vx = -GD.rsin(HOMING_SPEED, e.angle);
    vy = GD.rcos(HOMING_SPEED, e.angle);
    int16_t dy = (16 * 272) - e.y;
    int16_t dx = (16 * 240) - e.x;
    uint16_t seek = GD.atan2(dy, dx);
    int16_t steer = seek - e.angle;
    if (abs(steer) > HOMING_SLEW)
      e.angle -= dir * HOMING_SLEW;
    else
      dir = 0;
    if (((t & 7) == 0) && PROB(7, 8)) {
      int x, y;
      tailpipe(x, y);
      if ((0 <= y) && (0 <= x) && (x < (16 * 480)))
        game.fires.create(x >> 4, y >> 4);
    }
  }
  e.setxy_16ths(e.x + vx, e.y + vy);
  // Return true if way offscreen
  return (e.x < (16 * -200)) ||
         (e.x > (16 * 680)) ||
         (e.y > (16 * 262));
}

byte HeliObject::update()
{
  e.x += vx;
  if (0 <= state) {
    e.y += vy;
    vy = min(vy + 1, 6);
    state++;
  }
  if (PROB(1, 300)) {
    game.missiles.airlaunch(e, (vx < 0) ? 0x4000 : 0xc000, vx);
  }
  if ((state == -1) && game.missiles.collide(e)) {
    game.rewards.create(e.x, e.y, "+$100");
    game.base.reward(100);
    game.sparks.launch(8, YELLOW, e.x << 4, e.y << 4);
    e.set(&COPTER_FALL_SHAPE);
    state = 0;
    vy = -4;
    GD.play(TUBA, 36);
  }
  if ((0 <= state) && e.y > 252) {
    game.sparks.launch(5, YELLOW, e.x << 4, e.y << 4);
    game.explosions.create(e.x << 4, e.y << 4, 0x0000);
    return 1;
  }
  return ((e.x < HELI_LEFT) || (e.x > HELI_RIGHT));
}

byte SoldierObject::update(int t)
{
  if ((t & 1) == 0) {
    a = (a + 1) % sizeof(soldier_a);
    e.x += vx;
  }
  if (game.missiles.collide(e)) {
    game.fires.create(e.x, e.y);
    game.rewards.create(e.x, e.y, "+$50");
    game.base.reward(50);
    game.sparks.launch(6, RED, e.x << 4, e.y << 4);
    GD.play(TUBA, 108);
    return 1;
  }
  return ((e.x < SOLDIER_LEFT) || (e.x > SOLDIER_RIGHT));
}

static void blocktext(int x, int y, byte font, const char *s)  //' context{
{
  GD.SaveContext();
  GD.ColorRGB(0x000000);
  GD.cmd_text(x-1, y-1, font, 0, s);
  GD.cmd_text(x+1, y-1, font, 0, s);
  GD.cmd_text(x-1, y+1, font, 0, s);
  GD.cmd_text(x+1, y+1, font, 0, s);
  GD.RestoreContext();

  GD.cmd_text(x, y, font, 0, s);
} //' }context

static void draw_fade(byte fade)
{
  GD.TagMask(0);  //' fade{
  GD.ColorA(fade);
  GD.ColorRGB(0x000000);
  GD.Begin(RECTS);
  GD.Vertex2ii(0, 0);
  GD.Vertex2ii(480, 272); //' }fade
}

static void welcome()
{
  // Streamer stream;

  GD.safeload("nightw.gd2");
  byte fade = 0;
  int t;

  const char song[] = "mesmeriz.ima";
  // stream.begin(song);
  byte playing = 1;
  do {
    GD.get_inputs();
    if (GD.inputs.tag == 100)
      fade = 1;
    if (fade != 0)
      fade = min(255, fade + 30);
    draw_dxt1(BACKGROUND_COLOR_HANDLE, BACKGROUND_BITS_HANDLE);
    GD.ColorRGB(0xd7f2fd);

    blocktext(25, 16, WELCOME_DISPLAYFONT_HANDLE, "NIGHTSTRIKE");

    byte flash = 128 + GD.rsin(127, t);
    t += 1000;
    GD.ColorA(flash);
    GD.Tag(100);
    blocktext(51, 114, WELCOME_DISPLAYFONT_HANDLE, "START");

    draw_fade(fade);
    GD.swap();
    GD.random();
    // if (!stream.feed()) {
    //   playing = 0;
    //   GD.sample(0, 0, 0, 0);
    // }
  } while (fade != 255);
  GD.sample(0, 0, 0, 0);
  // GD.play(MUTE); GD.play(UNMUTE);
}

void setup()
{
  Serial.begin(1000000);  // JCB
  GD.begin();

  GD.cmd_track(240, 271, 1, 1, 0x01);  //' cmd_track{ }cmd_track
}

void loop()
{
  static byte in_welcome = 1;
  if (in_welcome) {
    welcome();
    game.load_level(0);
    in_welcome = 0;
  } else {
    GD.get_inputs();
    uint32_t t0 = millis();
    byte alive = game.update(true);
    game.draw();

    if (!alive) {
      for (byte i = 255; i; i--) {
        game.draw();
        draw_fade(255 - i);
        GD.ColorA(255);
        GD.ColorRGB(0xffffff);
        blocktext(200, 136, INFOFONT_HANDLE, "GAME OVER");
        GD.swap();
        game.update(false);
      }
      in_welcome = 1;
      GD.Clear();
      GD.swap();
      return;
    }
    // uint32_t took = millis() - t0;   // JCB
    // GD.cmd_number(479, 100, 27, 3 | OPT_RIGHTX, took);   // JCB
    // caption(t - 428, "Graphics by MindChamber");  // JCB
    t++;  // JCB
    GD.swap();
  }
}
