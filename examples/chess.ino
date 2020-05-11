#include <EEPROM.h>
#include <SPI.h>
#include <GD2.h>

#include "chess_assets.h"

byte sf = 1;

static byte sinus(byte x)
{
  return 128 + GD.rsin(128, -16384 + (x << 7));
}

void setup()
{
  Serial.begin(1000000);    // JCB
  GD.begin(~GD_STORAGE);
  LOAD_ASSETS();

  sf = GD.h / 272;

  GD.BitmapHandle(0);
  GD.BitmapSize(BILINEAR, BORDER, BORDER, sf * 32, sf * 32);
  GD.BitmapHandle(1);
  GD.BitmapSource(CHECKER);
  GD.BitmapSize(NEAREST, REPEAT, REPEAT, sf * 256, sf * 256);
  GD.BitmapLayout(L8, CHECKER_WIDTH, CHECKER_HEIGHT);
}

/***************************************************************************/
/*                               micro-Max,                                */
/* A chess program smaller than 2KB (of non-blank source), by H.G. Muller  */
/* Port to Atmel ATMega644 and AVR GCC, by Andre Adrian                    */
/***************************************************************************/
/* version 4.8 (1953 characters) features:                                 */
/* - recursive negamax search                                              */
/* - all-capture MVV/LVA quiescence search                                 */
/* - (internal) iterative deepening                                        */
/* - best-move-first 'sorting'                                             */
/* - a hash table storing score and best move                              */
/* - futility pruning                                                      */
/* - king safety through magnetic, frozen king in middle-game              */
/* - R=2 null-move pruning                                                 */
/* - keep hash and repetition-draw detection                               */
/* - better defense against passers through gradual promotion              */
/* - extend check evasions in inner nodes                                  */
/* - reduction of all non-Pawn, non-capture moves except hash move (LMR)   */
/* - full FIDE rules (expt under-promotion) and move-legality checking     */

/* 26nov2008 no hash table                                                 */
/* 29nov2008 all IO via myputchar(), mygetchar(), pseudo random generator  */

#define VERBOSE

#define W while
#define M 0x88
#define S 128
#define I 8000

long N, T;                  /* N=evaluated positions+S, T=recursion limit */
short Q,O,K,R,k=16;        /* k=moving side */
char *p,c[5],Z;   /* p=pointer to c, c=user input, computer output, Z=recursion counter */

signed char L,
w[]={0,2,2,7,-1,8,12,23},                      /* relative piece values    */
o[]={-16,-15,-17,0,1,16,0,1,16,15,17,0,14,18,31,33,0, /* step-vector lists */
     7,-1,11,6,8,3,6,                          /* 1st dir. in o[] per piece*/
     6,3,5,7,4,5,3,6},                         /* initial piece setup      */
b[]={     /* board is left part, center-pts table is right part, and dummy */
  22, 19, 21, 23, 20, 21, 19, 22, 28, 21, 16, 13, 12, 13, 16, 21, 
  18, 18, 18, 18, 18, 18, 18, 18, 22, 15, 10,  7,  6,  7, 10, 15, 
   0,  0,  0,  0,  0,  0,  0,  0, 18, 11,  6,  3,  2,  3,  6, 11, 
   0,  0,  0,  0,  0,  0,  0,  0, 16,  9,  4,  1,  0,  1,  4,  9, 
   0,  0,  0,  0,  0,  0,  0,  0, 16,  9,  4,  1,  0,  1,  4,  9, 
   0,  0,  0,  0,  0,  0,  0,  0, 18, 11,  6,  3,  2,  3,  6, 11, 
   9,  9,  9,  9,  9,  9,  9,  9, 22, 15, 10,  7,  6,  7, 10, 15, 
  14, 11, 13, 15, 12, 13, 11, 14, 28, 21, 16, 13, 12, 13, 16, 21, 0
};

volatile char breakpoint; /* for debugger */
                                
/* User interface routines */
void myputchar(char c) {
#ifdef DUMPDEV    // JCB{
  fprintf(stderr, "%c", c);
#endif    // }JCB
}

void myputs(const char *s) {
  while(*s) myputchar(*s++);
  myputchar('\n');
}

char mygetchar(void) {
  return 10;                          /* self play computer with computer */
#ifdef VERBOSE
  return getchar();
#else  
  return 10;                          /* self play computer with computer */
#endif  
}

/* 16bit pseudo random generator */
#define MYRAND_MAX 65535

unsigned short r = 1;                     /* pseudo random generator seed */

void mysrand(unsigned short r_) {
 r = r_;
}

unsigned short myrand(void) {
 return r=((r<<11)+(r<<7)+r)>>1;
}

static void sp()
{
  void* x;
  x = &x;
  Serial.println((size_t)x, HEX);
}

short D(short q, short l, short e, byte E,byte z, byte n)                          /* recursive minimax search */
     /* (q,l)=window, e=current eval. score, */
     /* E=e.p. sqr.z=prev.dest, n=depth; return score */        
{                       
 short m,v,i,P,V,s;
 unsigned char t,p,u,x,y,X,Y,H,B,j,d,h,F,G,C;
 signed char r;

 // if (0) { REPORT(Z); sp(); }
 if (++Z>30) {                                     /* stack underrun check */
  breakpoint=1;               /* AVR Studio 4 Breakpoint for stack underrun */
  myputchar('u');
  --Z;return e;                                    
 }
 
 q--;                                          /* adj. window: delay bonus */
 k^=24;                                        /* change sides             */
 d=Y=0;                                        /* start iter. from scratch */
 X=myrand()&~M;                                /* start at random field    */
 W(d++<n||d<3||                                /* iterative deepening loop */
   z&K==I&&(N<T&d<98||                         /* root: deepen upto time   */
   (K=X,L=Y&~M,d=3)))                          /* time's up: go do best    */
 {x=B=X;                                       /* start scan at prev. best */
  h=Y&S;                                       /* request try noncastl. 1st*/
  P=d<3?I:D(-l,1-l,-e,S,0,d-3);                /* Search null move         */
  m=-P<l|R>35?d>2?-I:e:-P;                     /* Prune or stand-pat       */
  ++N;                                         /* node count (for timing)  */
  do{u=b[x];                                   /* scan board looking for   */
   if(u&k)                                     /*  own piece (inefficient!)*/
   {r=p=u&7;                                   /* p = piece type (set r>0) */
    j=o[p+16];                                 /* first step vector f.piece*/
    W(r=p>2&r<0?-r:-o[++j])                    /* loop over directions o[] */
    {A:                                        /* resume normal after best */
     y=x;F=G=S;                                /* (x,y)=move, (F,G)=castl.R*/
     do{                                       /* y traverses ray, or:     */
      H=y=h?Y^h:y+r;                           /* sneak in prev. best move */
      if(y&M)break;                            /* board edge hit           */
      m=E-S&b[E]&&y-E<2&E-y<2?I:m;             /* bad castling             */
      if(p<3&y==E)H^=16;                       /* shift capt.sqr. H if e.p.*/
      t=b[H];if(t&k|p<3&!(y-x&7)-!t)break;     /* capt. own, bad pawn mode */
      i=37*w[t&7]+(t&192);                     /* value of capt. piece t   */
      m=i<0?I:m;                               /* K capture                */
      if(m>=l&d>1)goto C;                      /* abort on fail high       */
      v=d-1?e:i-p;                             /* MVV/LVA scoring          */
      if(d-!t>1)                               /* remaining depth          */
      {v=p<6?b[x+8]-b[y+8]:0;                  /* center positional pts.   */
       b[G]=b[H]=b[x]=0;b[y]=u|32;             /* do move, set non-virgin  */
       if(!(G&M))b[F]=k+6,v+=50;               /* castling: put R & score  */
       v-=p-4|R>29?0:20;                       /* penalize mid-game K move */
       if(p<3)                                 /* pawns:                   */
       {v-=9*((x-2&M||b[x-2]-u)+               /* structure, undefended    */
              (x+2&M||b[x+2]-u)-1              /*        squares plus bias */
             +(b[x^16]==k+36))                 /* kling to non-virgin King */
             -(R>>2);                          /* end-game Pawn-push bonus */
        V=y+r+1&S?647-p:2*(u&y+16&32);         /* promotion or 6/7th bonus */
        b[y]+=V;i+=V;                          /* change piece, add score  */
       }
       v+=e+i;V=m>q?m:q;                       /* new eval and alpha       */
       C=d-1-(d>5&p>2&!t&!h);
       C=R>29|d<3|P-I?C:d;                     /* extend 1 ply if in check */
       do
        s=C>2|v>V?-D(-l,-V,-v,                 /* recursive eval. of reply */
                              F,0,C):v;        /* or fail low if futile    */
       W(s>q&++C<d);v=s;
       if(z&&K-I&&v+I&&x==K&y==L)              /* move pending & in root:  */
       {Q=-e-i;O=F;                            /*   exit if legal & found  */
        R+=i>>7;--Z;return l;                  /* captured non-P material  */
       }
       b[G]=k+6;b[F]=b[y]=0;b[x]=u;b[H]=t;     /* undo move,G can be dummy */
      }
      if(v>m)                                  /* new best, update max,best*/
       m=v,X=x,Y=y|S&F;                        /* mark double move with S  */
      if(h){h=0;goto A;}                       /* redo after doing old best*/
      if(x+r-y|u&32|                           /* not 1st step,moved before*/
         p>2&(p-4|j-7||                        /* no P & no lateral K move,*/
         b[G=x+3^r>>1&7]-k-6                   /* no virgin R in corner G, */
         ||b[G^1]|b[G^2])                      /* no 2 empty sq. next to R */
        )t+=p<5;                               /* fake capt. for nonsliding*/
      else F=y;                                /* enable e.p.              */
     }W(!t);                                   /* if not capt. continue ray*/
  }}}W((x=x+9&~M)-B);                          /* next sqr. of board, wrap */
C:if(m>I-M|m<M-I)d=98;                         /* mate holds to any depth  */
  m=m+I|P==I?m:0;                              /* best loses K: (stale)mate*/
  if(z&&d>2)
   {*c='a'+(X&7);c[1]='8'-(X>>4);c[2]='a'+(Y&7);c[3]='8'-(Y>>4&7);c[4]=0;
    breakpoint=2;           /* AVR Studio 4 Breakpoint for moves, watch c[] */
#ifdef DUMPDEV    // JCB{
    fprintf(stderr, "%2d ply, %9d searched, score=%6d by %c%c%c%c\n",d-1,N-S,m,
     'a'+(X&7),'8'-(X>>4),'a'+(Y&7),'8'-(Y>>4&7)); /* uncomment for Kibitz */   
#else
    REPORT(d);
    Serial.println(c);
#endif          // }JCB
  }  
 }                                             /*    encoded in X S,8 bits */
 k^=24;                                        /* change sides back        */
 --Z;return m+=m<e;                            /* delayed-loss bonus       */
}

void print_board()
{
 short N=-1;
 W(++N<121)
  myputchar(N&8&&(N+=7)?10:".?inkbrq?I?NKBRQ"[b[N]&15]);      /* Pawn is i */
}

static void p2(int &x, int &y, const char *c)
{
  x = 16 * (32 + sf * 32 * (c[0] - 'a'));
  y = 16 * ( 8 + sf * 32 * ('8' - c[1]));
}

#define MOVETIME    30
#define OVERSAMPLE  8

int clocks[2];

void draw_board()
{
//                .  ?  i  n  k  b  r  q  ?  I  ?  N  K  B  R  Q
  byte xlat[] = { 0, 0, 0, 1, 5, 2, 3, 4, 0, 6, 0, 7, 11,8, 9,10 };

  for (int i = 0; i < MOVETIME; i++) {
    GD.cmd_gradient(0, 0, 0x101010, GD.w, GD.h, 0x202060);
    GD.cmd_bgcolor(0x101020);

    GD.Begin(BITMAPS);
    GD.SaveContext();
    GD.ColorRGB(0xfff0c0);
    GD.ColorA(0xc0);
    GD.cmd_scale(F16(sf * 32), F16(sf * 32));
    GD.cmd_setmatrix();
    GD.Vertex2ii(32, 8, 1, 0);
    GD.RestoreContext();

    int moving = -1;
    if (c[0])
      moving = 16 * ('8' - c[3]) + (c[2] - 'a');

    GD.SaveContext();
    GD.ColorRGB(0xe0e0e0);
    GD.cmd_loadidentity();
    GD.cmd_scale(F16(sf), F16(sf));
    GD.cmd_setmatrix();
    for (int y = 0; y < 8; y++)
      for (int x = 0; x < 8; x++) {
        int pos = 16 * y + x;
        byte piece = b[pos] & 15;
        if (pos != moving && piece != 0)
          GD.Vertex2ii(32 + sf * 32 * x, 8 + sf * 32 * y, 0, xlat[piece]);
      }

    if (c[0]) {
      GD.Begin(BITMAPS);
      byte piece = b[moving] & 15;
      GD.Cell(xlat[piece]);
      int x0, y0, x1, y1;
      p2(x0, y0, c + 0);
      p2(x1, y1, c + 2);
      GD.ColorRGB(0xffffff);    //' a{
      GD.ColorA((255 / OVERSAMPLE) + 50);
      for (int j = 0; j < OVERSAMPLE; j++) {
        byte linear = 255 * (i * OVERSAMPLE + j) /
                            (OVERSAMPLE * MOVETIME - 1);
        byte scurve = sinus(linear);
        int x = x0 + (long)(x1 - x0) * scurve / 255;
        int y = y0 + (long)(y1 - y0) * scurve / 255;

        GD.Vertex2f(x, y);
      } //' }a
      GD.ColorA(255);
    }
    GD.RestoreContext();

    GD.ColorRGB((k == 16) ? 0xffffffUL : 0x606060);
    GD.cmd_clock(sf * 384, sf *  60, sf * 50, OPT_FLAT | OPT_NOSECS, 0, 0, clocks[0], 0);
    GD.ColorRGB((k != 16) ? 0xffffffUL : 0x606060);
    GD.cmd_clock(sf * 384, sf * (272 - 60), sf * 50, OPT_FLAT | OPT_NOSECS, 0, 0, clocks[1], 0);

    GD.ColorRGB(0xffffff);
    GD.cmd_text(sf * 384, sf * 136, 30, OPT_CENTER, c);

    GD.swap();
  }
}

void loop()
{

#ifdef VERBOSE  // JCB{
  print_board();
#endif          // }JCB
  draw_board();
  K=I;                                               /* invalid move       */
#if 0           // JCB{
  p=c;W((*p++=mygetchar())>10);                      /* read input line    */
  if(*c-10)K=*c-16*c[1]+799,L=c[2]-16*c[3]+799;      /* parse entered move */
#endif          // }JCB
  N=0;T=0x9;                                  /* T=Computer Play strength */
 long t0 = millis();
 if(!(D(-I,I,Q,O,1,3)>-I+1))                           /* think or check & do*/
   for (;;);
 int took = 60 * (millis() - t0) / 1000;
 clocks[(k==16)] += took;
}

