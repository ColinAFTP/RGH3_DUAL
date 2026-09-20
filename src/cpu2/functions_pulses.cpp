#include <Arduino.h>

#include "constants.h"
#include "functions_pulses.h"

namespace {

// Step and direction pins of the nine steppers, in stepper index order
const uint8_t stepPins[NUM_GAPS] = {
  STEPPER1_PULSE_PIN, STEPPER2_PULSE_PIN, STEPPER3_PULSE_PIN, STEPPER4_PULSE_PIN, STEPPER5_PULSE_PIN,
  STEPPER6_PULSE_PIN, STEPPER7_PULSE_PIN, STEPPER8_PULSE_PIN, STEPPER9_PULSE_PIN };
const uint8_t dirPins[NUM_GAPS] = {
  STEPPER1_DIR_PIN, STEPPER2_DIR_PIN, STEPPER3_DIR_PIN, STEPPER4_DIR_PIN, STEPPER5_DIR_PIN,
  STEPPER6_DIR_PIN, STEPPER7_DIR_PIN, STEPPER8_DIR_PIN, STEPPER9_DIR_PIN };

// TeensyStep drives the direction pin HIGH for the positive (open) direction and LOW for the negative (close) direction
constexpr int DIR_OPEN_LEVEL = HIGH;
constexpr int DIR_CLOSE_LEVEL = LOW;

IntervalTimer pulseTimer;
bool engineOn = false;

volatile bool runFlag[NUM_GAPS];        // Set by the main loop: this stepper should be running now
volatile uint32_t pulseCounts[NUM_GAPS];// Pulses emitted per stepper since the engine started
volatile uint32_t lastSensorReadUs = 0; // micros() of the last confirmation that the sensor data is fresh
volatile bool pulseStale = false;       // Set by the interrupt when it stopped the pulses for old data. The main loop reports it and clears it
volatile float targetRate = 1000;       // Steps/s
volatile float startRate = 400;         // Steps/s
volatile float rampPerTick = 0;         // Rate increase per tick while ramping up

float curRate[NUM_GAPS];                // Current pulse rate in steps/s (interrupt only)
float phaseAcc[NUM_GAPS];               // Step phase accumulator, a pulse is emitted each time it passes 1.0 (interrupt only)
bool pinHigh[NUM_GAPS];                 // Step pin is currently high (interrupt only)

// Timer interrupt: called every HOME_TICK_US
void pulseISR() {
  constexpr float tick = HOME_TICK_US * 1e-6f;

  // Never pulse on old data: if the main loop has not confirmed the sensors recently, everything stops (and restarts with the ramp)
  bool stale = (uint32_t)(micros() - lastSensorReadUs) > HOME_SENSOR_TIMEOUT_US;
  if (stale) pulseStale = true;

  for (int i = 0; i < NUM_GAPS; i++) {
    // End the pulse started on the previous tick
    if (pinHigh[i]) {
      digitalWrite(stepPins[i], LOW);
      pinHigh[i] = false;
    }
    if (runFlag[i] && !stale) {
      // Ramp up to the target rate
      if (curRate[i] < targetRate) {
        curRate[i] += rampPerTick;
        if (curRate[i] > targetRate) curRate[i] = targetRate;
      }
      phaseAcc[i] += curRate[i] * tick;
      if (phaseAcc[i] >= 1.0f) {
        phaseAcc[i] -= 1.0f;
        digitalWrite(stepPins[i], HIGH);
        pinHigh[i] = true;
        pulseCounts[i] = pulseCounts[i] + 1;
      }
    } else {
      // Stopped: the next start begins at the start rate, with the first pulse right away
      curRate[i] = startRate;
      phaseAcc[i] = 0.999f;
    }
  }
}

}  // namespace

void pulsesBegin(float rate, float start, uint32_t rampMs) {
  pulseTimer.end();
  targetRate = rate;
  startRate = start;
  rampPerTick = rate > start ? (rate - start) / ((float)rampMs * 1000.0f / HOME_TICK_US) : 0;
  for (int i = 0; i < NUM_GAPS; i++) {
    runFlag[i] = false;
    pulseCounts[i] = 0;
    curRate[i] = start;
    phaseAcc[i] = 0.999f;
    pinHigh[i] = false;
    digitalWrite(dirPins[i], DIR_CLOSE_LEVEL);
  }
  pulseStale = false;
  lastSensorReadUs = micros();
  delayMicroseconds(10);                // Direction setup time before the first pulse
  pulseTimer.begin(pulseISR, HOME_TICK_US);
  engineOn = true;
}

void pulsesEnd() {
  pulseTimer.end();
  engineOn = false;
  for (int i = 0; i < NUM_GAPS; i++) {
    runFlag[i] = false;
    digitalWrite(stepPins[i], LOW);
    pinHigh[i] = false;
  }
}

bool pulsesEngineOn() {
  return engineOn;
}

void pulsesRun(int i, bool run) {
  runFlag[i] = run;
}

bool pulsesIsRunning(int i) {
  return runFlag[i];
}

void pulsesStopAll() {
  for (int i = 0; i < NUM_GAPS; i++) runFlag[i] = false;
}

void pulsesSetDirection(int i, int dir) {
  digitalWrite(dirPins[i], dir > 0 ? DIR_OPEN_LEVEL : DIR_CLOSE_LEVEL);
  delayMicroseconds(10);                // Direction setup time before the next pulse
}

uint32_t pulsesCount(int i) {
  return pulseCounts[i];
}

void pulsesDataFresh() {
  lastSensorReadUs = micros();
}

bool pulsesTakeStale() {
  if (pulseStale) {
    pulseStale = false;
    return true;
  }
  return false;
}
