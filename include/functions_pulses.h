#ifndef FUNCTIONS_PULSES_H
#define FUNCTIONS_PULSES_H

#include <stdint.h>

// The direct pulse engine (CPU2): one timer interrupt generates step pulses for the nine steppers at a constant rate, with a short ramp up
// when a stepper starts and an instant stop. It does not use TeensyStep. It is used by the homing routine and by manual mode.
// The engine only pulses while the main loop keeps confirming that its sensor data is fresh (pulsesDataFresh()): if the last confirmation is
// older than HOME_SENSOR_TIMEOUT_US, all pulses stop by themselves, so a stalled main loop can never leave a spreader running.

void pulsesBegin(float rate, float startRate, uint32_t rampMs);   // Start the engine. Every stepper is stopped, pointed in the close direction, and its pulse count is zero
void pulsesEnd();                       // Stop the engine and leave every step pin low
bool pulsesEngineOn();
void pulsesRun(int i, bool run);        // Ask stepper i to run (true) or stop (false). Stopping is instant
bool pulsesIsRunning(int i);            // True if stepper i has been asked to run
void pulsesStopAll();                   // Stop every stepper (the engine keeps running)
void pulsesSetDirection(int i, int dir);// +1 = open (direction pin HIGH), -1 = close (LOW). Only call while stepper i is stopped
uint32_t pulsesCount(int i);            // Pulses emitted by stepper i since pulsesBegin()
void pulsesDataFresh();                 // The main loop has just read good sensor data: keeps the pulses going
bool pulsesTakeStale();                 // True once if the interrupt stopped the pulses because the data was too old (clears itself)

#endif
