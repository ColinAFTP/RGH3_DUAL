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

// Manual mode.
//
// Manual mode is on while the manual DIP switch (hardwired line INPUT_A2 from CPU1) or the PLC (coil ADDR_MANUAL, read over I2C) asks for it.
//  - The DIP switch overrides the PLC: turning it on stops a running move (with deceleration) or homing routine at once. A PLC request waits
//    until CPU2 stands still.
//  - The PLC moves one spreader at a time with the direct pulse engine (the same as homing, no TeensyStep): register ADDR_MANUAL_PTR is the spreader
//    number and coil ADDR_MANUAL_OPN / ADDR_MANUAL_CLS moves it forward (open) / backward (close) while the coil is on.
//  - Closing is the mirror image: the spreader closes until its own home sensor is on (touching its inner neighbour), then that neighbour is pushed along too,
//    and so on inwards, until the whole chain is up against the static spreader and nothing can move any more.
//  - Opening also pushes along every spreader further out that is touching the one in front of it (its home sensor is on), so a spreader never runs
//    into the next one. Opening stops at the travel limit or when an over travel sensor is on.
//  - Leaving manual mode homes automatically. Pattern requests are refused while manual mode is on (main.cpp).
// The positions are counted from the pulses and given back to TeensyStep whenever the movement stops, so the gripper still knows where it is.

namespace {

enum ManualState { M_OFF, M_WAITING, M_ACTIVE };
ManualState state = M_OFF;

bool softRequest = false;               // The PLC has requested manual mode (last value read from CPU1)
ManualCommand cmd = {0, 0, 0};          // The last manual command read from CPU1
bool haveCmd = false;
uint32_t lastPollMs = 0;
int linkFailStreak = 0;

bool jogging = false;                   // The engine is running for a jog
int jogSpreader = 0;                    // Spreader number being jogged
int jogDir = 0;                         // +1 open, -1 close
bool latched = false;                   // A jog was stopped by a limit: the coil must be released before a new jog starts

long livePos[NUM_GAPS];                 // Stepper positions in steps, counted from the pulses
uint32_t seenCount[NUM_GAPS];           // Pulse counts already added to livePos
int dirState[NUM_GAPS];                 // Direction each stepper's pin is set to: +1 open, -1 close
uint32_t stoppedUs[NUM_GAPS];           // micros() when each stepper was last told to stop
uint32_t jogStartCount[NUM_GAPS];       // Pulse counts at the start of this jog

// Add the pulses emitted since the last call to the positions
void foldPositions() {
  if (!pulsesEngineOn()) return;
  for (int i = 0; i < NUM_GAPS; i++) {
    uint32_t count = pulsesCount(i);
    livePos[i] += (long)dirState[i] * (long)(count - seenCount[i]);
    seenCount[i] = count;
  }
}

// Stop everything, take over the counted positions and shut the engine down
void finishJog(int reason) {
  if (!pulsesEngineOn()) {
    jogging = false;
    return;
  }
  pulsesStopAll();
  delayMicroseconds(60);                // Let the last pulse end
  foldPositions();
  pulsesEnd();
  for (int i = 0; i < NUM_GAPS; i++) {
    if (livePos[i] < 0) livePos[i] = 0;
    setStepperPosition(i, livePos[i]);
  }
  if (jogging) statusEvent(EVT_JOG_STOP, reason);
  jogging = false;
}

// Ask CPU1 for the manual command. Returns false if the transfer failed.
bool pollCommand() {
  lastPollMs = millis();
  ManualCommand c;
  if (readManualCommand(c)) {
    cmd = c;
    haveCmd = true;
    linkFailStreak = 0;
    softRequest = (c.flags & MANUAL_FLAG_SOFT) != 0;
    if (pulsesEngineOn()) pulsesDataFresh();
    return true;
  }
  // Never move blind
  pulsesStopAll();
  if (++linkFailStreak >= HOME_IO_FAIL_LIMIT && jogging) {
    finishJog(EVT_JOG_LINK);
  }
  return false;
}

// Start, stop or reverse one stepper. The direction pin is only changed while the stepper has been stopped for a moment.
void applyRun(int i, bool run, int dir) {
  if (run) {
    if (!pulsesIsRunning(i)) {
      if (dirState[i] != dir) {
        if ((uint32_t)(micros() - stoppedUs[i]) < 100) return;      // The last pulse must be over first: try again on the next pass
        foldPositions();
        pulsesSetDirection(i, dir);
        dirState[i] = dir;
      }
      pulsesRun(i, true);
    } else if (dirState[i] != dir) {
      pulsesRun(i, false);
      stoppedUs[i] = micros();
    }
  } else if (pulsesIsRunning(i)) {
    pulsesRun(i, false);
    stoppedUs[i] = micros();
  }
}

void enterManual(bool dipLine) {
  state = M_ACTIVE;
  digitalWrite(OUTPUT_B2, LOW);         // At Target is off while in manual mode
  updateStepperPositions();
  for (int i = 0; i < NUM_GAPS; i++) {
    livePos[i] = stepperPositions[i];
    seenCount[i] = 0;
    dirState[i] = -1;
    stoppedUs[i] = 0;
  }
  latched = false;
  jogging = false;
  statusEvent(EVT_MANUAL_ON, (dipLine ? 1 : 0) | (softRequest ? 2 : 0));
  Serial.println("Manual mode on.");
}

void exitManual() {
  finishJog(EVT_JOG_MODE_END);
  state = M_OFF;
  statusEvent(EVT_MANUAL_OFF);
  Serial.println("Manual mode off: automatic home.");
  // Leaving manual mode homes automatically. A fault blocks it: the fault must be reset first.
  if (faultActive()) {
    statusEvent(EVT_AUTO_HOME_SKIPPED);
    return;
  }
  requestStarted();
  if (!homeRequest()) {
    requestRefused(EVT_REASON_HOME_FAILED);
  }
}

// One pass of the jog logic (manual mode active)
void jogService() {
  bool needPoll = jogging || (millis() - lastPollMs >= MANUAL_POLL_IDLE_MS);
  bool fresh = false;
  if (needPoll) fresh = pollCommand();
  if (!haveCmd) return;
  if (jogging && !fresh) return;        // The engine was stopped by pollCommand(); it restarts when the link is back

  foldPositions();

  bool open = (cmd.flags & MANUAL_FLAG_OPEN) != 0;
  bool close = (cmd.flags & MANUAL_FLAG_CLOSE) != 0;
  int idx = spreaderIndex(cmd.spreader);
  int want = 0;
  if (open != close) {
    if (idx < 0) {
      static int lastBad = -1;
      if (lastBad != cmd.spreader) {
        lastBad = cmd.spreader;
        statusEvent(EVT_MANUAL_BAD_SPREADER, cmd.spreader);
      }
    } else {
      want = open ? +1 : -1;
    }
  }

  if (want == 0) {
    latched = false;
    if (jogging) finishJog(EVT_JOG_RELEASED);
    return;
  }
  if (jogging && (cmd.spreader != jogSpreader || want != jogDir)) {
    finishJog(EVT_JOG_RELEASED);        // Another spreader or direction: restart cleanly
    return;
  }
  if (latched) return;

  // Which steppers should run?
  bool run[NUM_GAPS] = {false};
  int stopReason = -1;
  uint16_t in = cmd.inputs;
  auto proxyOn = [&](int k) { return ((in >> (PROXY_FIRST_BIT + k)) & 1) != 0; };

  if (want < 0) {
    // Close: the mirror image of opening. The spreader moves until it touches its inner neighbour (its own home sensor is on). From then on the
    // inner neighbour is pushed along too, and so on inwards, until everything in the chain is up against the one inside it (the last one
    // against the static spreader). Then nothing can move any more.
    bool leftSide = idx < NUM_LEFT_SPREADERS;
    int inward = leftSide ? +1 : -1;                    // Towards the static spreader, in stepper index
    int innermost = leftSide ? NUM_LEFT_SPREADERS - 1 : NUM_LEFT_SPREADERS;
    // canMove[j]: spreader j can close, because it is not touching its inner neighbour yet, or that neighbour can move out of the way
    bool canMove[NUM_GAPS] = {false};
    for (int j = innermost; ; j -= inward) {
      canMove[j] = !proxyOn(j) || (j != innermost && canMove[j + inward]);
      if (j == idx) break;
    }
    run[idx] = canMove[idx];
    for (int j = idx + inward; leftSide ? (j <= innermost) : (j >= innermost); j += inward) {
      // The inner neighbour is pushed along while the spreader in front of it is touching it
      run[j] = run[j - inward] && proxyOn(j - inward) && canMove[j];
    }
    if (!run[idx]) stopReason = EVT_JOG_TOUCHING;
  } else {
    // Open: the spreader itself, and every spreader further out that is touching the one in front of it
    run[idx] = true;
    bool leftSide = idx < NUM_LEFT_SPREADERS;
    int step = leftSide ? -1 : +1;      // Outwards in stepper index
    for (int m = idx + step; leftSide ? (m >= 0) : (m < NUM_GAPS); m += step) {
      if (run[m - step] && proxyOn(m)) run[m] = true; else break;
    }
    if (positionsKnown()) {
      for (int i = 0; i < NUM_GAPS; i++) {
        if (run[i] && livePos[i] >= MAX_STEPS) stopReason = EVT_JOG_LIMIT;
      }
    }
    if (OVERTRAVEL_ENABLED) {
      if (run[0] && (in & (1 << OVERTRAVEL_LEFT_BIT))) stopReason = EVT_JOG_OVERTRAVEL;
      if (run[NUM_GAPS - 1] && (in & (1 << OVERTRAVEL_RIGHT_BIT))) stopReason = EVT_JOG_OVERTRAVEL;
    }
  }
  if (jogging) {
    // One jog is limited in length, in case a home sensor has failed
    for (int i = 0; i < NUM_GAPS; i++) {
      if (run[i] && pulsesCount(i) - jogStartCount[i] > (uint32_t)MANUAL_MAX_JOG_STEPS) stopReason = EVT_JOG_LIMIT;
    }
  }

  if (stopReason >= 0) {
    if (jogging) finishJog(stopReason);
    latched = true;                     // The coil must be released before another jog starts
    return;
  }

  if (!jogging) {
    if (!pulsesEngineOn()) {
      pulsesBegin(MANUAL_PULSE_RATE, MANUAL_START_RATE, MANUAL_RAMP_MS);
      for (int i = 0; i < NUM_GAPS; i++) {
        seenCount[i] = 0;
        dirState[i] = -1;               // pulsesBegin() points every stepper in the close direction
        stoppedUs[i] = 0;
      }
    }
    for (int i = 0; i < NUM_GAPS; i++) jogStartCount[i] = pulsesCount(i);
    jogging = true;
    jogSpreader = cmd.spreader;
    jogDir = want;
    statusEvent(EVT_JOG_START, cmd.spreader + (want > 0 ? 100 : 200));
  }
  for (int i = 0; i < NUM_GAPS; i++) applyRun(i, run[i], want);
}

}  // namespace

void manualService() {
  bool dipLine = digitalRead(INPUT_A2);
  // While manual mode is off the PLC request is polled slowly. The DIP switch is a hardwired line and needs no polling.
  if (state != M_ACTIVE && millis() - lastPollMs >= 100) {
    pollCommand();
  }
  bool requested = dipLine || softRequest;

  switch (state) {
    case M_OFF:
      if (requested) {
        state = M_WAITING;
        if (dipLine) {
          // The DIP switch is the hardware override: stop whatever is moving now
          if (homingActive()) homingAbort();
          stopMovesRamped();
        }
      }
      break;

    case M_WAITING:
      if (!requested) {
        state = M_OFF;
      } else if (!moveInProgress() && !homingActive()) {
        enterManual(dipLine);
      }
      break;

    case M_ACTIVE:
      if (!requested) {
        exitManual();
      } else {
        jogService();
      }
      break;
  }
}

bool manualActive() {
  return state == M_ACTIVE;
}

bool manualRequested() {
  return state != M_OFF;
}

long manualPositionSteps(int i) {
  return livePos[i];
}
