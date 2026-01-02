#include "astro.h"
#include <math.h>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define DEG2RAD(x) ((x) * ((float)M_PI / 180.0f))

static void normalize(float *x, float *y, float *z)
{
    float len = sqrtf((*x)*(*x) + (*y)*(*y) + (*z)*(*z));
    if (len > 1e-6f)
    {
        *x /= len;
        *y /= len;
        *z /= len;
    }
}

static void cross(float ax, float ay, float az, float bx, float by, float bz,
                    float *ox, float *oy, float *oz)
{
    *ox = ay*bz - az*by;
    *oy = az*bx - ax*bz;
    *oz = ax*by - ay*bx;
}

static float dot(float ax, float ay, float az, float bx, float by, float bz)
{
    return ax*bx + ay*by + az*bz;
}

void astro_radec_to_unit(float ra_hours, float dec_deg, float *x, float *y, float *z)
{
    //RA in hours -> radians (24h = 360deg)
    float ra = DEG2RAD(ra_hours * 15.0f);
    float dec = DEG2RAD(dec_deg);

    // Standard celestial sphere to Cartesian
    float cd = cosf(dec);
    *x = cd * cosf(ra);
    *y = cd * sinf(ra);
    *z = sinf(dec);

    normalize(x, y, z);
}

void astro_camera_basis(float yaw_deg, float pitch_deg, float roll_deg,
                        float *rx, float *ry, float *rz,
                        float *ux, float *uy, float *uz,
                        float *fx, float *fy, float *fz)
{
    float yaw = DEG2RAD(yaw_deg);
    float pitch = DEG2RAD(pitch_deg);
    float roll = DEG2RAD(roll_deg);

    // Start with a forward vector looking down +Y
    float f0x = 0.f, f0y = 1.f, f0z = 0.f;
    float u0x = 0.f, u0y = 0.f, u0z = 1.f;

    // Apply yaw about z
    float cy = cosf(yaw), sy = sinf(yaw);
    float fx1 = cy*f0x - sy*f0y;
    float fy1 = sy*f0x + cy*f0y;
    float fz1 = f0z;

    float ux1 = cy*u0x - sy*u0y;
    float uy1 = sy*u0x + cy*u0y;
    float uz1 = u0z;

    // Apply pitch about x
    float cp = cosf(pitch), sp = sinf(pitch);
    float fx2 = fx1;
    float fy2 = cp*fy1 - sp*fz1;
    float fz2 = sp*fy1 + cp*fz1;

    float ux2 = ux1;
    float uy2 = cp*uy1 - sp*uz1;
    float uz2 = sp*uy1 + cp*uz1;

    // Apply roll about Y
    float cr = cosf(roll), sr = sinf(roll);
    *fx = cr*fx2 + sr*fz2;
    *fy = fy2;
    *fz = -sr*fx2 + cr*fz2;

    *ux = cr*ux2 + sr*uz2;
    *uy = uy2;
    *uz = -sr*ux2 + cr*uz2;

    normalize(fx, fy, fz);
    normalize(ux, uy, uz);

    // Right = forward x up
    cross(*fx, *fy, *fz, *ux, *uy, *uz, rx, ry, rz);
    normalize(rx, ry, rz);

    // Re-orthoganalize up = right x forward
    cross(*rx, *ry, *rz, *fx, *fy, *fz, ux, uy ,uz);
    normalize(ux, uy, uz);
}

int astro_project_dir(float dirx, float diry, float dirz,
                      float rx, float ry, float rz,
                      float ux, float uy, float uz,
                      float fx, float fy, float fz,
                      int w, int h, float fov_deg,
                      int *outx, int *outy, float *out_depth)
{
    // dir in camera coords using dot with basis vectors
    float cx = dot(dirx, diry, dirz, rx, ry, rz);
    float cy = dot(dirx, diry, dirz, ux, uy, uz);
    float cz = dot(dirx, diry, dirz, fx, fy, fz);

    // behind camera
    if (cz <= 1e-4f) return 0;

    // perspective projection
    float fov = DEG2RAD(fov_deg);
    float focal = (float)w / (2.0f * tanf(fov * 0.5f));

    float sx = (cx / cz) * focal + (float)w * 0.5f;
    float sy = (-cy / cz) * focal +(float)h * 0.5f;

    if (sx < 0 || sx >= w || sy < 0 || sy >= h)
    {
        return 0;
    }

    *outx = (int)sx;
    *outy = (int)sy;
    if (out_depth) *out_depth = cz;
    return 1;
}