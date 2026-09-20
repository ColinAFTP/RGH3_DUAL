#ifndef FUNCTIONS_MANUAL_H
#define FUNCTIONS_MANUAL_H

// Manual mode (CPU2). See the description above DIP_MANUAL_PIN in constants.h.

void manualService();                   // Call every loop, early: handles entering and leaving manual mode and the jog commands
bool manualActive();                    // Manual mode is on: nothing else moves and the PLC jogs one spreader at a time
bool manualRequested();                 // The DIP switch or the PLC asked for manual mode (it may still be waiting for a move to stop)
long manualPositionSteps(int i);        // Position of stepper i in steps from home while manual mode is active (counted from the pulses)

#endif
