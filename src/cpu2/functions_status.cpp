#include <Arduino.h>
#include <i2c_driver.h>
#include <i2c_driver_wire.h>

#include "constants.h"
#include "functions_homing.h"
#include "functions_i2c.h"
#include "functions_manual.h"
#include "functions_status.h"
#include "functions_steppers.h"
#include "structures.h"
#include "variables.h"

namespace {

constexpr int EVENT_QUEUE_SIZE = 16;
StatusEvent eventQueue[EVENT_QUEUE_SIZE];
int eventHead = 0;                      // Next free slot
int eventCount = 0;                     // Events waiting to be sent
uint32_t lastSendTime = 0;
uint8_t refusedReason = 0;              // Reason the last request was refused, 0 = not refused

// The state that is reported to CPU1
uint8_t currentState() {
  if (manualActive()) return STATE_MANUAL;
  if (faultActive()) return STATE_FAULT;
  int stage = homingStage();
  if (stage == 2) return STATE_HOMING_PULSES;
  if (stage == 1) return STATE_HOMING_APPROACH;
  if (moveInProgress()) return STATE_MOVING;
  if (!positionsKnown()) return STATE_UNKNOWN_POS;
  return STATE_IDLE;
}

}  // namespace

void statusEvent(uint8_t code, int16_t arg) {
  if (eventCount == EVENT_QUEUE_SIZE) {
    eventCount--;                       // Queue full: drop the oldest event
  }
  eventQueue[eventHead] = {code, arg};
  eventHead = (eventHead + 1) % EVENT_QUEUE_SIZE;
  eventCount++;
}

void requestStarted() {
  refusedReason = 0;
  digitalWrite(OUTPUT_B4, LOW);
}

void requestRefused(int reason) {
  refusedReason = (uint8_t)reason;
  digitalWrite(OUTPUT_B4, HIGH);
  statusEvent(EVT_REFUSED, reason);
}

void statusService() {
  uint32_t now = millis();
  bool due = (now - lastSendTime >= STATUS_PERIOD_MS) || (eventCount > 0 && now - lastSendTime >= 50);
  if (!due) return;
  lastSendTime = now;

  StatusPacket packet;
  packet.state = currentState();
  packet.flags = (positionsKnown() ? STATUS_FLAG_KNOWN : 0) | (homingBusy() ? STATUS_FLAG_HOMING : 0) | (manualActive() ? STATUS_FLAG_MANUAL : 0);
  packet.faultMask = faultMask();
  packet.faultType = faultType();
  if (manualActive()) {
    // Manual mode counts the pulses of the jog that is running
    for (int i = 0; i < NUM_GAPS; i++) {
      packet.positions[i] = (int16_t)lroundf(manualPositionSteps(i) * 10.0f / STEPS_PER_MM);
    }
  } else if (homingStage() == 2) {
    // The direct pulse stage does not use TeensyStep, so its positions are estimated from the pulses counted
    for (int i = 0; i < NUM_GAPS; i++) {
      packet.positions[i] = (int16_t)lroundf(cascadePositionSteps(i) * 10.0f / STEPS_PER_MM);
    }
  } else {
    updateStepperPositions();
    for (int i = 0; i < NUM_GAPS; i++) {
      packet.positions[i] = (int16_t)lroundf(stepperPositions[i] * 10.0f / STEPS_PER_MM);
    }
  }

  // The oldest events go first. They are only removed from the queue once CPU1 has acknowledged the transfer.
  int n = eventCount < STATUS_MAX_EVENTS ? eventCount : STATUS_MAX_EVENTS;
  int tail = (eventHead - eventCount + EVENT_QUEUE_SIZE) % EVENT_QUEUE_SIZE;
  packet.numEvents = n;
  for (int i = 0; i < STATUS_MAX_EVENTS; i++) {
    packet.events[i] = (i < n) ? eventQueue[(tail + i) % EVENT_QUEUE_SIZE] : StatusEvent{0, 0};
  }
  packet.refusedReason = refusedReason;
  uint32_t ioReads;
  uint16_t ioFails, otherFails;
  i2cStats(ioReads, ioFails, otherFails);
  packet.ioReads = ioReads;
  packet.ioFails = ioFails;
  packet.otherFails = otherFails;

  Wire2.beginTransmission(0x40);
  Wire2.write(I2C_CMD_STATUS);
  Wire2.write((const uint8_t*)&packet, sizeof(packet));
  if (Wire2.endTransmission() == 0) {
    eventCount -= n;
  } else {
    i2cCountOtherFail();
  }
}
