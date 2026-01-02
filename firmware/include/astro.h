#ifndef ASTRO_H
#define ASTRO_H

// Convert RA/Dec to a unit direction vector in world space.
// ra_hours: 0..24, dec_deg: -90..+90
void astro_radec_to_unit(float ra_hours, float dec_deg, float *x, float *y, float *z);

// Build camera basis vectors from yaw/pitch/roll degrees.
// Produces right/up/forward vectors (each is length 1).
void astro_camera_basis(float yaw_deg, float pitch_deg, float roll_deg,
                        float *rx, float *ry, float *rz,
                        float *ux, float *uy, float *uz,
                        float *fx, float *fy, float *fz);

// Project a world direction vector onto screen.
// Returns 1 if visible, 0 if behind, outside view.
// fov_deg is horizontal FOV in degrees.
int astro_project_dir(float dirx, float diry, float dirz,
                        float rx, float ry, float rz,
                        float ux, float uy, float uz, 
                        float fx, float fy, float fz,
                        int w, int h, float fov_deg, 
                        int *outx, int *outy, float *out_depth);

// NEW: time helpers + local sky conversion
double astro_julian_date_utc(int year, int month, int day, int hour, int min, double sec);
double astro_gmst_hours(double jd_utc);
double astro_lst_hours(double jd_utc, double lon_deg);

// NEW: RA/Dec -> Alt/Az (degrees)
void astro_radec_to_altaz(float ra_hours, float dec_deg,
                          double jd_utc, double lat_deg, double lon_deg,
                          float *out_alt_deg, float *out_az_deg);

// NEW: Alt/Az -> local unit vector (ENU-ish)
void astro_altaz_to_unit(float alt_deg, float az_deg, float *x, float *y, float *z);

#endif