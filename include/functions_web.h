#ifndef FUNCTIONS_WEB_H
#define FUNCTIONS_WEB_H

#include <stdint.h>

// Read only diagnostics web server on CPU1 (port 80), the CPU1 event log and the CPU2 status decoder.

void webSetup();                        // Start the web server. Call after modbusSetup() (Ethernet must be running)
void webService();                      // Call every loop. Non-blocking: serves at most one connection, in small steps
void webLoopTick();                     // Call once per loop pass, at the start: measures how long the loop takes
void cpu2StatusService();               // Call every loop: decodes the status packets and events received from CPU2
void logEvent(const char* fmt, ...);    // Add a line to the event log shown on the web page. Main loop context only, never from an interrupt
void logInputChanges();                 // Call once per input sample: logs proxy input changes and counts glitches (changes that reverse within 5 ms)
uint32_t inputGlitches();               // Number of input glitches seen since start
bool cpu2Online();                      // CPU2 is sending its status (it is running)
bool cpu2Lost();                        // CPU2 stopped reporting, or never reported within the boot grace time: this is a fault
uint8_t cpu2RefusedReason();            // Why CPU2 refused the last request (EVT_REASON_ constant, 0 = not refused)
uint8_t cpu2Flags();                    // The STATUS_FLAG_ bits of CPU2's last status (positions known, homing, manual mode)

#endif
