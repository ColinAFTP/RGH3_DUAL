#include "constants.h"
#include "functions_io.h"
#include "variables.h"

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

// Initialise the hardwired CPU1/CPU2 signals in CPU2
void initCPU2HardIO() {
  // Pull-downs so a disconnected wire reads LOW instead of floating and giving false triggers
  pinMode(INPUT_A1, INPUT_PULLDOWN);
  pinMode(INPUT_A2, INPUT_PULLDOWN);
  pinMode(INPUT_A3, INPUT_PULLDOWN);
  pinMode(INPUT_A4, INPUT_PULLDOWN);
  pinMode(OUTPUT_B1, OUTPUT);
  pinMode(OUTPUT_B2, OUTPUT);
  pinMode(OUTPUT_B3, OUTPUT);
  pinMode(OUTPUT_B4, OUTPUT);
  digitalWrite(OUTPUT_B1, LOW);
  digitalWrite(OUTPUT_B2, LOW);
  digitalWrite(OUTPUT_B3, LOW);
  digitalWrite(OUTPUT_B4, LOW);
};

// Update the inputs
void inputsCheck() {
  // Pulse the load pin to load the current inputs into the shift registers
  digitalWrite(INPUTS_DATA_LOAD_PIN, LOW);
  delayMicroseconds(5);
  digitalWrite(INPUTS_DATA_LOAD_PIN, HIGH);
  delayMicroseconds(5);
  // Load the bits from the shift registers using the FastShiftIn library
  inputData = FSI->read16();
  // Invert the inputs because there are pull-up resistors
  inputData = ~inputData;
}

// Map the input data to an array of Boolean variables
void inputsStrip() {
  for (int i = 0; i < 16; i++) {
    digitalInput[i] = (inputData >> i) & 0x01;
  }
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

// Update the Home, At Target and Homing Fault status bits, and the two feedback relays.
// Home is calculated here from the home proximity sensors: all nine on and no fault.
// At Target and the fault come from CPU2 on the hardwired lines.
void feedbackCheck() {
  bool atTarget = digitalRead(INPUT_B2);
  bool fault = digitalRead(INPUT_B3);
  bool atHome = ((inputData & PROXY_ALL_MASK) == PROXY_ALL_MASK) && !fault;

  // Update the Modbus discrete status bits
  modbusServer.discreteInputWrite(ADDR_HOME, atHome);
  modbusServer.discreteInputWrite(ADDR_MOVE_DONE, atTarget);
  modbusServer.discreteInputWrite(ADDR_HOMING_FAULT, fault);

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

