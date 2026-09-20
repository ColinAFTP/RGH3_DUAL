#ifndef FUNCTIONS_STATUS_H
#define FUNCTIONS_STATUS_H

#include <stdint.h>

// CPU2 reports its state and events to CPU1 over I2C so CPU1 can show them on its web page, and tells CPU1 when a request was refused.

void statusEvent(uint8_t code, int16_t arg = 0);    // Queue an event (EVT_ code) for the CPU1 event log. Main loop context only, never from an interrupt
void statusService();                               // Call every loop: sends the status packet when it is due (also while the direct pulse homing stage runs)
void requestStarted();                              // A new request (start move trigger) has arrived: clear the "refused" flag and line
void requestRefused(int reason);                    // The request was refused (EVT_REASON_ constant): sets the refused line, remembers the reason, queues an event
#endif
