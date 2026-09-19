#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <stdint.h>

// Modbus holding address constants
constexpr int ADDR_TICKER = 101;
constexpr int ADDR_HOME_COUNTS = 102;
constexpr int ADDR_INPUTS = 103;
constexpr int ADDR_PATTERN = 104;             // PLC selects the gripper position: 0 = go HOME, 1 to NUM_PATTERNS = gap pattern n (register block n-1 below)
constexpr int ADDR_SPEED = 105;
constexpr int ADDR_RELAYS = 106;              // Each of the first 16 bits corresponds to the switching of the 16 relays
constexpr int ADDR_MANUAL_PTR = 107;          // Pointer to which spreader will be moved in manual mode (1-9)
constexpr int ADDR_FAULT_SPREADERS = 108;     // Bitmask of the spreaders that failed to home. Bit (n-1) is spreader n (spreader 5 is static so bit 4 is never set). Read only from the PLC. Cleared by a fault reset.

// Pattern 1 gaps
constexpr int ADDR_PATTERN_0_0 = 111;
constexpr int ADDR_PATTERN_0_1 = 112;
constexpr int ADDR_PATTERN_0_2 = 113;
constexpr int ADDR_PATTERN_0_3 = 114;
constexpr int ADDR_PATTERN_0_4 = 115;
constexpr int ADDR_PATTERN_0_5 = 116;
constexpr int ADDR_PATTERN_0_6 = 117;
constexpr int ADDR_PATTERN_0_7 = 118;
constexpr int ADDR_PATTERN_0_8 = 119;
constexpr int ADDR_PATTERN_0_9 = 120;
// Pattern 2 gaps
constexpr int ADDR_PATTERN_1_0 = 121;
constexpr int ADDR_PATTERN_1_1 = 122;
constexpr int ADDR_PATTERN_1_2 = 123;
constexpr int ADDR_PATTERN_1_3 = 124;
constexpr int ADDR_PATTERN_1_4 = 125;
constexpr int ADDR_PATTERN_1_5 = 126;
constexpr int ADDR_PATTERN_1_6 = 127;
constexpr int ADDR_PATTERN_1_7 = 128;
constexpr int ADDR_PATTERN_1_8 = 129;
constexpr int ADDR_PATTERN_1_9 = 130;
// Pattern 3 gaps
constexpr int ADDR_PATTERN_2_0 = 131;
constexpr int ADDR_PATTERN_2_1 = 132;
constexpr int ADDR_PATTERN_2_2 = 133;
constexpr int ADDR_PATTERN_2_3 = 134;
constexpr int ADDR_PATTERN_2_4 = 135;
constexpr int ADDR_PATTERN_2_5 = 136;
constexpr int ADDR_PATTERN_2_6 = 137;
constexpr int ADDR_PATTERN_2_7 = 138;
constexpr int ADDR_PATTERN_2_8 = 139;
constexpr int ADDR_PATTERN_2_9 = 140;
// Pattern 4 gaps
constexpr int ADDR_PATTERN_3_0 = 141;
constexpr int ADDR_PATTERN_3_1 = 142;
constexpr int ADDR_PATTERN_3_2 = 143;
constexpr int ADDR_PATTERN_3_3 = 144;
constexpr int ADDR_PATTERN_3_4 = 145;
constexpr int ADDR_PATTERN_3_5 = 146;
constexpr int ADDR_PATTERN_3_6 = 147;
constexpr int ADDR_PATTERN_3_7 = 148;
constexpr int ADDR_PATTERN_3_8 = 149;
constexpr int ADDR_PATTERN_3_9 = 150;
// Pattern 5 gaps
constexpr int ADDR_PATTERN_4_0 = 151;
constexpr int ADDR_PATTERN_4_1 = 152;
constexpr int ADDR_PATTERN_4_2 = 153;
constexpr int ADDR_PATTERN_4_3 = 154;
constexpr int ADDR_PATTERN_4_4 = 155;
constexpr int ADDR_PATTERN_4_5 = 156;
constexpr int ADDR_PATTERN_4_6 = 157;
constexpr int ADDR_PATTERN_4_7 = 158;
constexpr int ADDR_PATTERN_4_8 = 159;
constexpr int ADDR_PATTERN_4_9 = 160;

// Modbus coil address constant
const int ADDR_HOMING = 101;            // Homing procedure requested by PLC
const int ADDR_GAP_UPDATE = 102;        // Update all the gap patterns
const int ADDR_RELAY_TEST = 103;        // Run the relay test routine
const int ADDR_MANUAL = 104;            // Manual mode request from PLC to move spreaders
const int ADDR_MANUAL_OPN = 105;        // Manual move spreader in open direction
const int ADDR_MANUAL_CLS = 106;        // Manual move spreader in close direction
const int ADDR_FAULT_RESET = 107;       // PLC sets this to reset a fault. CPU1 clears it again and pulses OUTPUT_A3 so CPU2 clears its fault and starts a search home

// Modbus status address constants (discrete inputs, read only from the PLC)
const int ADDR_HOME = 117;              // Gripper is home: all nine home proximity sensors (proxy 2 to 10) are on and there is no fault. Calculated live by CPU1
const int ADDR_MOVE_DONE = 118;         // At target: the requested pattern move or homing has finished. Off while a move or homing is running
const int ADDR_MANUAL_MODE = 119;       // Gripper is in manual mode
const int ADDR_HOMING_FAULT = 120;      // Homing fault is active. Register ADDR_FAULT_SPREADERS says which spreaders failed. Cleared with ADDR_FAULT_RESET

// General constants
constexpr bool DEBUG_STEPPER_CALC = false;  // Print stepper target calculations
constexpr bool DEBUG_PLOT = false;       // Print stepper positions in Serial Plotter format (CPU2)
constexpr bool DEBUG_GAP_UPDATE = false;    // Print every gap value read from the PLC (CPU1)
const int DEBOUNCE_DELAY = 20;          // 20 ms debounce delay for inputs

// Stepper movement control constants
constexpr int INIT_SPEED = 5000;            // 15,000 achieves a move time of 1.444 s for 16,000 steps
constexpr int MAX_SPEED = 14000;
constexpr float INIT_ACCEL = 20000;         // 40,000 achieves a move time of 1.444 s for 16,000 steps
constexpr int MAX_STEPS = 16000;            // 16,000 is the maximum amount of steps for the rack length
constexpr float STEPS_PER_MM = 70.73553;    // Racks & pinions are mod 2, so tooth pitch is 2xPi mm. Pinions have 18 teeth. Distance per revolution is 113.0973 mm. There are 8000 steps per revolution, so distance per step is 0.014137 mm

// Pattern constants
constexpr int NUM_PATTERNS = 5;             // Number of gap patterns. The PLC selects them with register 104 = 1 to NUM_PATTERNS (0 = home, which needs no gap data)
constexpr int NUM_GAPS = 9;                 // Number of gaps actually used in the current application. There are 10 spreaders, so 9 gaps and 9 steppers
constexpr int STRIDE_GAPS = 10;             // Fixed Modbus register spacing per pattern

// Spreader layout. There are 10 spreaders numbered 1 to 10 from left to right (looking from the control box side).
// Spreader 5 is static. Stepper index 0 to 8 drives spreaders 1,2,3,4,6,7,8,9,10 in that order.
// Gap n (index 0 to 8) is the gap between spreader n+1 and n+2, so gap 3 is spreader 4 to the static spreader 5 and gap 4 is the static spreader 5 to spreader 6.
constexpr int NUM_LEFT_SPREADERS = 4;       // Spreaders 1 to 4 are left of the static spreader (stepper index 0 to 3, gaps 0 to 3)
constexpr int NUM_RIGHT_SPREADERS = 5;      // Spreaders 6 to 10 are right of the static spreader (stepper index 4 to 8, gaps 4 to 8)
static_assert(NUM_LEFT_SPREADERS + NUM_RIGHT_SPREADERS == NUM_GAPS, "Spreader layout must add up to NUM_GAPS");

// Home proximity sensors. They are wired to the CPU1 input shift registers (input bit = proxy number - 1).
// Proxy 1 (bit 0) and proxy 11 (bit 10) are the left and right over travel sensors and are not used at the moment.
// Proxy 2 to 5 are the home sensors of spreaders 1 to 4 and proxy 6 to 10 those of spreaders 6 to 10, so the sensor of stepper index i is on input bit PROXY_FIRST_BIT + i.
// A sensor is on when its spreader is closed up against its neighbour on the side of the static spreader.
constexpr int PROXY_FIRST_BIT = 1;
constexpr uint16_t PROXY_ALL_MASK = 0x03FE; // Input bits 1 to 9: all nine home sensors

// Homing constants (CPU2). These are meant to be tuned on the real gripper.
// A normal home moves every stepper with TeensyStep to HOME_APPROACH_MM from home, then the direct pulse routine closes
// the remaining distance and stops each stepper the moment its proximity sensor goes on. Set HOME_APPROACH_MM to 0 to let
// TeensyStep move all the way to position 0 (no direct pulse stage) if the hybrid homing turns out to be too slow.
// After a power up without all sensors on, or after a fault, the direct pulse routine runs on its own (search home).
constexpr float HOME_APPROACH_MM = 5.0;                 // Distance from home at which TeensyStep hands over to the direct pulse routine
constexpr long HOME_APPROACH_STEPS = (long)(HOME_APPROACH_MM * STEPS_PER_MM + 0.5f);
constexpr int HOME_PULSE_RATE = 1500;                   // Direct pulse homing speed in steps/s
constexpr int HOME_START_RATE = 400;                    // Direct pulse speed when a stepper starts moving, in steps/s
constexpr int HOME_RAMP_MS = 80;                        // Time to ramp from HOME_START_RATE to HOME_PULSE_RATE. Stopping is always instant
constexpr int HOME_TICK_US = 20;                        // Direct pulse timer period. A step pulse is one tick wide, so keep HOME_PULSE_RATE well below 1 / (2 x tick)
constexpr int HOME_MAX_STEPS = MAX_STEPS + 500;         // A stepper that emits more than this many pulses without its sensor going on has failed to home
constexpr uint32_t HOME_TIMEOUT_MS = 30000;             // Overall time limit for the direct pulse routine
constexpr int HOME_IO_FAIL_LIMIT = 50;                  // Consecutive failed I2C reads of the sensors before homing is aborted with a fault. Pulses stop at the first failed read

// I2C commands from CPU2 (master) to CPU1 (slave, address 0x40)
constexpr uint8_t I2C_CMD_IO = 1;                       // Request: 16 bit input word (uint16_t)
constexpr uint8_t I2C_CMD_GAPS = 2;                     // Request: all gap patterns and the stepper speed (PatternPacket)
constexpr uint8_t I2C_CMD_PATTERN = 3;                  // Request: pattern selection (int)
constexpr uint8_t I2C_CMD_FAULT_MASK = 4;               // Write: followed by the 16 bit failed spreader bitmask, low byte first. CPU1 copies it to ADDR_FAULT_SPREADERS

// CPU1 pin numbers
// ================
// Relay shift register pins
constexpr int RELAY_DATA_CLOCK_PIN = 2;       // SHCP
constexpr int RELAY_DATA_PIN = 3;             // DATA
constexpr int RELAY_DATA_LATCH_PIN = 4;       // STCP

// Input shift register pins
constexpr int INPUTS_DATA_PIN = 5;            // Q7
constexpr int INPUTS_DATA_LOAD_PIN = 6;       // PL
constexpr int INPUTS_DATA_CLOCK_PIN = 7;      // CP

// Hardwired IO signal pins between CPU1 and CPU2. A pins are CPU1 outputs (wired to the CPU2 INPUT_A pins), B pins are CPU2 outputs (wired to the CPU1 INPUT_B pins).
constexpr int OUTPUT_A1 = 33;                 // Start move: CPU1 holds this high for 500 ms when the PLC changes the pattern (register 104). CPU2 acts on the rising edge and reads the pattern over I2C. Pattern 0 means go home
constexpr int OUTPUT_A2 = 32;                 // Manual mode enabled (not implemented yet)
constexpr int OUTPUT_A3 = 31;                 // Fault reset: CPU1 pulses this high for 100 ms when the PLC sets the fault reset coil. CPU2 clears its homing fault on the rising edge and starts a search home
constexpr int OUTPUT_A4 = 30;                 // Spare
constexpr int INPUT_B1 = 29;                  // Reserved, not used. The Home signal is calculated by CPU1 from the home proximity sensors
constexpr int INPUT_B2 = 28;                  // At target: high when CPU2 has finished a move or homing, low while it is moving or homing
constexpr int INPUT_B3 = 27;                  // Homing fault: high while CPU2 has a homing fault. The failed spreaders are sent over I2C (I2C_CMD_FAULT_MASK)
constexpr int INPUT_B4 = 26;                  // Spare

// IP address DIP switch pins
constexpr int DIP_SW1 = 39;
constexpr int DIP_SW2 = 40;
constexpr int DIP_SW3 = 41;

// CPU2 pin numbers
// ================
// Stepper driver pins
constexpr int STEPPER1_PULSE_PIN = 13;
constexpr int STEPPER1_DIR_PIN = 33;
constexpr int STEPPER2_PULSE_PIN = 12;
constexpr int STEPPER2_DIR_PIN = 34;
constexpr int STEPPER3_PULSE_PIN = 11;
constexpr int STEPPER3_DIR_PIN = 35;
constexpr int STEPPER4_PULSE_PIN = 10;
constexpr int STEPPER4_DIR_PIN = 36;
constexpr int STEPPER5_PULSE_PIN = 9;
constexpr int STEPPER5_DIR_PIN = 37;
constexpr int STEPPER6_PULSE_PIN = 8;
constexpr int STEPPER6_DIR_PIN = 38;
constexpr int STEPPER7_PULSE_PIN = 7;
constexpr int STEPPER7_DIR_PIN = 32;
constexpr int STEPPER8_PULSE_PIN = 6;
constexpr int STEPPER8_DIR_PIN = 31;
constexpr int STEPPER9_PULSE_PIN = 5;
constexpr int STEPPER9_DIR_PIN = 30;
constexpr int STEPPER10_PULSE_PIN = 4;
constexpr int STEPPER10_DIR_PIN = 29;
constexpr int STEPPER11_PULSE_PIN = 3;
constexpr int STEPPER11_DIR_PIN = 28;
constexpr int STEPPER12_PULSE_PIN = 2;
constexpr int STEPPER12_DIR_PIN = 27;

// Hardwired IO signal pins between CPU1 and CPU2 (see the CPU1 pin descriptions above). These are pulled down so a disconnected wire cannot cause false triggers.
constexpr int INPUT_A1 = 23;                  // Start move: CPU2 acts on the rising edge, reads the pattern over I2C. Pattern 0 means go home
constexpr int INPUT_A2 = 17;                  // Manual mode enabled (not implemented yet)
constexpr int INPUT_A3 = 18;                  // Fault reset: rising edge clears the homing fault and starts a search home
constexpr int INPUT_A4 = 19;                  // Spare
constexpr int OUTPUT_B1 = 20;                 // Reserved, not used. The Home signal is calculated by CPU1 from the home proximity sensors
constexpr int OUTPUT_B2 = 21;                 // At target: set high when a move or homing has finished, cleared when a move or homing starts
constexpr int OUTPUT_B3 = 22;                 // Homing fault: set high while a homing fault is active (until reset with INPUT_A3)
constexpr int OUTPUT_B4 = 26;                 // Spare

#endif