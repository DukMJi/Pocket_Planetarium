#ifndef IMU_H
#define IMU_H

#include <stdint.h>

typedef struct {
    float yaw;
    float pitch;
    float roll;
} imu_data_t;

int imu_init(void);
int imu_read(imu_data_t *data);
void imu_close(void);

#endif