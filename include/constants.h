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

// Pattern constants
constexpr int NUM_PATTERNS = 6;
constexpr int NUM_GAPS = 9;

// CPU1 pin numbers
// ================
// Relay shift register pins
constexpr int RELAY_DATA_CLOCK_PIN = 2;       //SHCP
constexpr int RELAY_DATA_PIN = 3;             //DATA
constexpr int RELAY_DATA_LATCH_PIN = 4;       //STCP

// Input shift register pins
constexpr int INPUTS_DATA_PIN = 5;            //Q7
constexpr int INPUTS_DATA_LOAD_PIN = 6;       //PL
constexpr int INPUTS_DATA_CLOCK_PIN = 7;      //CP

// Hardwired IO signal pins
constexpr int OUTPUT_A1 = 33;
constexpr int OUTPUT_A2 = 32;
constexpr int OUTPUT_A3 = 31;
constexpr int OUTPUT_A4 = 30;
constexpr int INPUT_B1 = 29;
constexpr int INPUT_B2 = 28;
constexpr int INPUT_B3 = 27;
constexpr int INPUT_B4 = 26;

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
constexpr int INPUT_A1 = 23;
constexpr int INPUT_A2 = 17;
constexpr int INPUT_A3 = 18;
constexpr int INPUT_A4 = 19;
constexpr int OUTPUT_B1 = 20;
constexpr int OUTPUT_B2 = 21;
constexpr int OUTPUT_B3 = 22;
constexpr int OUTPUT_B4 = 26;


#endif