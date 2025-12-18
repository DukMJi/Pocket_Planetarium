#ifndef IMU_H
#define IMU_H

#include <stdint.h>

typedef struct {
    float yaw;
    float pitch;
    float roll;
} imu_data_t;

// returns 0 on success, -1 on failure.
int imu_init(void);

// returns 0 on success, -1 on failure.
int imu_read(imu_data_t *data);
void imu_close(void);

// helper: fill angles with simulated values (always works)
void imu_sim_step(imu_data_t *data, float dt_seconds);

#endif