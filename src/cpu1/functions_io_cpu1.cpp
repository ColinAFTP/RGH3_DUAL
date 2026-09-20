#include "constants.h"
#include "functions_io.h"
#include "functions_web.h"
#include "variables_cpu1.h"

// Initialise the shift registers after the pin modes have been set
void initShiftRegisters() {
  pinMode(INPUTS_DATA_PIN, INPUT);
  pinMode(INPUTS_DATA_CLOCK_PIN, OUTPUT);
  pinMode(INPUTS_DATA_LOAD_PIN, OUTPUT);
  // Now that pins are configured, construct the object
  FSI = new FastShiftIn(INPUTS_DATA_PIN, INPUTS_DATA_CLOCK_PIN, MSBFIRST);
}

// Initialise the hardwired CPU1/CPU2 signals in CPU1
void initCPU1HardIO() {
  pinMode(OUTPUT_A1, OUTPUT);
  pinMode(OUTPUT_A2, OUTPUT);
  pinMode(OUTPUT_A3, OUTPUT);
  pinMode(OUTPUT_A4, OUTPUT);
  digitalWrite(OUTPUT_A1, LOW);
  digitalWrite(OUTPUT_A2, LOW);
  digitalWrite(OUTPUT_A3, LOW);
  digitalWrite(OUTPUT_A4, LOW);
  // Pull-downs so a disconnected wire reads LOW instead of floating
  pinMode(INPUT_B1, INPUT_PULLDOWN);
  pinMode(INPUT_B2, INPUT_PULLDOWN);
  pinMode(INPUT_B3, INPUT_PULLDOWN);
  pinMode(INPUT_B4, INPUT_PULLDOWN);
};

// Update the inputs
void inputsCheck() {
  static bool started = false;
  static uint8_t holdCount[16];         // Consecutive samples for which each input has differed from its filtered value

  // Pulse the load pin to load the current inputs into the shift registers
  digitalWrite(INPUTS_DATA_LOAD_PIN, LOW);
  delayMicroseconds(5);
  digitalWrite(INPUTS_DATA_LOAD_PIN, HIGH);
  delayMicroseconds(5);
  // Load the bits from the shift registers using the FastShiftIn library
  uint16_t raw = FSI->read16();
  // Invert the inputs because there are pull-up resistors
  raw = ~raw;
  inputDataRaw = raw;

  if (!started) {
    // The first sample is taken as it is, so start up is not delayed
    started = true;
    inputData = raw;
    return;
  }

  // Glitch filter: an input changes only after its raw value has been different from the filtered value for
  // INPUT_FILTER_SAMPLES samples in a row. A single-sample noise pulse never gets through.
  uint16_t filtered = inputData;
  for (int b = 0; b < 16; b++) {
    bool rawBit = (raw >> b) & 1;
    bool filteredBit = (filtered >> b) & 1;
    if (rawBit == filteredBit) {
      holdCount[b] = 0;
    } else if (++holdCount[b] >= INPUT_FILTER_SAMPLES) {
      filtered ^= (uint16_t)1 << b;
      holdCount[b] = 0;
    }
  }
  inputData = filtered;
}

// This subroutine clocks data serially to the 74HC595 chips. There are 2 in series so that 16 relays can be controlled at the same time. 
void relayControl(word outputData) {
  // This shifts 16 bits out MSB first, on the rising edge of the clock
  int i=0;
  int pinState;
  // Set the relay shift register control pins to output
  pinMode(RELAY_DATA_LATCH_PIN, OUTPUT);
  pinMode(RELAY_DATA_CLOCK_PIN, OUTPUT);
  pinMode(RELAY_DATA_PIN, OUTPUT);
  // Clear everything out just in case to prepare shift register for bit shifting
  digitalWrite(RELAY_DATA_LATCH_PIN, 0);
  digitalWrite(RELAY_DATA_PIN, 0);
  digitalWrite(RELAY_DATA_CLOCK_PIN, 0);
  // For each bit in the word outputData&#xFFFD;
  // Notice that we are COUNTING DOWN in our for loop so that %00000001 or "1" will go through such that it will be pin Q0 that lights.
  for (i = 15; i >= 0; i--) {
    digitalWrite(RELAY_DATA_CLOCK_PIN, 0);
    // The << function is a bit shift left function. It will shift the "1" left by the value in index "i". This is how the bit mask moves through the 16 bits of the outputData word and sets pinState accordingly.
    if ( outputData & (1<<i) ) {
      pinState= 1;
    } else {
      pinState= 0;
    }
    // Set the data pin HIGH or LOW depending on pinState
    digitalWrite(RELAY_DATA_PIN, pinState);
    // Shift the new data bit on upstroke of clock pin
    digitalWrite(RELAY_DATA_CLOCK_PIN, 1);
    // Zero the data pin after shift to prevent bleed through
    digitalWrite(RELAY_DATA_PIN, 0);
  }
  // Stop shifting
  digitalWrite(RELAY_DATA_CLOCK_PIN, 0);
  // Latch new data to outputs
  digitalWrite(RELAY_DATA_LATCH_PIN, 1);
}

// Update the Home, At Target, Fault, Move Refused and CPU2 Online status bits, the fault type and refused reason registers,
// and the two feedback relays.
//  - Home: all nine home sensors on, no fault, and CPU2 running.
//  - At Target: CPU2 says so, CPU2 is running, and it is not blanked. It is blanked from the moment the PLC selects a new pattern
//    until CPU2 has dropped its own line, so the PLC never sees the old "at target" after a new request.
//  - Fault: CPU2 has a fault (homing failed or over travel), or CPU2 is lost.
//  - Move Refused: CPU1 or CPU2 refused the last request. The reason is in register ADDR_REFUSED_REASON.
void feedbackCheck() {
  bool cpu2Ok = cpu2Online();
  bool cpu2Fault = digitalRead(INPUT_B3);
  bool fault = cpu2Fault || cpu2Lost();

  // At Target blanking ends when CPU2 has dropped its line, or after 500 ms (the length of the start move pulse)
  bool b2 = digitalRead(INPUT_B2);
  if (atTargetBlank && (!b2 || millis() - atTargetBlankStart > 500)) {
    atTargetBlank = false;
  }
  bool atTarget = b2 && !atTargetBlank && cpu2Ok;
  bool atHome = ((inputData & PROXY_ALL_MASK) == PROXY_ALL_MASK) && !fault && cpu2Ok;

  // The refused flag: from CPU1 (invalid pattern) or from CPU2 (line B4). The reason comes with CPU2's next status.
  bool refused = cpu1Refused || digitalRead(INPUT_B4);
  uint8_t reason = cpu1Refused ? cpu1RefusedReason : (refused ? cpu2RefusedReason() : 0);
  uint8_t faultType = cpu2Lost() ? FAULT_CPU2 : faultTypeRx;
  if (!fault) faultType = FAULT_NONE;

  statusHome = atHome;
  statusAtTarget = atTarget;
  statusFault = fault;
  statusRefused = refused;
  statusRefusedReason = reason;

  // Update the Modbus discrete status bits
  modbusServer.discreteInputWrite(ADDR_HOME, atHome);
  modbusServer.discreteInputWrite(ADDR_MOVE_DONE, atTarget);
  modbusServer.discreteInputWrite(ADDR_HOMING_FAULT, fault);
  modbusServer.discreteInputWrite(ADDR_MOVE_REFUSED, refused);
  modbusServer.discreteInputWrite(ADDR_CPU2_ONLINE, cpu2Ok);

  // The two registers are only written when they change
  if (faultType != faultTypeShown) {
    faultTypeShown = faultType;
    modbusServer.holdingRegisterWrite(ADDR_FAULT_TYPE, faultType);
  }
  static uint8_t reasonWritten = 255;
  if (reason != reasonWritten) {
    reasonWritten = reason;
    modbusServer.holdingRegisterWrite(ADDR_REFUSED_REASON, reason);
  }
  
  // Set relay 1 (bit 0) if home and relay 2 (bit 1) if at target
  word newRelayData = relayData;
  if (atHome) {
    newRelayData |= 0x01;
  } else {
    newRelayData &= ~0x01;
  }
  if (atTarget) {
    newRelayData |= 0x02;
  } else {
    newRelayData &= ~0x02;
  }
  
  // Write the updated relay data back to the holding register so the PLC can see it
  if (newRelayData != relayData) {
    relayData = newRelayData;
    modbusServer.holdingRegisterWrite(ADDR_RELAYS, relayData);
  }
}

