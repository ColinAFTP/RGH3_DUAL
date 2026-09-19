# RGH3_DUAL Project Overview

This project implements a dual-Teensy 4.1 motion control system designed to manage a multi-axis "gripper" or "spreader" mechanism. The system distributes tasks between two controllers to ensure reliable high-speed motor control while maintaining a Modbus-based interface for external PLC coordination.

## Project Architecture

The system is split into two specialized controllers, **CPU1** and **CPU2**, which communicate over I2C and hardwired synchronization lines.

### 1. CPU 1: The Gateway & Logic Controller (`src/cpu1/`)
- **Modbus TCP Server:** Acts as the primary bridge to an external PLC over Ethernet.
- **Pattern Management:** Manages millimetric "gap patterns" defining the spacing between spreader arms.
- **System IO:** Reads digital inputs via shift registers (`FastShiftIn`) and controls 16 relays.
- **Coordination:** Pulses output `A1` to trigger CPU2 when a new move is required.

### 2. CPU 2: The Motion Controller (`src/cpu2/`)
- **Dedicated Motion Control:** Manages 9 stepper motors using the `TeensyStep4` library.
- **Kinematic Calculations:** Derives motor positions from gap patterns using a "midpoint-out" logic.
- **I2C Slave/Master:** Requests pattern data from CPU1 when triggered by the hardware sync line.

## Shared Components (`src/shared/`, `include/`)
- **Constants:** Shared Modbus addresses, pin definitions, and physical constants (e.g., `STEPS_PER_MM`).
- **Variables:** Global state shared across both build environments.
- **Functions:** Common logic for I2C communication, IO handling, and stepper configuration.

## Key Technologies
- **Framework:** Arduino (via PlatformIO)
- **Processor:** Teensy 4.1 (Dual)
- **Libraries:**
  - `TeensyStep4`: High-performance non-blocking stepper control.
  - `FastShiftIn`: Optimized shift register reading.
  - `ArduinoModbus` & `ArduinoRS485`: Industrial communication.
  - `teensy4_i2c-extended`: Enhanced I2C driver for Teensy 4.x.

## Building and Running

The project is managed with PlatformIO. There are two distinct environments defined in `platformio.ini`.

### Build Commands
- **Build CPU 1:** `pio run -e cpu1`
- **Build CPU 2:** `pio run -e cpu2`
- **Upload CPU 1:** `pio run -e cpu1 -t upload`
- **Upload CPU 2:** `pio run -e cpu2 -t upload`

### Monitoring
- **CPU 1 Serial (Modbus/Logic):** 115200 baud (Default COM9)
- **CPU 2 Serial (Motion/Debug):** 115200 baud (Default COM10)

## Development Conventions
- **Non-Blocking Architecture:** Avoid `delay()` and blocking loops. CPU2 must remain responsive to motion timers, while CPU1 must remain responsive to Modbus polling.
- **Hardware Sync:** Critical move triggers use hardwired IO (`OUTPUT_A1` on CPU1 to `INPUT_A1` on CPU2) rather than I2C to ensure minimal latency and jitter.
- **Shared Source:** Logic that applies to both controllers (like I2C packet structures or constant definitions) MUST reside in `src/shared` or `include`.

## Current Status & Roadmap
- [x] Dual-processor communication protocol (I2C Command 1, 2, 3).
- [x] Modbus TCP Server implementation on CPU1.
- [x] Kinematic target calculation on CPU2.
- [ ] **TODO:** Implement `g1.move()` or `g1.moveAsync()` on CPU2 to execute physical motion.
- [ ] **TODO:** Implement Homing sequence logic in `functions_steppers.cpp`.
- [ ] **TODO:** Wire up `MOVE_DONE` and `HOME_DONE` feedback signals from CPU2 back to CPU1.
