#include "imu.h"
#include <stdint.h>
#include <stdio.h>

#ifdef __linux__
    #include <unistd.h>
    #include <fcntl.h>
    #include <sys/ioctl.h>
    #include <linux/i2c-dev.h>
    #include <math.h>
#else
    // macOS / non-Linux: no I2C.  Provide stubs.
    #include <math.h>
#endif

#define MPU6050_ADDR         0x68
#define MPU6050_PWR_MGMT_1   0x6B
#define MPU6050_ACCEL_XOUT_H 0x3B

static int i2c_fd = -1;

int imu_init(void)
{
#ifdef __linux__
    const char *dev = "/dev/i2c-1";
    i2c_fd = open(dev, O_RDWR);
    if (i2c_fd < 0)
    {
        perror("open /dev/i2c-1");
        return -1;
    }

    if (ioctl(i2c_fd, I2C_SLAVE, MPU6050_ADDR) < 0)
    {
        perror("ioctl I2C_SLAVE");
        close(i2c_fd);
        i2c_fd = -1;
        return -1;
    }

    // Wake up MPU6050
    uint8_t buf[2] = {MPU6050_PWR_MGMT_1, 0};
    if (write(i2c_fd, buf, 2) != 2)
    {
        perror("write PWR_MGMT_1");
        close(i2c_fd);
        i2c_fd = -1;
        return -1;
    }

    return 0;
#else
    // macOS stub
    i2c_fd = -1;
    return -1;
#endif
}

int imu_read(imu_data_t *d)
{
    if (!d) return -1;

#ifdef __linux__

    if (i2c_fd < 0)
    {
        return -1;
    }

    uint8_t reg = MPU6050_ACCEL_XOUT_H;
    if (write(i2c_fd, &reg, 1) != 1)
    {
        return -1;
    }

    uint8_t data[14];
    if (read(i2c_fd, data, 14) != 14)
    {
        return -1;
    }

    int16_t ax = (data[0] << 8) | data[1];
    int16_t ay = (data[2] << 8) | data[3];
    int16_t az = (data[4] << 8) | data[5];
    int16_t gx = (data[8] << 8) | data[9];
    int16_t gy = (data[10] << 8) | data[11];
    int16_t gz = (data[12] << 8) | data[13];

    // Convert raw accel to g's (+/- 2g)
    float axg = ax / 16384.0f;
    float ayg = ay / 16384.0f;
    float azg = az / 16384.0f;

    // Convert gyro to deg/s (+/- 250 deg/s)
    float gxds = gx / 131.0f;
    float gyds = gy / 131.0f;
    float gzds = gz / 131.0f;

    d->pitch = atan2f(axg, sqrtf(ayg*ayg + azg*azg)) * 57.2958f;
    d->roll  = atan2f(ayg, sqrtf(axg*axg + azg*azg)) * 57.2958f;
    d->yaw   = gzds;    // placeholder

    return 0;
#else
    return -1;
#endif
}

void imu_close(void)
{
#ifdef __linux__
    if (i2c_fd >= 0)
    {
        close(i2c_fd);
        i2c_fd = -1;
    }
#endif
}