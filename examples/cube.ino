#include <EEPROM.h>
#include <SPI.h>
#include <GD2.h>

#undef REPORT
#define REPORT(VAR) (Serial.print(#VAR "="), Serial.println(VAR, DEC))

#define PICTURE_HANDLE 0

#define P 125
#define N -P

static const PROGMEM int8_t CUBE_vertices[] = {
  P,P,P,
  N,P,P,
  P,N,P,
  N,N,P,

  P,P,N,
  N,P,N,
  P,N,N,
  N,N,N,
};

// each line is a face: count, normal, 4 vertices

static const PROGMEM int8_t CUBE_faces[] = {
  4, 0,0,127,   0, 1, 3, 2,
  4, 0,0,-127,  6, 7, 5, 4,

  4, 0,127,0,   4, 5, 1, 0,
  4, 0,-127,0,  2, 3, 7, 6,

  4, 127,0,0,   0, 2, 6, 4,
  4, -127,0,0,  3, 1, 5, 7,

  -1
};

////////////////////////////////////////////////////////////////////////////////
//                                  3D Projection
////////////////////////////////////////////////////////////////////////////////

static float model_mat[9] = {
  1.0, 0.0, 0.0,
  0.0, 1.0, 0.0,
  0.0, 0.0, 1.0 };
static float normal_mat[9] = {
  1.0, 0.0, 0.0,
  0.0, 1.0, 0.0,
  0.0, 0.0, 1.0 };

#define M(nm,i,j)       ((nm)[3 * (i) + (j)])

// 3x3 matrix multiplication: c = a * b
void mult_matrices(float *a, float *b, float *c)
{
  int i, j, k;
  float result[9];
  for(i = 0; i < 3; i++) {
    for(j = 0; j < 3; j++) {
      M(result,i,j) = 0.0f;
      for(k = 0; k < 3; k++) {
        M(result,i,j) += M(a,i,k) * M(b,k,j);
      }
    }
  }
  memcpy(c, result, sizeof(result));
}

// Based on glRotate()
// Returns 3x3 rotation matrix in 'm'
// and its invese in 'mi'

static void rotate(float *m, float *mi, float angle, float *axis)
{
  float x = axis[0];
  float y = axis[1];
  float z = axis[2];

  float s = sin(angle);
  float c = cos(angle);

  float xx = x*x*(1-c);
  float xy = x*y*(1-c);
  float xz = x*z*(1-c);
  float yy = y*y*(1-c);
  float yz = y*z*(1-c);
  float zz = z*z*(1-c);

  float xs = x * s;
  float ys = y * s;
  float zs = z * s;

  m[0] = xx + c;
  m[1] = xy - zs;
  m[2] = xz + ys;

  m[3] = xy + zs;
  m[4] = yy + c;
  m[5] = yz - xs;

  m[6] = xz - ys;
  m[7] = yz + xs;
  m[8] = zz + c;

  mi[0] = m[0];
  mi[1] = xy + zs;
  mi[2] = xz - ys;

  mi[3] = xy - zs;
  mi[4] = m[4];
  mi[5] = yz + xs;

  mi[6] = xz + ys;
  mi[7] = yz - xs;
  mi[8] = m[8];
}

static void rotation(float angle, float *axis)
{
  float mat[9], mati[9];

  rotate(mat, mati, angle, axis);
  mult_matrices(model_mat, mat, model_mat);
  mult_matrices(mati, normal_mat, normal_mat);
}

#define N_VERTICES  (sizeof(CUBE_vertices) / 3)

typedef struct {
  int x, y;
} point2;

static point2 projected[N_VERTICES];

void project()
{
  byte vx;
  const PROGMEM int8_t *pm = CUBE_vertices; 
  const PROGMEM int8_t *pm_e = pm + sizeof(CUBE_vertices);
  point2 *dst = projected;
  int16_t x, y, z;
  int scale = 64 * GD.h / 280;
  while (pm < pm_e) {
    x = (scale * (int8_t)pgm_read_byte_near(pm++)) >> 6;
    y = (scale * (int8_t)pgm_read_byte_near(pm++)) >> 6;
    z = (scale * (int8_t)pgm_read_byte_near(pm++)) >> 6;
    float xx = x * model_mat[0] + y * model_mat[3] + z * model_mat[6];
    float yy = x * model_mat[1] + y * model_mat[4] + z * model_mat[7];
    dst->x = 16 * (GD.w / 2 + xx);
    dst->y = 16 * (GD.h / 2 + yy);
    dst++;
  }
}

static void transform_normal(int8_t &nx, int8_t &ny, int8_t &nz)
{
  int8_t xx = nx * normal_mat[0] + ny * normal_mat[1] + nz * normal_mat[2];
  int8_t yy = nx * normal_mat[3] + ny * normal_mat[4] + nz * normal_mat[5];
  int8_t zz = nx * normal_mat[6] + ny * normal_mat[7] + nz * normal_mat[8];
  nx = xx, ny = yy, nz = zz;
}

static void quad(int x1, int y1,
                 int x2, int y2,
                 int x3, int y3,
                 int bx1, int by1,
                 int bx3, int by3)
{
  // Compute the fourth vertex of the parallelogram, (x4,y4)
  int x4 = x3 + (x1 - x2);
  int y4 = y3 + (y1 - y2);

  // Apply Scissor to the extents of the quad
  int minx = max(0,    min(min(x1, x2), min(x3, x4)));
  int maxx = min(GD.w, max(max(x1, x2), max(x3, x4)));
  int miny = max(0,    min(min(y1, y2), min(y3, y4)));
  int maxy = min(GD.h, max(max(y1, y2), max(y3, y4)));
  GD.ScissorXY(minx, miny);
  GD.ScissorSize(maxx - minx, maxy - miny);
  // GD.ClearColorRGB(0, 255, 0); GD.Clear();

  // Set the new bitmap transform
  GD.cmd32(0xffffff21UL); // bitmap transform
  GD.cmd32(x1 - minx); GD.cmd32(y1 - miny);
  GD.cmd32(x2 - minx); GD.cmd32(y2 - miny);
  GD.cmd32(x3 - minx); GD.cmd32(y3 - miny);

  GD.cmd32(bx1); GD.cmd32(by1);
  GD.cmd32(bx1); GD.cmd32(by3);
  GD.cmd32(bx3); GD.cmd32(by3);
  GD.cmd32(0);

  // Draw the quad
  GD.Vertex2f(PIXELS(minx), PIXELS(miny));
}

void draw_faces(int dir)
{
  int R = 15;
  const PROGMEM int8_t *p = CUBE_faces; 
  byte n;

  GD.BlendFunc(ONE, ONE_MINUS_SRC_ALPHA);
  GD.Begin(BITMAPS);

  while ((n = pgm_read_byte_near(p++)) != 0xff) {
    int8_t nx = pgm_read_byte_near(p++);
    int8_t ny = pgm_read_byte_near(p++);
    int8_t nz = pgm_read_byte_near(p++);
    byte v1 = pgm_read_byte_near(p);
    byte v2 = pgm_read_byte_near(p + 1);
    byte v3 = pgm_read_byte_near(p + 2);
    p += n;

    long x1 = projected[v1].x;
    long y1 = projected[v1].y;
    long x2 = projected[v2].x;
    long y2 = projected[v2].y;
    long x3 = projected[v3].x;
    long y3 = projected[v3].y;
    long area = (x1 - x3) * (y2 - y1) - (x1 - x2) * (y3 - y1);
    byte face = (area < 0);
    if (face == dir) {
      uint16_t r = 20, g = 20, b = 80;  // Ambient

      if (face) {
        transform_normal(nx, ny, nz);
        int d = max(0, -nz);                      // diffuse light from +ve Z
        r += 2 * d;
        g += 2 * d;
        b += 2 * d;
      }
      GD.ColorRGB(min(255, r), min(255, g), min(255, b));

      x1 >>= 4;
      y1 >>= 4;
      x2 >>= 4;
      y2 >>= 4;
      x3 >>= 4;
      y3 >>= 4;
      quad(x1, y1, x2, y2, x3, y3,
           -R, -R,
           128 + R, 128 + R);
    }
  }
}

/*****************************************************************/

/* simple trackball-like motion control */
/* Based on projtex.c - by David Yu and David Blythe, SGI */

float angle, axis[3] = {0,1,0};
float lastPos[3];

void
ptov(int x, int y, int width, int height, float v[3])
{
  float d, a;

  /* project x,y onto a hemi-sphere centered within width, height */
  v[0] = (2.0 * x - width) / width;
  v[1] = (2.0 * y - height) / height;
  d = sqrt(v[0] * v[0] + v[1] * v[1]);
  v[2] = cos((M_PI / 2.0) * ((d < 1.0) ? d : 1.0));
  a = 1.0 / sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
  v[0] *= a;
  v[1] *= a;
  v[2] *= a;
}

void
startMotion(int x, int y)
{
  angle = 0.0;
  ptov(x, y, GD.w, GD.h, lastPos);
}

void
trackMotion(int x, int y)
{
  float curPos[3], dx, dy, dz;

  ptov(x, y, GD.w, GD.h, curPos);

  dx = curPos[0] - lastPos[0];
  dy = curPos[1] - lastPos[1];
  dz = curPos[2] - lastPos[2];
  angle = (M_PI / 2) * sqrt(dx * dx + dy * dy + dz * dz);

  axis[0] = lastPos[1] * curPos[2] - lastPos[2] * curPos[1];
  axis[1] = lastPos[2] * curPos[0] - lastPos[0] * curPos[2];
  axis[2] = lastPos[0] * curPos[1] - lastPos[1] * curPos[0];

  float mag = 1 / sqrt(axis[0] * axis[0] + axis[1] * axis[1] + axis[2] * axis[2]);
  axis[0] *= mag;
  axis[1] *= mag;
  axis[2] *= mag;

  lastPos[0] = curPos[0];
  lastPos[1] = curPos[1];
  lastPos[2] = curPos[2];
}

/*****************************************************************/

Bitmap background, foreground;

void setup()
{
  Serial.begin(1000000); // JCB
  GD.begin();

  background.fromfile("tree.jpg");
  foreground.fromfile("healsky3.jpg");

  foreground.bind(PICTURE_HANDLE);
  GD.BitmapHandle(PICTURE_HANDLE);
  GD.BitmapSize(NEAREST, BORDER, BORDER, GD.w, GD.h);

  startMotion(240, 136);
  trackMotion(247, 138);
}

byte prev_touching;

void loop()
{
  GD.get_inputs();

  GD.Clear();
  GD.SaveContext();         // {
  GD.ColorRGB(0x605040);
  background.wallpaper();
  GD.RestoreContext();      // }

  if (!prev_touching && GD.inputs.touching)
    startMotion(GD.inputs.x, GD.inputs.y);
  else if (GD.inputs.touching)
    trackMotion(GD.inputs.x, GD.inputs.y);
  prev_touching = GD.inputs.touching;

  if (angle != 0.0f)
    rotation(angle, axis);

  project();
  draw_faces(0);
  draw_faces(1);
  GD.RestoreContext();

  GD.swap();
}
