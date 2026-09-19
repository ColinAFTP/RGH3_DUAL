#ifndef FUNCTIONS_WEB_H
#define FUNCTIONS_WEB_H

// Read only diagnostics web server on CPU1 (port 80), the CPU1 event log and the CPU2 status decoder.

void webSetup();                        // Start the web server. Call after modbusSetup() (Ethernet must be running)
void webService();                      // Call every loop. Non-blocking: serves at most one connection, in small steps
void webLoopTick();                     // Call once per loop pass, at the start: measures how long the loop takes
void cpu2StatusService();               // Call every loop: decodes the status packets and events received from CPU2
void logEvent(const char* fmt, ...);    // Add a line to the event log shown on the web page. Main loop context only, never from an interrupt

#endif
