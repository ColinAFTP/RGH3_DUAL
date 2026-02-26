#ifndef CONSTANTS_H
#define CONSTANTS_H

// Modbus holding address constants
constexpr int ADDR_TICKER = 101;
constexpr int ADDR_HOME_COUNTS = 102;
constexpr int ADDR_INPUTS = 103;
constexpr int ADDR_PATTERN = 104;
constexpr int ADDR_SPEED = 105;
constexpr int ADDR_RELAYS = 106;              // Each of the first 16 bits corresponds to the switching of the 16 relays
constexpr int ADDR_MANUAL_PTR = 107;          // Pointer to which spreader will be moved in manual mode (1-9)

// Pattern 0 gaps
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
// Pattern 1 gaps
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
// Pattern 2 gaps
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
// Pattern 3 gaps
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
// Pattern 4 gaps
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

// Modbus status address constants
const int ADDR_HOME = 117;              // Gripper spreaders at their pickup positions 
const int ADDR_MOVE_DONE = 118;         // Gripper spreaders finsihed moving
const int ADDR_MANUAL_MODE = 119;       // Gripper is in manual mode

// General constants
const int DEBOUNCE_DELAY = 20;          // 20 ms debounce delay for inputs

// Stepper movement control constants
constexpr int INIT_SPEED = 5000;            // 15,000 achieves a move time of 1.444 s for 16,000 steps
constexpr int HOME_SPEED = 10000;    
constexpr int MAX_SPEED = 14000;
constexpr float INIT_ACCEL = 20000;         // 40,000 achieves a move time of 1.444 s for 16,000 steps
constexpr int MAX_STEPS = 16000;            // 16,000 is the maximum amount of steps for the rack length
constexpr float STEPS_PER_MM = 70.73553;    // Racks & pinions are mod 2, so tooth pitch is 2xPi mm. Pinions have 18 teeth. Distance per revolution is 113.0973 mm. There are 8000 steps per revolution, so distance per step is 0.014137 mm

// Pattern constants
constexpr int NUM_PATTERNS = 6;
constexpr int NUM_GAPS = 9;

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

// Hardwired IO signal pins
constexpr int OUTPUT_A1 = 33;                 // Initiate new stepper movement on rising edge
constexpr int OUTPUT_A2 = 32;                 // Manual mode enabled
constexpr int OUTPUT_A3 = 31;                 // Spare
constexpr int OUTPUT_A4 = 30;                 // Spare
constexpr int INPUT_B1 = 29;                  // Stepper motors are at their home positions
constexpr int INPUT_B2 = 28;                  // Stepper motors are at their target positions 
constexpr int INPUT_B3 = 27;                  // Spare
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

// Hardwired IO signal pins
constexpr int INPUT_A1 = 23;                  // Initiate new stepper movement on rising edge
constexpr int INPUT_A2 = 17;                  // Manual mode enabled
constexpr int INPUT_A3 = 18;                  // Spare
constexpr int INPUT_A4 = 19;                  // Spare
constexpr int OUTPUT_B1 = 20;                 // Stepper motors are at their home positions
constexpr int OUTPUT_B2 = 21;                 // Stepper motors are at their target positions 
constexpr int OUTPUT_B3 = 22;                 // Spare
constexpr int OUTPUT_B4 = 26;                 // Spare

#endif