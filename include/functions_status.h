#ifndef FUNCTIONS_STATUS_H
#define FUNCTIONS_STATUS_H

#include <stdint.h>

// CPU2 reports its state and events to CPU1 over I2C so CPU1 can show them on its web page.

void statusEvent(uint8_t code, int16_t arg = 0);    // Queue an event (EVT_ code) for the CPU1 event log. Main loop context only, never from an interrupt
void statusService();                               // Call every loop: sends the status packet when it is due

#endif
