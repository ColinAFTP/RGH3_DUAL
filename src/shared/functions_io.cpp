#include "constants.h"
#include "functions_io.h"
#include "variables.h"

// Initialise the shift registers after the pin modes have been set
void initShiftRegisters() {
  pinMode(INPUTS_DATA_PIN, INPUT);
  pinMode(INPUTS_DATA_CLOCK_PIN, OUTPUT);
  // Now that pins are configured, construct the object
  FSI = new FastShiftIn(INPUTS_DATA_PIN, INPUTS_DATA_CLOCK_PIN, MSBFIRST);
}

// Initialise the hardwired CPU1/CPU2 signals in CPU1
void initCPU1HardIO() {
  pinMode(OUTPUT_A1, OUTPUT);
  pinMode(OUTPUT_A2, OUTPUT);
  pinMode(OUTPUT_A3, OUTPUT);
  pinMode(OUTPUT_A4, OUTPUT);
  pinMode(INPUT_B1, INPUT);
  pinMode(INPUT_B2, INPUT);
  pinMode(INPUT_B3, INPUT);
  pinMode(INPUT_B4, INPUT);
};                  

// Initialise the hardwired CPU1/CPU2 signals in CPU2
void initCPU2HardIO() {
  pinMode(INPUT_A1, INPUT);
  pinMode(INPUT_A2, INPUT);
  pinMode(INPUT_A3, INPUT);
  pinMode(INPUT_A4, INPUT);
  pinMode(OUTPUT_B1, OUTPUT);
  pinMode(OUTPUT_B2, OUTPUT);
  pinMode(OUTPUT_B3, OUTPUT);
  pinMode(OUTPUT_B4, OUTPUT);
};                  

// Update the inputs
void inputsCheck() {
  // Pulse the load pin to load the current inputs into the shift registers
  digitalWrite(INPUTS_DATA_LOAD_PIN, LOW);
  delay(1);
  digitalWrite(INPUTS_DATA_LOAD_PIN, HIGH);
  delay(1);
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

// Check for "at home" and "at target" feedback signals from CPU2
void feedbackCheck() {
  bool atHome = digitalRead(INPUT_B1);
  bool atTarget = digitalRead(INPUT_B2);
  
  // Update the Modbus discrete status bits
  modbusServer.discreteInputWrite(ADDR_HOME, atHome);
  modbusServer.discreteInputWrite(ADDR_MOVE_DONE, atTarget);
  
  // Set relay 1 (bit 0) if at home
  if (atHome) {
    relayData |= 0x01;
  } else {
    relayData &= ~0x01;
  }
  
  // Set relay 2 (bit 1) if at target
  if (atTarget) {
    relayData |= 0x02;
  } else {
    relayData &= ~0x02;
  }
  
  // Write the updated relay data back to the holding register so the PLC can see it
  modbusServer.holdingRegisterWrite(ADDR_RELAYS, relayData);
}

