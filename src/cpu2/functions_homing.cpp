#include <Arduino.h>

#include "constants.h"
#include "functions_homing.h"
#include "functions_i2c.h"
#include "functions_manual.h"
#include "functions_pulses.h"
#include "functions_status.h"
#include "functions_steppers.h"
#include "spreaders.h"
#include "variables.h"

// Homing of the spreaders.
//
// The direct pulse routine does not use TeensyStep. The pulse engine (functions_pulses.cpp) generates the step pulses for all nine
// steppers. The main loop reads the home proximity sensors from CPU1 over I2C, and lets each stepper run closed (towards home) while its
// sensor is off. The moment a sensor goes on, its pulses stop. If the sensor goes off again (the neighbour on the home side moved away)
// the stepper starts again, so the spreaders follow each other and every spreader homes as soon as it can.

namespace {

enum HomeState { HOME_IDLE, HOME_APPROACH, HOME_CASCADE };
HomeState homeState = HOME_IDLE;

bool posKnown = false;                  // Stepper positions are trustworthy (homed, no fault since)
bool faultOn = false;
uint16_t faultMaskValue = 0;            // Bit (spreader number - 1) is set for each spreader that failed to home
uint8_t faultTypeValue = FAULT_NONE;    // FAULT_HOMING or FAULT_OVERTRAVEL while a fault is active
bool maskPublished = true;              // False while the mask still has to be sent to CPU1
uint32_t lastPublishTry = 0;
int ioFailCount = 0;
uint32_t cascadeStartTime = 0;
bool tsHomeMove = false;                // A TeensyStep only home move is running (HOME_APPROACH_MM = 0)
long cascadeStartPos[NUM_GAPS];         // Stepper positions when the direct pulse stage started
bool cascadeStartKnown = false;         // ...and whether they were trustworthy

uint16_t allSpreadersMask() {
  uint16_t m = 0;
  for (int i = 0; i < NUM_GAPS; i++) m |= spreaderBit(i);
  return m;
}

void publishMask() {
  if (maskPublished) return;
  lastPublishTry = millis();
  if (writeFaultMask(faultMaskValue, faultTypeValue)) {
    maskPublished = true;
  }
}

void raiseFault(uint16_t mask, uint8_t type = FAULT_HOMING) {
  pulsesEnd();
  homeState = HOME_IDLE;
  tsHomeMove = false;
  faultOn = true;
  posKnown = false;
  faultMaskValue = mask;
  faultTypeValue = type;
  maskPublished = false;
  digitalWrite(OUTPUT_B2, LOW);
  digitalWrite(OUTPUT_B3, HIGH);
  Serial.print(type == FAULT_OVERTRAVEL ? "OVER TRAVEL FAULT. Spreader bitmask: 0x" : "HOMING FAULT. Failed spreader bitmask: 0x");
  Serial.println(mask, HEX);
  statusEvent(type == FAULT_OVERTRAVEL ? EVT_OVERTRAVEL : EVT_HOME_FAULT, mask);
  publishMask();
}

// Start the direct pulse routine: every stepper is pointed in the close direction, the engine starts, and
// the main loop decides which steppers actually run from the sensors.
void beginCascade() {
  updateStepperPositions();
  for (int i = 0; i < NUM_GAPS; i++) cascadeStartPos[i] = stepperPositions[i];
  cascadeStartKnown = posKnown;
  ioFailCount = 0;
  cascadeStartTime = millis();
  digitalWrite(OUTPUT_B2, LOW);
  homeState = HOME_CASCADE;
  pulsesBegin(HOME_PULSE_RATE, HOME_START_RATE, HOME_RAMP_MS);
  Serial.println("Homing: direct pulse routine started.");
  statusEvent(EVT_HOME_PULSES);
}

void completeHoming() {
  pulsesEnd();
  setAllStepperPositions(0);
  posKnown = true;
  homeState = HOME_IDLE;
  digitalWrite(OUTPUT_B2, HIGH);
  Serial.println("Homing complete: all spreaders home.");
  statusEvent(EVT_HOME_DONE);
}

// One pass of the direct pulse routine, called every loop
void cascadeStep() {
  if (DEBUG_STALL_TEST) {
    static uint32_t lastStall = 0;
    if (millis() - lastStall > 1000) {
      lastStall = millis();
      delay(40);                        // TEST ONLY: longer than HOME_SENSOR_TIMEOUT_US, so the engine must stop the pulses
    }
  }

  int io = readIO();
  if (io < 0) {
    // Never move blind: stop until the sensors can be read again
    pulsesStopAll();
    if (++ioFailCount == 1) statusEvent(EVT_IO_FAIL);
    if (ioFailCount >= HOME_IO_FAIL_LIMIT) {
      raiseFault(allSpreadersMask());
    }
    return;
  }
  ioFailCount = 0;
  pulsesDataFresh();
  if (pulsesTakeStale()) {
    statusEvent(EVT_PULSE_TIMEOUT);
  }

  bool allOn = true;
  uint16_t failed = 0;
  uint16_t notOn = 0;
  for (int i = 0; i < NUM_GAPS; i++) {
    bool on = (io >> (PROXY_FIRST_BIT + i)) & 1;
    pulsesRun(i, !on);
    if (!on) {
      allOn = false;
      notOn |= spreaderBit(i);
      if (pulsesCount(i) > (uint32_t)HOME_MAX_STEPS) failed |= spreaderBit(i);
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

  if (digitalRead(INPUT_A2)) {
    // The manual DIP switch is on: start in manual mode without moving anything. Leaving manual mode homes automatically.
    Serial.println("Power up with the manual DIP switch on: no automatic home.");
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
    tsHomeMove = triggerMove();
    return tsHomeMove;
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

  if (moveFinished) tsHomeMove = false;

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

// Cancel a homing routine that is running (the manual DIP switch was turned on). A TeensyStep move that is part of it must be stopped by the caller.
// The positions the pulses have brought about are kept if they were known when the routine started.
void homingAbort() {
  if (homeState == HOME_CASCADE) {
    if (cascadeStartKnown) {
      for (int i = 0; i < NUM_GAPS; i++) setStepperPosition(i, cascadePositionSteps(i));
    } else {
      posKnown = false;
    }
    pulsesEnd();
  }
  homeState = HOME_IDLE;
  tsHomeMove = false;
  digitalWrite(OUTPUT_B2, LOW);
  statusEvent(EVT_MOVE_ABORTED, 1);
}

bool homingActive() {
  return homeState != HOME_IDLE;
}

bool homingBusy() {
  return homeState != HOME_IDLE || tsHomeMove;
}

bool positionsKnown() {
  return posKnown;
}

void setPositionsKnown(bool known) {
  posKnown = known;
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
  faultTypeValue = FAULT_NONE;
  maskPublished = false;
  publishMask();
  beginCascade();
}

// Over travel protection. CPU1 holds INPUT_A4 high while an over travel sensor is on. Stop the TeensyStep motion at once and raise a fault.
// Not checked during the direct pulse stage (homing closes the spreaders, which moves them away from the sensors) or in manual mode
// (manual mode blocks opening the end spreaders itself).
void overTravelService() {
  if (!OVERTRAVEL_ENABLED || faultOn || homeState == HOME_CASCADE || manualRequested()) return;
  if (!digitalRead(INPUT_A4)) return;

  emergencyStopMoves();

  // Which side? Ask CPU1 for the inputs. If that fails, report both spreaders.
  uint16_t mask = 0;
  int io = readIO();
  if (io >= 0) {
    if (io & (1 << OVERTRAVEL_LEFT_BIT)) mask |= spreaderBit(0);
    if (io & (1 << OVERTRAVEL_RIGHT_BIT)) mask |= spreaderBit(NUM_GAPS - 1);
  }
  if (mask == 0) mask = spreaderBit(0) | spreaderBit(NUM_GAPS - 1);
  raiseFault(mask, FAULT_OVERTRAVEL);
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

uint8_t faultType() {
  return faultTypeValue;
}

long cascadePositionSteps(int i) {
  if (!cascadeStartKnown) return 0;
  long p = cascadeStartPos[i] - (long)pulsesCount(i);
  return p < 0 ? 0 : p;
}
