#ifndef CONSTANTS_H
#define CONSTANTS_H

// Shared constants (declared here, defined in constants.cpp)
constexpr int NUM_PATTERNS = 6;
constexpr int NUM_GAPS = 9;

// CPU1 pin numbers
// Relay shift register pins
constexpr int RELAY_DATA_CLOCK_PIN = 2;       //SHCP
constexpr int RELAY_DATA_PIN = 3;             //DATA
constexpr int RELAY_DATA_LATCH_PIN = 4;       //STCP

// Input shift register pins
constexpr int INPUTS_DATA_PIN = 5;            //Q7
constexpr int INPUTS_DATA_LOAD_PIN = 6;       //PL
constexpr int INPUTS_DATA_CLOCK_PIN = 7;      //CP

// IP address DIP switch pins
constexpr int DIP_SW1 = 39;
constexpr int DIP_SW2 = 40;
constexpr int DIP_SW3 = 41;

// CPU2 pin numbers
// Stepper driver pins
const int STEPPER1_PULSE_PIN = 13;
const int STEPPER1_DIR_PIN = 33;
const int STEPPER2_PULSE_PIN = 12;
const int STEPPER2_DIR_PIN = 34;
const int STEPPER3_PULSE_PIN = 11;
const int STEPPER3_DIR_PIN = 35;
const int STEPPER4_PULSE_PIN = 10;
const int STEPPER4_DIR_PIN = 36;
const int STEPPER5_PULSE_PIN = 9;
const int STEPPER5_DIR_PIN = 37;
const int STEPPER6_PULSE_PIN = 8;
const int STEPPER6_DIR_PIN = 38;
const int STEPPER7_PULSE_PIN = 7;
const int STEPPER7_DIR_PIN = 32;
const int STEPPER8_PULSE_PIN = 6;
const int STEPPER8_DIR_PIN = 31;
const int STEPPER9_PULSE_PIN = 5;
const int STEPPER9_DIR_PIN = 30;
const int STEPPER10_PULSE_PIN = 4;
const int STEPPER10_DIR_PIN = 29;
const int STEPPER11_PULSE_PIN = 3;
const int STEPPER11_DIR_PIN = 28;
const int STEPPER12_PULSE_PIN = 2;
const int STEPPER12_DIR_PIN = 27;


#endif