#ifndef STRUCTURES_H
#define STRUCTURES_H

#include "constants.h"

typedef struct {
    float patternData[NUM_PATTERNS][STRIDE_GAPS];
    int stepperSpeed;
} PatternPacket;

// An event that CPU2 reports to CPU1 for the web page event log (see the EVT_ constants)
typedef struct __attribute__((packed)) {
    uint8_t code;                       // EVT_ code
    int16_t arg;                        // Meaning depends on the code
} StatusEvent;

// Status that CPU2 sends to CPU1 about 4 times a second (I2C_CMD_STATUS). CPU1 shows it on the web page.
typedef struct __attribute__((packed)) {
    uint8_t state;                      // STATE_ constant
    uint8_t flags;                      // STATUS_FLAG_ bits: positions known, homing busy, manual mode active
    uint16_t faultMask;                 // Failed spreader bitmask, same as register ADDR_FAULT_SPREADERS
    uint8_t faultType;                  // FAULT_ constant, same as register ADDR_FAULT_TYPE
    int16_t positions[NUM_GAPS];        // Stepper positions in tenths of a millimetre from home
    uint8_t numEvents;                  // Number of valid entries in events[]
    StatusEvent events[STATUS_MAX_EVENTS];
    uint8_t refusedReason;              // Reason the last request was refused (EVT_REASON_ constant, 0 = none)
    uint32_t ioReads;                   // Number of times CPU2 read the inputs from CPU1 over I2C
    uint16_t ioFails;                   // ...and how many of those failed
    uint16_t otherFails;                // Failed I2C transfers of every other kind (status, gaps, pattern, fault message)
} StatusPacket;


// What CPU2 reads from CPU1 in manual mode (I2C_CMD_MANUAL)
typedef struct __attribute__((packed)) {
    uint8_t flags;                      // MANUAL_FLAG_ bits
    uint8_t spreader;                   // Holding register ADDR_MANUAL_PTR: the spreader to move (1 to 10)
    uint16_t inputs;                    // The 16 filtered inputs (the home sensors decide which spreaders are pushed along)
} ManualCommand;

#endif
