#include "imu.h"
#include <stdint.h>
#include <stdio.h>

/*
 * Linux builds (Raspberry Pi) use real I2C access.
 * Non-Linux builds (macOS) compile stubbed versions
 * so development can continue without hardware.
 */
#ifdef __linux__
    #include <unistd.h>
    #include <fcntl.h>
    #include <sys/ioctl.h>
    #include <linux/i2c-dev.h>
    #include <math.h>
#endif

/*
 * MPU6050 register definitions.
 * These values come from the MPU6050 datasheet
 * */
#define MPU6050_ADDR         0x68
#define MPU6050_PWR_MGMT_1   0x6B
#define MPU6050_ACCEL_XOUT_H 0x3B

// File descriptor for /dev/i2c-1 (Linux only)
static int i2c_fd = -1;

/*
 * Initializes the MPU 6050 over I2C
 * - Opens /dev/i2c-1
 * - Sets the I2C slave address
 * - Wakes the MPU 6050 from sleep mode
 */
int imu_init(void)
{
#ifdef __linux__
    const char *dev = "/dev/i2c-1";

    // Open I2C bus
    i2c_fd = open(dev, O_RDWR);
    if (i2c_fd < 0)
    {
        perror("open /dev/i2c-1");
        return -1;
    }

    // Set MPU6050 as active I2C slave
    if (ioctl(i2c_fd, I2C_SLAVE, MPU6050_ADDR) < 0)
    {
        perror("ioctl I2C_SLAVE");
        close(i2c_fd);
        i2c_fd = -1;
        return -1;
    }

    // Wake up MPU6050 by clearing sleep bit
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
    // macOS stub: IMU not available.
    i2c_fd = -1;
    return -1;
#endif
}

/*
 * Reads raw accelerometer and gyroscope data from the MPU6050
 * and converts it into pitch, roll, and yaw values.
 * 
 * Pitch/Roll:
 *  Derived from accelerometer using trigonometry.
 * 
 * Yaw:
 *  Currently a placeholder derived from gyro Z-axis.
 */
int imu_read(imu_data_t *d)
{
    if (!d) return -1;

#ifdef __linux__

    if (i2c_fd < 0)
    {
        return -1;
    }

    // Select starting register
    uint8_t reg = MPU6050_ACCEL_XOUT_H;
    if (write(i2c_fd, &reg, 1) != 1)
    {
        return -1;
    }

    // Read 14 bytes: accel, temp, gyro
    uint8_t data[14];
    if (read(i2c_fd, data, 14) != 14)
    {
        return -1;
    }

    // Raw accelerometer values
    int16_t ax = (data[0] << 8) | data[1];
    int16_t ay = (data[2] << 8) | data[3];
    int16_t az = (data[4] << 8) | data[5];

    // Raw gyroscope values
    int16_t gx = (data[8] << 8) | data[9];
    int16_t gy = (data[10] << 8) | data[11];
    int16_t gz = (data[12] << 8) | data[13];

    // Convert raw accel to g's (+/- 2g)
    float axg = ax / 16384.0f;
    float ayg = ay / 16384.0f;
    float azg = az / 16384.0f;

    // Convert gyro to deg/s (+/- 250 deg/s)
    float gzds = gz / 131.0f;

    // Compute orientation angles
    d->pitch = atan2f(axg, sqrtf(ayg*ayg + azg*azg)) * 57.2958f;
    d->roll  = atan2f(ayg, sqrtf(axg*axg + azg*azg)) * 57.2958f;
    d->yaw   = gzds;    // placeholder

    return 0;
#else
    return -1;
#endif
}

// Closes the I2C device if open.
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

void imu_sim_step(imu_data_t *d, float dt)
{
    static float yaw = 0.f;
    static float pitch = 0.f;
    static float roll = 0.f;

    if (!d) return;

    yaw += 30.f * dt;
    pitch += 20.f * dt;
    roll += 15.f * dt;

    if (yaw > 360.f)    yaw -= 360.f;
    if (pitch > 360.f)  pitch -= 360.f;
    if (roll > 360.f)   roll -= 360.f;

    d->yaw = yaw;
    d->pitch = pitch;
    d->roll = roll;
}