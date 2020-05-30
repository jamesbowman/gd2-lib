#include <EEPROM.h>
#include <SPI.h>
#include <GD2.h>

#include "cobra_assets.h"

////////////////////////////////////////////////////////////////////////////////
//                                  3D Projection
////////////////////////////////////////////////////////////////////////////////

static byte VXSCALE = 16;

static float model_mat[9] = { 1.0, 0.0, 0.0,
                        0.0, 1.0, 0.0,
                        0.0, 0.0, 1.0 };
static float normal_mat[9] = { 1.0, 0.0, 0.0,
                        0.0, 1.0, 0.0,
                        0.0, 0.0, 1.0 };

#define M(nm,i,j)       ((nm)[3 * (i) + (j)])

void mult_matrices(float *a, float *b, float *c)
{
  int i, j, k;
  float result[9];
  for(i = 0; i < 3; i++) {
    for(j = 0; j < 3; j++) {
      M(result,i,j) = 0.0f;
      for(k = 0; k < 3; k++) {
        M(result,i,j) +=  M(a,i,k) *  M(b,k,j);
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
  float mat[9];
  float mati[9];

  rotate(mat, mati, angle, axis);
  mult_matrices(model_mat, mat, model_mat);
  mult_matrices(mati, normal_mat, normal_mat);
}

#if 0 // JCB{
class Vector3
{
public:
    float x, y, z;

    void set(float _x, float _y, float _z) {
        x = _x;
        y = _y;
        z = _z;
    }

    // functions
    void normalize() {
      float invLength = 1 / sqrtf(x*x + y*y + z*z);
      x *= invLength;
      y *= invLength;
      z *= invLength;
    }
    Vector3& operator-=(const Vector3& rhs) {
      x -= rhs.x;
      y -= rhs.y;
      z -= rhs.z;
    }
    void cross(const Vector3& rhs) {
        float _x = y*rhs.z - z*rhs.y;
        float _y = z*rhs.x - x*rhs.z;
        float _z = x*rhs.y - y*rhs.x;
        set(_x, _y, _z);
    }
};
#endif
// }JCB

#define N_VERTICES  (sizeof(COBRA_vertices) / 3)

typedef struct {
  int x, y;
  float z;
} xyz;

static xyz projected[N_VERTICES];

void project(float distance)
{
  const PROGMEM int8_t *pm = COBRA_vertices; 
  const PROGMEM int8_t *pm_e = pm + sizeof(COBRA_vertices);
  xyz *dst = projected;
  int8_t x, y, z;
  int hw = GD.w / 2;

  while (pm < pm_e) {
    x = pgm_read_byte_near(pm++);
    y = pgm_read_byte_near(pm++);
    z = pgm_read_byte_near(pm++);
    float xx = x * model_mat[0] + y * model_mat[3] + z * model_mat[6];
    float yy = x * model_mat[1] + y * model_mat[4] + z * model_mat[7];
    float zz = x * model_mat[2] + y * model_mat[5] + z * model_mat[8] + distance;
    float q = hw / (100 + zz);
    dst->x = VXSCALE * (hw       + xx * q);
    dst->y = VXSCALE * (GD.h / 2 + yy * q);
    dst->z = zz;
    dst++;
  }
}

static void transform_normal(int8_t &nx, int8_t &ny, int8_t &nz)
{
  int8_t xx = nx * normal_mat[0] + ny * normal_mat[1] + nz * normal_mat[2];
  int8_t yy = nx * normal_mat[3] + ny * normal_mat[4] + nz * normal_mat[5];
  int8_t zz = nx * normal_mat[6] + ny * normal_mat[7] + nz * normal_mat[8];
  nx = xx;
  ny = yy;
  nz = zz;
}

#define EDGE_BYTES  5
static byte visible_edges[EDGE_BYTES];

void draw_faces()
{
  memset(visible_edges, 0, sizeof(visible_edges));

  const PROGMEM int8_t *p = COBRA_faces; 
  byte n;
  int c = 1;
  Poly po;
  while ((n = pgm_read_byte_near(p++)) != 0xff) {
    int8_t nx = pgm_read_byte_near(p++);
    int8_t ny = pgm_read_byte_near(p++);
    int8_t nz = pgm_read_byte_near(p++);
    byte face_edges[EDGE_BYTES];
    for (byte i = 0; i < EDGE_BYTES; i++)
      face_edges[i] = pgm_read_byte_near(p++);
    byte v1 = pgm_read_byte_near(p);
    byte v2 = pgm_read_byte_near(p + 1);
    byte v3 = pgm_read_byte_near(p + 2);
    long x1 = projected[v1].x;
    long y1 = projected[v1].y;
    long x2 = projected[v2].x;
    long y2 = projected[v2].y;
    long x3 = projected[v3].x;
    long y3 = projected[v3].y;
    long area = (x1 - x3) * (y2 - y1) - (x1 - x2) * (y3 - y1);

    if (area > 0) {
      for (byte i = 0; i < EDGE_BYTES; i++)
        visible_edges[i] |= face_edges[i];
      po.begin();
      for (int i = 0; i < n; i++) {
        byte vi = pgm_read_byte_near(p++);
        xyz *v = &projected[vi];
        po.v(v->x, v->y);
      }
      {
        transform_normal(nx, ny, nz);

        uint16_t r = 10, g = 10, b = 20;  // Ambient

        int d = -ny;                      // diffuse light from +ve Y
        if (d > 0) {
          r += d >> 2;
          g += d >> 1;
          b += d;
        }
                                          // use specular half angle
        d = ny * -90 + nz * -90;          // Range -16384 to +16384
        if (d > 8192) {
          byte l = pgm_read_byte_near(shiny + ((d - 8192) >> 4));
          r += l;
          g += l;
          b += l;
        }

        /*  JCB{
        d = nx;
        if (d > 0)  
          r += d >> 3;
        d = -nx;
        if (d > 0) 
          g += d >> 3; }JCB */  
        GD.ColorRGB(min(255, r), min(255, g), min(255, b));
      }
      po.draw();
    } else {
      p += n;
    }
    c += 1;
  }
}

void draw_edges()
{
  GD.ColorRGB(0x2e666e);
  GD.Begin(LINES);
  GD.LineWidth(20);

  const PROGMEM uint8_t *p = COBRA_edges; 
  byte *pvis = visible_edges;
  byte vis = 0;

  for (byte i = 0; i < sizeof(COBRA_edges) / 2; i++) {
    if ((i & 7) == 0)
      vis = *pvis++;
    byte v0 = pgm_read_byte_near(p++);
    byte v1 = pgm_read_byte_near(p++);

    if (vis & 1) {
      int x0 = projected[v0].x;
      int y0 = projected[v0].y;
      int x1 = projected[v1].x;
      int y1 = projected[v1].y;

      GD.Vertex2f(x0,y0);
      GD.Vertex2f(x1,y1);
    }
    vis >>= 1;
  }
}

static void draw_navlight(byte nf)
{
  float l0z = projected[N_VERTICES - 2].z;
  float l1z = projected[N_VERTICES - 1].z;
  byte i;
  if (nf == 0)  // draw the one with smallest z
    i = (l0z < l1z) ? (N_VERTICES - 2) : (N_VERTICES - 1);
  else
    i = (l0z < l1z) ? (N_VERTICES - 1) : (N_VERTICES - 2);

  GD.SaveContext();
  GD.BlendFunc(SRC_ALPHA, ONE);
  GD.Begin(BITMAPS);
  GD.BitmapHandle(LIGHT_HANDLE);
 
  GD.ColorRGB((i == N_VERTICES - 2) ? 0xfe2b18 : 0x4fff82);
  GD.Vertex2f(projected[i].x - (VXSCALE * LIGHT_WIDTH / 2),
              projected[i].y - (VXSCALE * LIGHT_WIDTH / 2));
  GD.RestoreContext();
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


void setup()
{
  teensy_sync();
  GD.begin();
  LOAD_ASSETS();
  GD.BitmapHandle(BACKGROUND_HANDLE);
  GD.BitmapSize(BILINEAR, REPEAT, REPEAT, GD.w, GD.h);
// #ifdef DUMPDEV    // JCB{
  startMotion(240, 136);
  trackMotion(243, 138);
// #endif          // }JCB
  startMotion(240, 136);
  trackMotion(243, 139);
}

static byte prev_touching;
static uint16_t t;

static void draw_sun(int x, int y, int rot)
{
  GD.cmd_loadidentity();
  GD.cmd_translate(F16(SUN_WIDTH / 2), F16(SUN_WIDTH / 2));
  GD.cmd_rotate(rot);
  GD.cmd_translate(-F16(SUN_WIDTH / 2), -F16(SUN_WIDTH / 2));
  GD.cmd_setmatrix();
  GD.Vertex2f(x - (VXSCALE * SUN_WIDTH / 2), y - (VXSCALE * SUN_WIDTH / 2));
}

void loop()
{
  if (ft8xx_model == 2) {
    GD.VertexFormat(3);
    GD.vxf = 3;
    VXSCALE = 8;
  }
  GD.Begin(BITMAPS);
  GD.SaveContext();
  GD.SaveContext();
  GD.BitmapHandle(BACKGROUND_HANDLE);
  GD.cmd_translate(-(long)t << 14, (long)t << 13);
  GD.cmd_rotate(3312);
  GD.cmd_setmatrix();
  GD.Vertex2ii(0, 0, 0, 0);
  GD.RestoreContext();

  int et = t - 0;
  int sun_x = (GD.w * VXSCALE) - (et << 2),
      sun_y = (100 * VXSCALE) + (et << 1);
  GD.SaveContext();
  GD.PointSize(52 * 16);
  GD.ColorRGB(0x000000);
  GD.Begin(POINTS);
  GD.Vertex2f(sun_x, sun_y);
  GD.RestoreContext();

  GD.SaveContext();
  GD.Begin(BITMAPS);
  GD.BlendFunc(ONE, ONE);
  GD.BitmapHandle(SUN_HANDLE);
  GD.ColorRGB(0xb0a090);
  draw_sun(sun_x, sun_y, t << 6);
  draw_sun(sun_x, sun_y, -t << 6);
  GD.RestoreContext();

  GD.get_inputs();
  byte touching = (GD.inputs.x != -32768);
  if (!prev_touching && touching)
    startMotion(GD.inputs.x, GD.inputs.y);
  else if (touching)
    trackMotion(GD.inputs.x, GD.inputs.y);
  prev_touching = touching;

  unsigned long t0 = micros();

  if (angle != 0.0f)
    rotation(angle, axis);

  project(0);
  draw_navlight(1);
  draw_faces();
  GD.RestoreContext();
  draw_edges();
  draw_navlight(0);
  GD.RestoreContext();
#ifndef DUMPDEV // JCB{
  GD.cmd_number(GD.w / 2, 7, 26, OPT_CENTER, micros() - t0);
#else
  GD.cmd_number(GD.w / 2, 7, 26, OPT_CENTER, t);
#endif        // }JCB

  GD.swap();
  if (t & 1)
    GD.dumpscreen();

  t++;
}
