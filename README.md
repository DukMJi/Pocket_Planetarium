# Pocket Planetarium

Pocket Planetarium is a handheld raspberry Pi project that uses an IMU (Inertial Measurement Unit) to track device orientation and render a real-time view of the night sky using SDL2.

The long-term goal is a self-contained, porable "digital planetarium" that responds to how the device is pointed in the real world.

---

## Current Status
Cross-platform build system (macOS + Raspberry Pi)  
SDL2 window and rendering loop  
Text overlay with SDL_ttf  
IMU module with real I2C support on Raspberry Pi  
Fallback simulation mode for development without hardware  

The application currently displays yaw, pitch, roll, and FPS data,
using real IMU readings when available or a simulated fallback when
hardware is not connected.

---

## Architecture Overview

- **main.c**
  - SDL initialization and render loop
  - Input handling and timing
  - Displays orientation data and diagnostics
  - Automatically falls back to simulated motion if IMU is unavailable

- **imu.c / imu.h**
  - Hardware abstraction layer for the MPU6050 IMU
  - Linux (Raspberry Pi) implementation using `/dev/i2c-1`
  - macOS stub implementation for development and testing

- **CMake**
  - Cross-platform build configuration
  - Builds cleanly on macOS and Raspberry Pi

---

## Development Workflow

This project is designed to allow most development to be done
without hardware:

- On **macOS**: IMU functions return errors and the program
  automatically uses simulated orientation data.
- On **Raspberry Pi**: Real IMU data is read over I2C.

This allows UI, math, and rendering logic to be developed before
physical assembly.

---

## Planned Features

- IMU calibration and filtering
- Horizon line and orientation-based UI
- Star field rendering and sky projection
- Optional GPS and time-based sky alignment
- Physical enclosure and handheld assembly

---

## Build Instructions (High Level)

```bash
mkdir build
cd build
cmake ..
make -j4