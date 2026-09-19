#ifndef VARIABLES_CPU1_H
#define VARIABLES_CPU1_H

// Variables used only by CPU1 (Modbus/Ethernet, the input and relay shift registers).

#include <Arduino.h>
#include <NativeEthernet.h>
#include <ArduinoRS485.h>       // ArduinoModbus depends on the ArduinoRS485 library
#include <ArduinoModbus.h>
#include <FastShiftIn.h>

#include "variables.h"

// Variables used for the IO signals
extern FastShiftIn* FSI;      // This is a pointer, not an object
extern uint16_t inputData;        // The 16 inputs after the glitch filter (INPUT_FILTER_SAMPLES). Everything uses this
extern uint16_t inputDataRaw;     // The 16 inputs as sampled, before the filter (used only to log glitches)
extern uint16_t inputDataPrevious;

// Variables used for Ethernet communications
extern EthernetServer ethernetServer;
extern EthernetClient ethernetClient;
extern ModbusTCPServer modbusServer;
extern int patternSelection;
extern int patternSelectionPrevious;
extern word relayData;
extern word relayDataPrevious;
extern int speedData;
extern int speedDataPrevious;
extern uint32_t tickerTime;
extern uint16_t secondTicker;
extern bool bootLoadGaps;

// Status calculated by feedbackCheck() and shown on the web page
extern bool statusHome;                 // All home proximity sensors on and no fault
extern bool statusAtTarget;             // CPU2 says a move or homing has finished
extern bool statusFault;                // CPU2 has a homing fault

// Status packets received from CPU2 over I2C. The receive interrupt fills the ring, the main loop empties it.
constexpr int STATUS_RING_SIZE = 4;
extern StatusPacket statusRing[STATUS_RING_SIZE];
extern volatile uint8_t statusRingHead;
extern volatile uint8_t statusRingTail;

#endif
