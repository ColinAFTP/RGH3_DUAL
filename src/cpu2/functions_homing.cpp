#include <Arduino.h>

#include "constants.h"
#include "functions_homing.h"
#include "functions_i2c.h"
#include "functions_status.h"
#include "functions_steppers.h"
#include "variables.h"

// Homing of the spreaders.
//
// The direct pulse routine does not use TeensyStep. One timer interrupt generates the step pulses for all nine steppers at a
// constant rate (with a short ramp up when a stepper starts). The main loop reads the home proximity sensors from CPU1 over
// I2C, and lets each stepper run closed (towards home) while its sensor is off. The moment a sensor goes on, its pulses stop.
// If the sensor goes off again (the neighbour on the home side moved away) the stepper starts again, so the spreaders
// follow each other and every spreader homes as soon as it can.

namespace {

// Step and direction pins of the nine steppers, in stepper index order
const uint8_t stepPins[NUM_GAPS] = {
  STEPPER1_PULSE_PIN, STEPPER2_PULSE_PIN, STEPPER3_PULSE_PIN, STEPPER4_PULSE_PIN, STEPPER5_PULSE_PIN,
  STEPPER6_PULSE_PIN, STEPPER7_PULSE_PIN, STEPPER8_PULSE_PIN, STEPPER9_PULSE_PIN };
const uint8_t dirPins[NUM_GAPS] = {
  STEPPER1_DIR_PIN, STEPPER2_DIR_PIN, STEPPER3_DIR_PIN, STEPPER4_DIR_PIN, STEPPER5_DIR_PIN,
  STEPPER6_DIR_PIN, STEPPER7_DIR_PIN, STEPPER8_DIR_PIN, STEPPER9_DIR_PIN };

// TeensyStep drives the direction pin HIGH for the positive (open) direction and LOW for the negative (close) direction
constexpr int DIR_CLOSE = LOW;

enum HomeState { HOME_IDLE, HOME_APPROACH, HOME_CASCADE };
HomeState homeState = HOME_IDLE;

bool posKnown = false;                  // Stepper positions are trustworthy (homed, no fault since)
bool faultOn = false;
uint16_t faultMaskValue = 0;            // Bit (spreader number - 1) is set for each spreader that failed to home
bool maskPublished = true;              // False while the mask still has to be sent to CPU1
uint32_t lastPublishTry = 0;
int ioFailCount = 0;
uint32_t cascadeStartTime = 0;

// Direct pulse generation. The timer interrupt only reads runFlag and updates the rest.
IntervalTimer pulseTimer;
volatile bool runFlag[NUM_GAPS];        // Set by the main loop: this stepper should be running now
volatile uint32_t pulseCount[NUM_GAPS]; // Pulses emitted per stepper since the direct pulse routine started
float curRate[NUM_GAPS];                // Current pulse rate in steps/s (interrupt only)
float phaseAcc[NUM_GAPS];               // Step phase accumulator, a pulse is emitted each time it passes 1.0 (interrupt only)
bool pinHigh[NUM_GAPS];                 // Step pin is currently high (interrupt only)

// The bit of stepper index i in the failed spreader bitmask: bit (spreader number - 1)
uint16_t spreaderBit(int i) {
  return (uint16_t)1 << (i < NUM_LEFT_SPREADERS ? i : i + 1);
}

uint16_t allSpreadersMask() {
  uint16_t m = 0;
  for (int i = 0; i < NUM_GAPS; i++) m |= spreaderBit(i);
  return m;
}

// Timer interrupt: called every HOME_TICK_US
void pulseISR() {
  constexpr float tick = HOME_TICK_US * 1e-6f;
  constexpr float rampPerTick = (float)(HOME_PULSE_RATE - HOME_START_RATE) / ((float)HOME_RAMP_MS * 1000.0f / HOME_TICK_US);

  for (int i = 0; i < NUM_GAPS; i++) {
    // End the pulse started on the previous tick
    if (pinHigh[i]) {
      digitalWrite(stepPins[i], LOW);
      pinHigh[i] = false;
    }
    if (runFlag[i]) {
      // Ramp up to the homing rate
      if (curRate[i] < HOME_PULSE_RATE) {
        curRate[i] += rampPerTick;
        if (curRate[i] > HOME_PULSE_RATE) curRate[i] = HOME_PULSE_RATE;
      }
      phaseAcc[i] += curRate[i] * tick;
      if (phaseAcc[i] >= 1.0f) {
        phaseAcc[i] -= 1.0f;
        digitalWrite(stepPins[i], HIGH);
        pinHigh[i] = true;
        pulseCount[i] = pulseCount[i] + 1;
      }
    } else {
      // Stopped: the next start begins at the start rate, with the first pulse right away
      curRate[i] = HOME_START_RATE;
      phaseAcc[i] = 0.999f;
    }
  }
}

void stopAllRunning() {
  for (int i = 0; i < NUM_GAPS; i++) runFlag[i] = false;
}

// Stop the timer and leave every step pin low
void endPulses() {
  pulseTimer.end();
  stopAllRunning();
  for (int i = 0; i < NUM_GAPS; i++) {
    digitalWrite(stepPins[i], LOW);
    pinHigh[i] = false;
  }
}

void publishMask() {
  if (maskPublished) return;
  lastPublishTry = millis();
  if (writeFaultMask(faultMaskValue)) {
    maskPublished = true;
  }
}

void raiseFault(uint16_t mask) {
  endPulses();
  homeState = HOME_IDLE;
  faultOn = true;
  posKnown = false;
  faultMaskValue = mask;
  maskPublished = false;
  digitalWrite(OUTPUT_B2, LOW);
  digitalWrite(OUTPUT_B3, HIGH);
  Serial.print("HOMING FAULT. Failed spreader bitmask: 0x");
  Serial.println(mask, HEX);
  statusEvent(EVT_HOME_FAULT, mask);
  publishMask();
}

// Start the direct pulse routine: every stepper is pointed in the close direction, the timer starts, and
// the main loop decides which steppers actually run from the sensors.
void beginCascade() {
  for (int i = 0; i < NUM_GAPS; i++) {
    runFlag[i] = false;
    pulseCount[i] = 0;
    curRate[i] = HOME_START_RATE;
    phaseAcc[i] = 0.999f;
    pinHigh[i] = false;
    digitalWrite(dirPins[i], DIR_CLOSE);
  }
  delayMicroseconds(10);                // Direction setup time before the first pulse
  ioFailCount = 0;
  cascadeStartTime = millis();
  digitalWrite(OUTPUT_B2, LOW);
  homeState = HOME_CASCADE;
  pulseTimer.begin(pulseISR, HOME_TICK_US);
  Serial.println("Homing: direct pulse routine started.");
  statusEvent(EVT_HOME_PULSES);
}

void completeHoming() {
  endPulses();
  setAllStepperPositions(0);
  posKnown = true;
  homeState = HOME_IDLE;
  digitalWrite(OUTPUT_B2, HIGH);
  Serial.println("Homing complete: all spreaders home.");
  statusEvent(EVT_HOME_DONE);
}

// One pass of the direct pulse routine, called every loop
void cascadeStep() {
  int io = readIO();
  if (io < 0) {
    // Never move blind: stop until the sensors can be read again
    stopAllRunning();
    if (++ioFailCount == 1) statusEvent(EVT_IO_FAIL);
    if (ioFailCount >= HOME_IO_FAIL_LIMIT) {
      raiseFault(allSpreadersMask());
    }
    return;
  }
  ioFailCount = 0;

  bool allOn = true;
  uint16_t failed = 0;
  uint16_t notOn = 0;
  for (int i = 0; i < NUM_GAPS; i++) {
    bool on = (io >> (PROXY_FIRST_BIT + i)) & 1;
    runFlag[i] = !on;
    if (!on) {
      allOn = false;
      notOn |= spreaderBit(i);
      if (pulseCount[i] > (uint32_t)HOME_MAX_STEPS) failed |= spreaderBit(i);
    }
  }

  if (failed) {
    raiseFault(failed);
  } else if (millis() - cascadeStartTime > HOME_TIMEOUT_MS) {
    raiseFault(notOn);
  } else if (allOn) {
    completeHoming();
  }
}

}  // namespace

void homingStartup() {
  // CPU1 answers I2C only once it has finished starting up and has read its inputs. Give it time.
  int io = -1;
  uint32_t t0 = millis();
  while (millis() - t0 < 20000) {
    io = readIO();
    if (io >= 0) break;
    delay(100);
  }

  if (io < 0) {
    Serial.println("Homing: CPU1 did not answer over I2C at power up.");
    raiseFault(allSpreadersMask());
    return;
  }

  if ((io & PROXY_ALL_MASK) == PROXY_ALL_MASK) {
    setAllStepperPositions(0);
    posKnown = true;
    digitalWrite(OUTPUT_B2, HIGH);
    Serial.println("Power up: all home sensors on, gripper is home.");
    statusEvent(EVT_POWERUP_HOME);
  } else {
    Serial.println("Power up: not all home sensors on, starting search home.");
    statusEvent(EVT_POWERUP_SEARCH);
    beginCascade();
  }
}

bool homeRequest() {
  if (faultOn || homeState != HOME_IDLE) return false;

  if (!posKnown) {
    // Positions cannot be trusted: search home with the direct pulse routine only
    statusEvent(EVT_HOME_START, 2);
    beginCascade();
    return true;
  }

  updateStepperPositions();
  updateStepperSpeeds(stepperSpeed);

  if (HOME_APPROACH_STEPS <= 0) {
    // TeensyStep all the way to position 0, no direct pulse stage
    statusEvent(EVT_HOME_START, 0);
    if (!stepTargetCalc(0)) return false;
    return triggerMove();
  }

  // TeensyStep to HOME_APPROACH_MM from home (steppers already closer stay where they are), then the direct pulse routine
  statusEvent(EVT_HOME_START, 1);
  bool anyMove = false;
  for (int i = 0; i < NUM_GAPS; i++) {
    long target = stepperPositions[i] < HOME_APPROACH_STEPS ? stepperPositions[i] : HOME_APPROACH_STEPS;
    if (target != stepperPositions[i]) anyMove = true;
    stepperTargets[i] = target;
  }
  if (!anyMove) {
    beginCascade();
    return true;
  }
  if (!triggerMove()) return false;
  homeState = HOME_APPROACH;
  return true;
}

void homingService(bool moveFinished) {
  if (!maskPublished && millis() - lastPublishTry >= 100) {
    publishMask();
  }

  switch (homeState) {
    case HOME_APPROACH:
      if (moveFinished) beginCascade();
      break;
    case HOME_CASCADE:
      cascadeStep();
      break;
    default:
      break;
  }
}

bool homingActive() {
  return homeState != HOME_IDLE;
}

bool positionsKnown() {
  return posKnown;
}

bool faultActive() {
  return faultOn;
}

void faultReset() {
  if (!faultOn) return;
  Serial.println("Fault reset: starting search home.");
  statusEvent(EVT_FAULT_RESET);
  faultOn = false;
  digitalWrite(OUTPUT_B3, LOW);
  faultMaskValue = 0;
  maskPublished = false;
  publishMask();
  beginCascade();
}

int homingStage() {
  switch (homeState) {
    case HOME_APPROACH: return 1;
    case HOME_CASCADE: return 2;
    default: return 0;
  }
}

uint16_t faultMask() {
  return faultMaskValue;
}
