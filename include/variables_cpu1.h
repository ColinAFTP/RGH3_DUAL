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
extern uint32_t lastModbusRequestMs;   // millis() when the last Modbus request from the PLC was answered
extern char plcRemoteText[24];                  // "a.b.c.d:port" of the connected Modbus client, for the event log
extern volatile uint8_t manualFlagsShared;      // MANUAL_FLAG_ bits for the manual command CPU2 reads over I2C (written by the main loop, read by the interrupt)
extern volatile uint8_t manualSpreaderShared;   // Spreader number for the manual command
extern uint32_t tickerTime;
extern uint16_t secondTicker;
extern bool bootLoadGaps;

// Status calculated by feedbackCheck() and shown on the web page
extern bool statusHome;                 // All home proximity sensors on and no fault
extern bool statusAtTarget;             // CPU2 says a move or homing has finished
extern bool statusFault;                // CPU2 has a homing fault
extern bool statusRefused;              // The last request was refused (by CPU1 or CPU2)
extern uint8_t statusRefusedReason;     // ...and why (EVT_REASON_ constant)
extern uint8_t faultTypeShown;          // The fault type shown to the PLC: FAULT_CPU2 if CPU2 is lost, otherwise the type CPU2 reported

// A pattern selection that CPU1 itself refused (out of range)
extern bool cpu1Refused;
extern uint8_t cpu1RefusedReason;

// At Target is forced off from the moment a new pattern is selected until CPU2 has dropped its own At Target line
extern bool atTargetBlank;
extern uint32_t atTargetBlankStart;

// Status packets received from CPU2 over I2C. The receive interrupt fills the ring, the main loop empties it.
constexpr int STATUS_RING_SIZE = 4;
extern StatusPacket statusRing[STATUS_RING_SIZE];
extern volatile uint8_t statusRingHead;
extern volatile uint8_t statusRingTail;

#endif
