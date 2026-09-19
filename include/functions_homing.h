#ifndef FUNCTIONS_HOMING_H
#define FUNCTIONS_HOMING_H

// Homing of the spreaders (CPU2 only). See the HOME_* constants in constants.h.

void homingStartup();                   // Call once at the end of setup(). Homes automatically if not all home sensors are on
bool homeRequest();                     // The PLC asked for home (pattern 0). Returns false if the request was refused
void homingService(bool moveFinished);  // Call every loop. moveFinished is the result of moveService()
bool homingActive();                    // True while a homing move (TeensyStep approach or direct pulse routine) is running
bool positionsKnown();                  // True once homing has set the stepper positions to 0 (and no fault has occurred since)
bool faultActive();                     // True while a homing fault is active. Pattern moves are refused
void faultReset();                      // Clear the homing fault and start a search home

#endif
