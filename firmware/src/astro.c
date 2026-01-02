#include "astro.h"
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define DEG2RAD_F(x) ((float)(x) * ((float)M_PI / 180.0f))
#define RAD2DEG_F(x) ((float)(x) * (180.0f / (float)M_PI))

#define DEG2RAD_D(x) ((double)(x) * (M_PI / 180.0))
#define RAD2DEG_D(x) ((double)(x) * (180.0 / M_PI))

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
    float ra  = DEG2RAD_F(ra_hours * 15.0f);
    float dec = DEG2RAD_F(dec_deg);

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
    float yaw   = DEG2RAD_F(yaw_deg);
    float pitch = DEG2RAD_F(pitch_deg);
    float roll  = DEG2RAD_F(roll_deg);

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
    float fov = DEG2RAD_F(fov_deg);
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

static double clamp_hours(double h)
{
    while (h < 0.0)   h += 24.0;
    while (h >= 24.0) h -= 24.0;
    return h;
}

// Julian Date (UTC) from calendar date/time
double astro_julian_date_utc(int year, int month, int day, int hour, int min, double sec)
{
    // Algorithm valid for Gregorian calendar
    int a = (14 - month) / 12;
    int y = year + 4800 - a;
    int m = month + 12*a - 3;

    int jdn = day + (153*m + 2)/5 + 365*y + y/4 - y/100 + y/400 - 32045;

    double day_fraction = (hour - 12) / 24.0 + min / 1440.0 + sec / 86400.0;
    return (double)jdn + day_fraction;
}

// Greenwich Mean Sidereal Time (hours) from JD(UTC) (approx, good enough for now)
double astro_gmst_hours(double jd_utc)
{
    double d = jd_utc - 2451545.0; // days since J2000.0
    double gmst = 18.697374558 + 24.06570982441908 * d;
    return clamp_hours(gmst);
}

// Local Sidereal Time (hours)
double astro_lst_hours(double jd_utc, double lon_deg)
{
    double gmst = astro_gmst_hours(jd_utc);
    double lst = gmst + lon_deg / 15.0; // 15 deg per hour
    return clamp_hours(lst);
}

// RA/Dec -> Alt/Az at observer location/time
void astro_radec_to_altaz(float ra_hours, float dec_deg,
                          double jd_utc, double lat_deg, double lon_deg,
                          float *out_alt_deg, float *out_az_deg)
{
    // Hour Angle H = LST - RA
    double lst = astro_lst_hours(jd_utc, lon_deg);
    double H_hours = lst - (double)ra_hours;
    H_hours = clamp_hours(H_hours);

    // Convert to radians
    double H = DEG2RAD_D(H_hours * 15.0);
    double dec = DEG2RAD_D((double)dec_deg);
    double lat = DEG2RAD_D(lat_deg);

    // Altitude:
    // sin(alt) = sin(dec)*sin(lat) + cos(dec)*cos(lat)*cos(H)
    double sin_alt = sin(dec)*sin(lat) + cos(dec)*cos(lat)*cos(H);
    if (sin_alt >  1.0) sin_alt =  1.0;
    if (sin_alt < -1.0) sin_alt = -1.0;
    double alt = asin(sin_alt);

    // Azimuth:
    // az = atan2( -sin(H), tan(dec)*cos(lat) - sin(lat)*cos(H) )
    double y = -sin(H);
    double x = tan(dec)*cos(lat) - sin(lat)*cos(H);
    double az = atan2(y, x); // radians
    if (az < 0) az += 2.0*M_PI;

    if (out_alt_deg) *out_alt_deg = (float)RAD2DEG_D(alt);
    if (out_az_deg)  *out_az_deg  = (float)RAD2DEG_D(az);
}

// Alt/Az -> local direction unit vector
// Convention: x = East, y = North, z = Up (ENU)
void astro_altaz_to_unit(float alt_deg, float az_deg, float *x, float *y, float *z)
{
    double alt = DEG2RAD_D((double)alt_deg);
    double az  = DEG2RAD_D((double)az_deg);

    double ca = cos(alt);
    double sa = sin(alt);
    double saz = sin(az);
    double caz = cos(az);

    // ENU:
    // East  = cos(alt) * sin(az)
    // North = cos(alt) * cos(az)
    // Up    = sin(alt)
    if (x) *x = (float)(ca * saz);
    if (y) *y = (float)(ca * caz);
    if (z) *z = (float)(sa);
}