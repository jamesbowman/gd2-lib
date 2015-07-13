#include <EEPROM.h>
#include <SPI.h>
#include <GD2.h>

#define UART_SPEED 1000000

#include "invaders_assets.h"

/*---------------------------------------------
  Trivia: There is NO random number generator
  anywhere in Space Invaders....
---------------------------------------------*/

/*---------------------------------------------
  Global definitions
---------------------------------------------*/
enum graphic_id {
  // Invader sprites - Top, Middle, Bottom, two animation frames each...
  GR_INVADER_T,
  GR_INVADER_M,
  GR_INVADER_B,
  GR_BOMB_ZIGZAG,  // Zigzag bomb
  GR_BOMB_BARS,    // The bomb with rolling horizontal bars across it
  GR_BOMB_DIAG,    // The bomb with diagonal bars across it
  GR_BOMB_OTHER,   // Other bomb graphics (explosion and blank)
  // The player (with bullet)
  GR_PLAYER,
  GR_BULLET,       // nb. Has a '0' in frame 2 (for the saucer...)
  // The saucer at the top
  GR_SAUCER,
  GR_SAUCER_SCORE,
  // Shields
  GR_SHIELD1,
  GR_SHIELD2,
  GR_SHIELD3,
  GR_SHIELD4
};

#define invaderRows 5
#define invadersPerRow 11
#define numInvaders (invaderRows*invadersPerRow)

// Positions of things on screen
// nb. Space Invaders screen is 256x224 pixels
#define screenTop 8
#define screenWidth 224
#define screenHeight 256
#define screenLeft ((480 - screenWidth) / 2)
// Player
#define playerMinLeft 18
#define playerMaxRight 188
#define playerYpos 216
#define playerSpeed 1
// Bullet
#define bulletHeight 4
#define bulletSpeed 4
#define bulletTop 35
// Invaders
#define invaderAppearX 26
#define invaderAppearY 64
#define invaderXspacing 16
#define invaderYspacing 16
#define invaderXstep 2
#define invaderYstep 8
#define invaderXmin 10
#define invaderXmax 202
// Saucer
#define saucerYpos 42
#define saucerSpeed 1
#define saucerXmin 0
#define saucerXmax (screenWidth-16)
#define saucerSkip 3
#define saucerFrequency (1*60)
// Shields
#define numShields 4
#define shieldXpos 32
#define shieldYpos 192 
#define shieldXstep 45
// Bombs
#define bombSpeed 1
#define bombYmax 230

/*---------------------------------------------
  Global vars
---------------------------------------------*/
// This increments once per frame
static unsigned int frameCounter;

// The current wave of invaders [0..n]
static unsigned int invaderWave;

// Number of lives the player has left...
static byte numLives;

// Player's score...
static unsigned int playerScore;

// High score
static unsigned int highScore;

// Number of living space invaders
static unsigned int remainingInvaders;

// Timer for the background heartbeat sound
static int beatCounter;

/*---------------------------------------------
  General functions
---------------------------------------------*/
void printScore(int8_t x, const char *m, unsigned int s, int8_t xoff)
{
  GD.cmd_text(screenLeft + 8 * x, screenTop, 16, 0, m);
  GD.cmd_number(screenLeft + 8 * (x + xoff), screenTop + 16, 16, 4, s);
}

void updateScore()
{
  highScore = max(highScore, playerScore);
  printScore(0, "SCORE", playerScore, 0);
  printScore(20, "HI-SCORE", highScore, 4);
}

void drawBases()
{
  GD.cmd_number(screenLeft, screenTop + 240, 16, 0, numLives);
  for (int i = 1; i < numLives; i++)
    GD.Vertex2ii(screenLeft + 16 * i, screenTop + 240, 0, CELL_PLAYER);
}

/*---------------------------------------------
  A generic object in the game
---------------------------------------------*/
enum object_status {
  S_WAITING,
  S_ALIVE,
  S_DYING,
  S_DEAD
};

struct GameObject {
  byte sprite;     // Which sprite to use for my graphic (see "sprite_id")
  byte status;     // What I'm doing at the moment
  int xpos,ypos;   // Position on screen
  byte frame;      // Animation frame
  byte handle;     // Bitmap handle
  // State of objects in the game
  void initialize(object_status t=S_WAITING, int x=400, int y=0) {
    status = t;
    xpos = x;
    ypos = y;
    handle = 0;
    frame = 0;
  }
  void draw() {
    if ((status != S_WAITING) && (status != S_DEAD))
      GD.Vertex2ii(xpos + screenLeft, ypos + screenTop, handle, frame);
  }
  byte collision() {
    return 0xff;
    // return GD.rd(0x2900+sprite);
  }
};

/*---------------------------------------------
 Player's bullet
---------------------------------------------*/
// Forward references to functions
bool killInvader(byte spriteNumber);
void shootShield(byte spriteNumber, int bulletX);
void shootSaucer();
void shootBomb(byte spriteNumber);
void incSaucerCounter();

class BulletObject : public GameObject {
  byte timer;
  bool visibleDeath;
  void die(bool v) {
    visibleDeath = v;
    status = S_DYING;
    timer = 12;
  }
public:
  void reset() {
    initialize();
    timer = 0;
  }
  void fire(GameObject& p) {
    if (status == S_WAITING){
      status = S_ALIVE;
      xpos = p.xpos;
      ypos = p.ypos+bulletSpeed-bulletHeight;
      // playerShootSound = true;
    }
  }
  void update() {
    switch (status) {
      case S_ALIVE:  ypos -= bulletSpeed;
                     if (ypos <= bulletTop) {
                       ypos = bulletTop;
                       die(true);
                     }
                     else {
                       frame = CELL_BULLET;
                     }
                     break;
      case S_DYING:  if (!--timer) {
                       status = S_WAITING;
                       incSaucerCounter();
                     }
                     else if (visibleDeath) {
                       frame = CELL_BULLET_BLAST;
                     }
                     break;
    }
    GD.wr16(REG_TAG_X, screenLeft + xpos + 6);
    GD.wr16(REG_TAG_Y, screenTop + ypos);
  }
  void setY(int y) {
    if (status == S_DYING) {
      ypos = y;
    }
  }
  // See if the bullet hit anything
  void collide() {
#if 0
    if (status == S_ALIVE) {
      byte b = collision();
      if (b != 0xff) {
        if ((b >= SP_FIRST_INVADER) and (b <= SP_LAST_INVADER)) {
          if (killInvader(b)) {
            die(false);
          }
        }
        if ((b >= SP_FIRST_SHIELD) and (b <= SP_LAST_SHIELD)) {
          shootShield(b,xpos);
          die(true);
        }
        if ((b >= SP_SAUCER1) and (b <= SP_SAUCER2)) {
          shootSaucer();
          die(false);
        }
        if ((b >= SP_BOMB1) and (b <= SP_BOMB3)) {
          shootBomb(b);
          die(false);
        }
      }
    }
#endif
  }
} bullet;

/*---------------------------------------------
 The player
---------------------------------------------*/
class Player : public GameObject {
  byte timer;
  int fake;
public:
  void reset() {
    timer = 2*numInvaders;
    initialize(S_WAITING,playerMinLeft,playerYpos);
    frame = CELL_PLAYER;
  }

  void update() {
    int frame = 3;
    switch (status) {
      case S_WAITING: xpos = playerMinLeft;
                      ypos = playerYpos;
                      if (!--timer) {
                        status = S_ALIVE;
                      }
                      break;
      case S_ALIVE:   /* if (joystick.left()) {
                        xpos -= playerSpeed;
                        if (xpos < playerMinLeft) {
                          xpos = playerMinLeft;
                        }
                      }
                      if (joystick.right()) {
                        xpos += playerSpeed;
                        if (xpos > playerMaxRight) {
                          xpos = playerMaxRight;
                        }
                      }
                      { byte n = Joystick::buttonA|Joystick::buttonB;
                        if (joystick.isPressed(n) and joystick.changed(n)) {
                          bullet.fire(*this);
                        }
                      }
                      */

                      {
                        byte control_left = 0, control_right = 0, control_fire = 0;
                        fake = (fake + 1) & 511;
                        control_right = fake < 256;
                        control_left  = 256 <= fake;
                        control_fire = (GD.random(70)) == 0;

                        if (control_left) {
                        xpos -= playerSpeed;
                        if (xpos < playerMinLeft) {
                          xpos = playerMinLeft;
                        }
                        }
                        if (control_right) {
                          xpos += playerSpeed;
                          if (xpos > playerMaxRight) {
                            xpos = playerMaxRight;
                          }
                        }
                        if (control_fire) {
                          bullet.fire(*this);
                        }
                      }
                        
                      frame = CELL_PLAYER;
                      break;
      case S_DYING:   if (!--timer) {
                        timer = 3 * remainingInvaders;
                        status = (--numLives > 0) ? S_WAITING : S_DEAD;
                      }
                      else {
                        frame = CELL_PLAYER + ((frameCounter & 4) == 0) ? 1 : 2;
                      }
                      break;
    }
  }
  void kill() {
    if (status == S_ALIVE) {
      status = S_DYING;
      timer = 50;
      // playerDeathSound = true;
    }
  }
  bool isAlive() {
    return (status==S_ALIVE);
  }
  bool isDying() {
    return (status==S_DYING);
  }
  bool isDead() {
    return (status==S_DEAD);
  }
  void wakeUp() {
  }
} player;

/*---------------------------------------------
  "Shields" for the player to hide behind
---------------------------------------------*/
static const PROGMEM uint8_t bomb_blast[] = {
  0x08, 0x22, 0x0d, 0x1e, 0x2e, 0x1f, 0x2e, 0x15
};
static const PROGMEM uint8_t bullet_blast[] = {
  0x89, 0x22, 0x7e, 0xff, 0xff, 0x7e, 0x24, 0x91
};

class Shields {
  struct BlastInfo {
    byte sprite;
    int xpos;
    void reset() {
      sprite = 255;
    }
    bool hasBlast() const {
      return (sprite!=255);
    }
    void blast(byte s, int x) {
      sprite = s;
      xpos = x;
    }
  };
  BlastInfo bulletBlast, bombBlast[3];
  void blastShield(BlastInfo& n, bool asBullet) {
    if (n.hasBlast()) {
      /*
      byte s = (n.sprite-SP_FIRST_SHIELD)>>1;
      int8_t x = int8_t(n.xpos-(shieldXpos+(s*shieldXstep)));
      int8_t y = zapShield(s,x,asBullet);
      if (asBullet) {
        bullet.setY(shieldYpos+y);
      }
      */
      n.reset();
    }
  }
public:
  void reset() {
    // remakeShields();
    int x = shieldXpos;
    for (int i=0; i<numShields; ++i) {
      x += shieldXstep;
    }
    bulletBlast.reset();
    for (int8_t i=0; i<3; ++i) {
      bombBlast[i].reset();
    }
  }
  void update() {
    blastShield(bulletBlast,true);
    for (int8_t i=0; i<3; ++i) {
      blastShield(bombBlast[i],false);
    }    
  }
  // Zap them in various ways
  // nb. We defer the action because updating the sprites
  // might be slow and we want to make sure all collision
  // detection happens in the vertical blank
  void shoot(byte s, int x) {
    bulletBlast.blast(s,x);
  }
  void bomb(byte s, int x) {
    for (int8_t i=0; i<3; ++i) {
      BlastInfo& b = bombBlast[i];
      if (!b.hasBlast()) {
        b.blast(s,x);
        break;
      }
    }
  }
  void draw() {
    GD.Tag(1);
    GD.Vertex2ii(screenLeft, shieldYpos+screenTop, 4, 0);
  }
  byte testpixel(int x, int y) {
    int s = SHIELDS + (x >> 3) + (y * (SHIELDS_WIDTH / 8));
    byte mask = 0x80 >> (x & 7);
    return GD.rd(s) & mask;
  }
  void clearpixel(int x, int y) {
    int s = SHIELDS + (x >> 3) + (y * (SHIELDS_WIDTH / 8));
    byte mask = 0x80 >> (x & 7);
    return GD.wr(s, GD.rd(s) & ~mask);
  }
  int zap(int x, int y, bool withBullet) {
    y -= shieldYpos;
    if ((y < 0) || (SHIELDS_HEIGHT <= y))
      return 0;
    x += 6;    // The pixels in the bullet are in column 6 of the graphic
    
    int hy = 99;   // collision y coordinate
    const PROGMEM uint8_t* blastMap;

    if (withBullet) {
      // Bullet, so want to find a set pixel below (x,y)
      for (int r=SHIELDS_HEIGHT-1; r >= y; --r) {
        if (testpixel(x, r)) {
          hy = r;
          break;
        }
      }
      blastMap = bullet_blast;
    } else {
      // Bomb, so want to find a set pixel above (x,y)
      // XXX - Bombs are wider...we check three columns
      for (int r = 0; r < y; ++r) {
        if (testpixel(x, r)) {
          hy = r - 2;
          break;
        }
      }
      blastMap = bomb_blast;
    }
    if (hy != 99) {
      x -= 3;
      // Blast a hole in it
      for (int j=0; j<8; ++j) {  // 8 lines tall
        const int py = hy+j;
        if ((0 <= py) && (py < SHIELDS_HEIGHT)) {
          byte blastMask = 0x80;
          byte blastGraphic = pgm_read_word_near(blastMap);
          for (int i=0; i<8; ++i) {    // 8 pixels wide...
            if ((blastGraphic & blastMask) != 0) {
              // Set shield pixel to 0 where there's a 1 in the source graphic
              clearpixel(x + i, py);
            }
            blastMask >>= 1;
          }
        }
        ++blastMap;
      }
    }
    return hy != 99;
  }
} shields;

void shootShield(byte sprite, int bulletX)
{
  shields.shoot(sprite,bulletX);
}

/*---------------------------------------------
  Flying saucer
  
  The score for the saucer depends on how
  many bullets you've fired. If you want
  a good score hit it with bullet 22 and
  every 15th bullet after that.
  
  The direction of the saucer also depends
  on the bullet count. If you're counting
  bullets then note the the saucer will
  appear on alternate sides and you can
  be ready for it.
 
  Repeat after me: There are NO random
  numbers in Space Invaders.
---------------------------------------------*/
static const PROGMEM uint8_t saucerScores[15] = {
  // nb. There's only one '300' here...
  10,5,10,15,10,10,5,30,10,10,10,5,15,10,5
};
class Saucer : public GameObject {
  byte timer, scoreTimer;
  byte score;
  byte bulletCounter;
  unsigned int timeUntilNextSaucer;
  bool leftRight,goingRight,showingScore;
  void startWaiting() {
    status = S_WAITING;
    timeUntilNextSaucer = saucerFrequency;
  }
public:
  void reset() {
    initialize();
    timer = 1;
    ypos = saucerYpos;
    showingScore = false;
    bulletCounter = 0;
    leftRight = true;
    timeUntilNextSaucer = saucerFrequency;
    handle = 1;
  }
  void update() {
    int8_t xoff=0;
    byte gr1=GR_SAUCER, gr2=gr1;
    byte fr1=3, fr2=fr1;  // Blank sprite
    switch (status) {
      case S_WAITING: if ((remainingInvaders>7) and !--timeUntilNextSaucer) {
                        status = S_ALIVE;
                        timer = saucerSkip;
                        goingRight = leftRight;
                        if (goingRight) {
                          xpos = saucerXmin-saucerSpeed;
                        }
                        else {
                          xpos = saucerXmax+saucerSpeed;
                        }
                        // saucerSound = true;
                      }
                      else {
                        // stopSaucerSnd = true;
                      }
                      break;
      case S_ALIVE:   if (!--timer) {
                        // The player has to go faster then the saucer so we skip frames...
                        timer = saucerSkip;
                      }
                      else {
                        if (goingRight) {
                          xpos += saucerSpeed;
                          if (xpos > saucerXmax) {
                            startWaiting();
                          }
                        }
                        else {
                          xpos -= saucerSpeed;
                          if (xpos < saucerXmin) {
                            startWaiting();
                          }
                        }
                      }
                      frame = 0;    // Normal saucer
                      break;
      case S_DYING:   if (!--timer) {
                        if (showingScore) {
                          startWaiting();
                        }
                        else {
                          timer = 60;
                          showingScore = true;
                          playerScore += score*10;
                        }
                      }
                      else {
                        if (showingScore) {
                          xoff = -5;
                          gr1 = GR_SAUCER_SCORE;
                          gr2 = GR_BULLET;    fr2 = 2;
                          if (score == 5) { fr1=0; xoff-=4;}
                          else if (score == 10) { fr1 = 1; }
                          else if (score == 15) { fr1 = 2; }
                          else if (score == 30) { fr1 = 3; }
                        }
                        else {
                          fr1 = 1;    // Explosion left
                          fr2 = 2;    // Explosion right
                          xoff = -5;  // Move it a bit to the left
                        }
                      }
                      frame = 1;    // Dying saucer
                      break;
    }
  }
  void incCounter() {
    if (++bulletCounter == 15) {
      bulletCounter = 0;
    }
    leftRight = !leftRight;
  }
  void kill() {
    status = S_DYING;
    timer = 36;
    // saucerDieSound = true;
    showingScore = false;
    score = pgm_read_byte(saucerScores+bulletCounter);
  }
} saucer;

void incSaucerCounter()
{
  saucer.incCounter();
}
void shootSaucer()
{
  saucer.kill();
}
/*---------------------------------------------
  A space invader...
---------------------------------------------*/
enum invader_type {
  INVADER_T,    // Top-row invader
  INVADER_M,    // Middle-row invader
  INVADER_B,    // Bottom-row invader
  NUM_INVADER_TYPES
};
static const PROGMEM uint8_t invaderGraphic[NUM_INVADER_TYPES] = {
  CELL_INVADERS + 0, CELL_INVADERS + 2, CELL_INVADERS + 4
};

static const PROGMEM uint8_t invaderScore[NUM_INVADER_TYPES] = {
  30, 20, 10
};

class Invader : public GameObject {
  // Bitmasks for my vars
  enum var_bits {
    TYPEMASK = 0x0003,    // Type of invader, 0=top row, 1=middle row, 2=bottom row
    ANIM     = 0x0010,    // Flip-flop for animation frame
    GO_RIGHT = 0x0020,    // Horizontal direction
    GO_DOWN  = 0x0040,    // If I should go downwards next time
  };
  byte vars;      // All my vars, packed together

  byte readTable(const PROGMEM uint8_t *t) {
    return pgm_read_byte_near(t + (vars&TYPEMASK));
  }
  void updateTheSprite() {
    byte img = readTable(invaderGraphic);
    switch (status) {
      case S_ALIVE:   frame = img + ((vars&ANIM)? 0:1);  // Two frame animation
                      break;
      case S_DYING:   frame = CELL_INVADER_EXPLOSION;             // Explosion graphic
                      break;
    }
  }
public:
  
  bool isAlive() const {
    return ((status==S_WAITING) or (status==S_ALIVE));
  }
  void goDown() {
    vars |= GO_DOWN;
  }

  // Put me on screen at (x,y), set my type and sprite number.
  // I will be invisible and appear next frame (ie. when you call "update()")
  void reset(int x, int y, invader_type t) {
    initialize(S_WAITING,x,y);
    frame = CELL_INVADERS + (2 * t);
    vars = t|GO_RIGHT;
    updateTheSprite();
  }

  // Update me, return "true" if I reach the edge of the screen
  bool update() {
    bool hitTheEdge = false;
    switch (status) {
      case S_WAITING: status = S_ALIVE;
                      break;
      case S_ALIVE:   if (vars&GO_DOWN) {
                        ypos += invaderYstep;
                        vars &= ~GO_DOWN;
                        vars ^= GO_RIGHT;
                      }
                      else {
                        if (vars&GO_RIGHT) {
                          xpos += invaderXstep;
                          hitTheEdge = (xpos >= invaderXmax);
                        }
                        else {
                          xpos -= invaderXstep;
                          hitTheEdge = (xpos <= invaderXmin);
                        }
                      }
                      vars = vars^ANIM;  // Animation flipflop
                      break;
    }
    updateTheSprite();
    return hitTheEdge;
  }
  bool die() {
    bool result = (status==S_ALIVE);
    if (result) {
      status = S_DYING;
      updateTheSprite();
      playerScore += readTable(invaderScore);
      // alienDeathSound = true;
    }
    return result;
  }
  void kill() {
    status = S_DEAD;
    updateTheSprite();
    --remainingInvaders;
  }
};

/*---------------------------------------------
  The array of invaders
---------------------------------------------*/
// Table for starting height of invaders on each level
static const PROGMEM int8_t invaderHeightTable[] = {
  1,2,3,3,3,4,4,4
};

class InvaderList {
  byte nextInvader;              // The invader to update on the next frame
  int8_t dyingInvader;             // Which invader is currently dying
  int8_t deathTimer;               // COuntdown during death phase
  bool anInvaderHitTheEdge;      // When "true" the invaders should go down a line and change direction
  bool anInvaderReachedTheBottom;// When "true" an invader has landed... Game Over!
  Invader invader[numInvaders];  // The invaders
  
  bool findNextLivingInvader() {
    // Find next living invader in the array
    bool foundOne = false;
    for (int8_t i=0; i<numInvaders; ++i) {
      if (++nextInvader == numInvaders) {
        // Actions taken after all the invaders have moved
        nextInvader = 0;
        if (anInvaderHitTheEdge) {
          for (int8_t j=0; j<numInvaders; ++j) {
            invader[j].goDown();
          }
          anInvaderHitTheEdge = false;
        }
      }
      if (invader[nextInvader].isAlive()) {
        foundOne = true;
        break;
      }
    }
    return foundOne;
  }
public:
  void reset(int8_t level) {
    int y = invaderAppearY+(invaderRows*invaderYspacing);
    if (invaderWave > 0) {
      int8_t w = pgm_read_byte(invaderHeightTable+((invaderWave-1)&7));
      y += w*invaderYstep;
    }
    for (int8_t row=0; row<invaderRows; ++row) {
      int x = invaderAppearX;
      for (int8_t col=0; col<invadersPerRow; ++col) {
        const int8_t index = (row*invadersPerRow)+col;
        Invader& n = invader[index];
        invader_type t = INVADER_B;
        if (row > 1) {  t = INVADER_M;   }
        if (row > 3) {  t = INVADER_T;   }
        n.reset(x, y, t);
        x += invaderXspacing;
      }
      y -= invaderYspacing;
    }
    remainingInvaders = numInvaders;
    nextInvader = 0;    // Start updating them here...
    dyingInvader = -1;
    deathTimer = 0;
    anInvaderHitTheEdge = false;
    anInvaderReachedTheBottom = false;
  }
  void update() {
    if (dyingInvader != -1) {
      // We stop marching when an invader dies
      if (!--deathTimer) {
        invader[dyingInvader].kill();
        dyingInvader = -1;
      }
    }
    else if (!player.isDying() and (remainingInvaders>0)) {
      // Update an invader
      Invader& n = invader[nextInvader];
      if (n.isAlive()) {
        // Move the invader
        if (n.update()) {
          anInvaderHitTheEdge = true;
        }
        if ((n.ypos+8) > player.ypos) {
          anInvaderReachedTheBottom = true;
        }
      }
      findNextLivingInvader();
    }
  }
  // Kill invader 'n'
  bool kill(byte n) {
    bool result = invader[n].die();
    if (result) {
      if (dyingInvader != -1) {
        invader[dyingInvader].kill();
      }
      dyingInvader = n;
      deathTimer = 16;
    }
    return result;
  }
  int8_t nearestColumnToPlayer() {
    Invader& n = invader[nextInvader];  // We know this invader is alive so use it as a reference
    int r = nextInvader%invadersPerRow; // The column this invader is in
    int left = n.xpos-(r*invaderXspacing);
    int c = (((player.xpos-left)+(invaderXspacing/2))/invaderXspacing);
    if ((c>=0) and (c<invadersPerRow)) {
      return c;
    }
    return -1;
  }
  const Invader *getColumn(int8_t c) {
    while ((c>=0) and (c<numInvaders)) {
      const Invader *v = invader+c;
      if (v->isAlive()) {
        return v;
      }
      c += invadersPerRow;
    }
    return 0;
  }
  bool haveLanded() {
    return anInvaderReachedTheBottom;
  }
  void draw() {
    for (int i = 0; i < numInvaders; i++) {
      GD.Tag(100 + i);
      invader[i].draw();
    }
  }
  void hit(int tag) {
    int c = tag - 100;
    if ((0 <= c) && (c < numInvaders)) {
      kill(c);
      bullet.reset();
    }
  }
} invaders;

bool killInvader(byte n)
{
  return invaders.kill(n);
}

/*---------------------------------------------------------
  Space invader bombs
  
  There's three bombs in Space Invaders. Two of them
  follow a pattern of columns, the other one always
  appears right above the player (to stop you getting
  bored...!)
  
  Mantra: There are NO random numbers in Space Invaders...

  nb. Column 1 is the most dangerous and column 5
      isn't in either table... :-)
---------------------------------------------------------*/
// Column table for the 'zigzag' bomb
static const PROGMEM int8_t zigzagBombColumns[] = {
  11,1,6,3,1,1,11,9,2,8,2,11,4,7,10,-1
};
// Column table for the bomb with horizontal bars across it
static const PROGMEM int8_t barBombColumns[] = {
  1,7,1,1,1,4,11,1,6,3,1,1,11,9,2,8,-1
};
byte bombTimer;    // Countdown until next bomb can be dropped
void resetBombTimer()
{
  if (!player.isAlive()) {
    bombTimer = 60;    // We don't drop for this long after you reanimate
  }
  else {
    // You get more bombs as the game progresses :-)
    if (playerScore < 200)       { bombTimer = 48;  }
    else if (playerScore < 1000) { bombTimer = 16;  }
    else if (playerScore < 2000) { bombTimer = 11;  }
    else if (playerScore < 3000) { bombTimer = 8;   }
    else                         { bombTimer = 7;   }
  }
}
class Bomb : public GameObject {
  byte graphic;
  byte timer;
  byte cycle;
  const PROGMEM int8_t *columnTable, *tablePtr;
  bool readyToDrop() {
    return (bombTimer==0);
  }
  int8_t getNextColumn() {
    int8_t c = pgm_read_byte(tablePtr);
    if (c == -1) {
      tablePtr = columnTable;
      c = pgm_read_byte(tablePtr);
    }
    else {
      ++tablePtr;
    }
    return c-1;
  }
public:
  Bomb() {
    tablePtr = 0;
  }
  bool isAlive() {
    return (status!=S_WAITING);
  }
  void die() {
    status = S_DYING;
    timer = 12;
  }
  void reset(byte gr, const int8_t *ct) {
    initialize();
    graphic = gr;
    columnTable = ct;
    if (!tablePtr) {
      tablePtr = ct;  // Only set this the first time...
    }
    cycle = timer = 0;
  }
  void update() {
    switch (status) {
      case S_WAITING: if (bombTimer == 0) {
                        int8_t c = -1;
                        if (columnTable) {
                          // Follow sequence of columns
                          c = getNextColumn();
                        }
                        else {
                          // Drop me above the player
                          c = invaders.nearestColumnToPlayer();
                        }
                        const Invader *v = invaders.getColumn(c);
                        if (v) {
                          status = S_ALIVE;
                          xpos = v->xpos;
                          ypos = v->ypos+8;
                          resetBombTimer();
                        }
                      }
                      break;
      case S_ALIVE:   ypos += bombSpeed;
                      if (ypos < bombYmax) {
                        if (shields.zap(xpos, ypos, false))
                          die();
                      } else {
                        ypos = bombYmax;
                        die();
                      }
                      if (++timer==2) {
                        ++cycle;
                        timer = 0;
                      }
                      frame = graphic + (cycle & 3);
                      break;
      case S_DYING:   if (!--timer) {
                        status = S_WAITING;
                      }
                      else {
                        frame = CELL_BOMB_BLAST;
                      }
                      break;
    }
  }
  void collide() {
    if (status==S_ALIVE) {
      byte b = collision();
      /*
      if (b == SP_PLAYER) {
        player.kill();
        status = S_DYING;
      }
      if ((b>=SP_FIRST_SHIELD) and (b<=SP_LAST_SHIELD)) {
        shields.bomb(b,xpos);
        die();
      }
      */
    }
  }
};

class Bombs {
  Bomb zigzag,bar,diag;
public:
  void reset() {
    resetBombTimer();
    zigzag.reset(CELL_ZIGZAGBOMB, zigzagBombColumns);
    bar   .reset(CELL_BARBOMB,    barBombColumns);
    diag  .reset(CELL_DIAGBOMB,   0);
  }
  void update() {
    if (player.isAlive()) {
      if (bombTimer > 0) {
        --bombTimer;
      }
      zigzag.update();
      bar   .update();
      diag  .update();
    }
  }
  void collide() {
    zigzag.collide();
    bar   .collide();
    diag  .collide();
  }
  void draw() {
    zigzag.draw();
    bar   .draw();
    diag  .draw();
  }
  void shoot(byte s) {
    if (zigzag.sprite==s) zigzag.die();
    if (bar.sprite   ==s) bar.die();
    if (diag.sprite  ==s) diag.die();
  }
} bombs;

void shootBomb(byte s)
{
  bombs.shoot(s);
}
/*---------------------------------------------
  Start next wave of invaders
---------------------------------------------*/
void startNextWave()
{
  beatCounter = 0;
  player.reset();
  bullet.reset();
  saucer.reset();
  bombs.reset();
  shields.reset();
  invaders.reset(invaderWave);
  if (++invaderWave == 0) {
    invaderWave = 1;
  }
}

/*---------------------------------------------
  Reset the game
---------------------------------------------*/
void resetGame()
{
  numLives = 3;
  playerScore = 0;
  invaderWave = 0;
  startNextWave();
  // GD.fill((64*((screenTop+239)>>3))+(screenLeft>>3),CH_FLOOR,screenWidth>>3);
}

uint32_t clock() {
  return micros();
  return GD.rd32(REG_CLOCK);
}

void loop()
{
  ++frameCounter;

  GD.get_inputs();
  int x, y, z;
  GD.get_accel(x, y, z);
  byte tag = GD.rd(REG_TAG);
  // tag = GD.rd(REG_TOUCH_TAG);
  long t0 = clock();
  GD.Clear();
  GD.Begin(BITMAPS);
  GD.ColorMask(1, 1, 1, 0);
  GD.ColorRGB(0x686868);
  GD.Vertex2ii(screenLeft - (248 - screenWidth) / 2, 0, 2, 0);
  GD.ColorMask(0, 0, 0, 1);
  GD.BlendFunc(ONE, ONE);
  GD.AlphaFunc(GREATER, 0);

  // Collision detection first (we have to do it all during vertical blanking!)
  bullet.collide();
  bombs.collide();
  // The rest of the game logic
  // joystick.read();
  player.update();
  bullet.update();
  saucer.update();
  bombs.update();
  shields.update();
  invaders.update();
  if (!remainingInvaders) {
    startNextWave();
  }
  if (player.isDying()) {
    bombs.reset();
    bullet.reset();
  }
  if (player.isDead()) {
    resetGame();
  }
  if (invaders.haveLanded()) {
    numLives = 1;
    player.kill();
  }

  saucer.draw();
  invaders.draw();
  invaders.hit(tag);
  bombs.draw();
  shields.draw();
  GD.TagMask(0);
  player.draw();
  bullet.draw();

  updateScore();
  drawBases();

  GD.ColorRGB(0xc0c0c0);
  GD.ColorMask(1, 1, 1, 0);
  GD.BlendFunc(DST_ALPHA, ONE);

  GD.cmd_loadidentity();
  GD.cmd_scale(F16(8), F16(8));
  GD.cmd_setmatrix();
  GD.Vertex2ii(screenLeft, screenTop, 3, 0);

  if (--beatCounter < 0) {
    // alienBeatSound = true;
    beatCounter = remainingInvaders+4;
  }

  static long tprev;
  GD.RestoreContext();
  // GD.cmd_number(60, 0, 26, OPT_RIGHTX, clock() - t0);
  // GD.cmd_number(60, 0, 26, OPT_RIGHTX, tag);
  tprev = t0;
  GD.swap();
}

void setup()
{
  Serial.begin(UART_SPEED);
  GD.begin();

  GD.cmd_inflate(0);
  GD.copy(invaders_assets, sizeof(invaders_assets));

  GD.BitmapHandle(0);
  GD.BitmapSource(SPR16);
  GD.BitmapSize(NEAREST, BORDER, BORDER, 16, 8);
  GD.BitmapLayout(L1, 2, 8);

  GD.BitmapHandle(1);
  GD.BitmapSource(SAUCER);
  GD.BitmapSize(NEAREST, BORDER, BORDER, 16, SAUCER_HEIGHT);
  GD.BitmapLayout(L1, SAUCER_WIDTH / 8, 8);

  GD.BitmapHandle(2);
  GD.cmd_loadimage(ASSETS_END, 0);
  GD.copy(background_jpg, sizeof(background_jpg));

  GD.BitmapHandle(3);
  GD.BitmapSource(OVERLAY);
  GD.BitmapSize(NEAREST, BORDER, BORDER, 8 * OVERLAY_WIDTH, 8 * OVERLAY_HEIGHT);
  GD.BitmapLayout(RGB332, OVERLAY_WIDTH, OVERLAY_HEIGHT);

  GD.BitmapHandle(4);
  GD.BitmapSource(SHIELDS);
  GD.BitmapSize(NEAREST, BORDER, BORDER, SHIELDS_WIDTH, SHIELDS_HEIGHT);
  GD.BitmapLayout(L1, SHIELDS_WIDTH / 8, SHIELDS_HEIGHT);

  resetGame();

  highScore = 0;
}
