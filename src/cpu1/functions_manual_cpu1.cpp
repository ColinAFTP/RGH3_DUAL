#include <Arduino.h>

#include "constants.h"
#include "functions_manual_cpu1.h"
#include "functions_web.h"
#include "variables_cpu1.h"

// CPU1 side of manual mode.
//  - The manual DIP switch is debounced and put on the hardwired line OUTPUT_A2 to CPU2 (CPU2 stops any motion the moment it goes high).
//  - The PLC request (coil ADDR_MANUAL), the jog coils (ADDR_MANUAL_OPN / ADDR_MANUAL_CLS) and the spreader register (ADDR_MANUAL_PTR) are
//    packed for CPU2, which reads them over I2C (I2C_CMD_MANUAL, see functions_i2c_cpu1.cpp).
//  - Dead man: the jog coils are cancelled if the PLC is not connected or has sent no Modbus request for MANUAL_COMM_TIMEOUT_MS. A PLC that loses
//    power or its cable while a coil is on must not leave a spreader running.
//  - The PLC is told: DI ADDR_MANUAL_MODE = CPU2 is in manual mode, coil ADDR_HOMING = CPU2 is homing.

namespace {

bool dipRaw = false;                    // Last raw reading of the DIP switch
bool dipStable = false;                 // Debounced state
uint32_t dipChangeMs = 0;
bool softOn = false;
bool softPrev = false;
uint8_t openClose = 0;
uint8_t spreader = 0;

bool readDip() {
  return digitalRead(DIP_MANUAL_PIN) == DIP_MANUAL_ACTIVE_LEVEL;
}

}  // namespace

void manualInit() {
  pinMode(DIP_MANUAL_PIN, INPUT_PULLDOWN);
  dipRaw = readDip();
  dipStable = dipRaw;
  digitalWrite(OUTPUT_A2, dipStable ? HIGH : LOW);
}

void manualCpu1Service() {
  // The DIP switch, debounced
  bool raw = readDip();
  if (raw != dipRaw) {
    dipRaw = raw;
    dipChangeMs = millis();
  }
  if (dipRaw != dipStable && millis() - dipChangeMs >= DIP_DEBOUNCE_MS) {
    dipStable = dipRaw;
    digitalWrite(OUTPUT_A2, dipStable ? HIGH : LOW);
    logEvent("Manual DIP switch %s", dipStable ? "ON" : "OFF");
  }

  // The PLC request and jog commands
  softOn = modbusServer.coilRead(ADDR_MANUAL) != 0;
  if (softOn != softPrev) {
    softPrev = softOn;
    logEvent("PLC manual mode request %s", softOn ? "ON" : "OFF");
  }
  bool open = modbusServer.coilRead(ADDR_MANUAL_OPN) != 0;
  bool close = modbusServer.coilRead(ADDR_MANUAL_CLS) != 0;
  long reg = modbusServer.holdingRegisterRead(ADDR_MANUAL_PTR);
  spreader = (reg < 0 || reg > 255) ? 0 : (uint8_t)reg;

  // Dead man: no PLC, no movement
  bool commOk = ethernetClient.connected() && (millis() - lastModbusRequestMs < MANUAL_COMM_TIMEOUT_MS);
  if (!commOk || !(dipStable || softOn)) {
    open = false;
    close = false;
  }
  openClose = (open ? 1 : 0) | (close ? 2 : 0);

  // What CPU2 reads over I2C. Written as one byte each, so the interrupt never sees a half updated value.
  manualFlagsShared = (dipStable ? MANUAL_FLAG_DIP : 0) | (softOn ? MANUAL_FLAG_SOFT : 0) |
                      (open ? MANUAL_FLAG_OPEN : 0) | (close ? MANUAL_FLAG_CLOSE : 0);
  manualSpreaderShared = spreader;

  // Tell the PLC: manual mode active (DI) and homing in progress (coil, written by CPU1, read by the PLC)
  static bool activeWritten = false, activeWrittenValid = false;
  bool active = manualCpu2Active();
  if (!activeWrittenValid || active != activeWritten) {
    activeWritten = active;
    activeWrittenValid = true;
    modbusServer.discreteInputWrite(ADDR_MANUAL_MODE, active);
  }
  bool homing = cpu2Online() && (cpu2Flags() & STATUS_FLAG_HOMING);
  if ((modbusServer.coilRead(ADDR_HOMING) != 0) != homing) {
    modbusServer.coilWrite(ADDR_HOMING, homing ? 1 : 0);
  }
}

bool manualDipOn() {
  return dipStable;
}

bool manualSoftOn() {
  return softOn;
}

bool manualCpu2Active() {
  return cpu2Online() && (cpu2Flags() & STATUS_FLAG_MANUAL);
}

bool manualModeOn() {
  return dipStable || softOn || manualCpu2Active();
}

uint8_t manualOpenClose() {
  return openClose;
}

uint8_t manualSpreader() {
  return spreader;
}

// The raw levels of the three DIP switches, for the web page: bit 0 = PCB DIP 1 (IP address bit 0), bit 1 = PCB DIP 2 (IP address bit 1), bit 2 = PCB DIP 3 (manual mode)
uint8_t manualDipPinsRaw() {
  return (digitalRead(DIP_PCB1_PIN) ? 1 : 0) | (digitalRead(DIP_PCB2_PIN) ? 2 : 0) | (digitalRead(DIP_PCB3_PIN) ? 4 : 0);
}
