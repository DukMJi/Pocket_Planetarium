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

#endif