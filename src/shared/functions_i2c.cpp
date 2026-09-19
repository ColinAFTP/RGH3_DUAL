#include <i2c_driver.h>
#include <i2c_driver_wire.h>

#include "functions_i2c.h"
#include "variables.h"

// Copy values from each gap pattern to a single array to be used for I2C communications
void copyToTransmitData() {
    for (int r = 0; r < NUM_PATTERNS; r++) {
        for (int c = 0; c < STRIDE_GAPS; c++) {
            transmitPacket.patternData[r][c] = gapArrays[r][c];
        }
    }

    transmitPacket.stepperSpeed = stepperSpeed;
}

// Copy values from a single array used for I2C communications to each gap pattern
void copyFromReceiveData() {
    for (int r = 0; r < NUM_PATTERNS; r++) {
        for (int c = 0; c < STRIDE_GAPS; c++) {
            gapArrays[r][c] = receivePacket.patternData[r][c];
        }
    }

    stepperSpeed = receivePacket.stepperSpeed;
    // Serial.print("   | Stepper speed: ");
    // Serial.println(stepperSpeed);
}

void onI2CRequest() {
    if (i2cCommand == 1) {
        // Send IO data
        Wire2.write((byte*)&inputData, sizeof(inputData));
    }
    else if (i2cCommand == 2) {
        // Send full gap pattern block
        copyToTransmitData();
        Wire2.write((byte*)&transmitPacket, sizeof(transmitPacket));
    }
    else if (i2cCommand == 3) {
        // Send pattern selection
        Wire2.write((byte*)&patternSelection, sizeof(patternSelection));
    }
}

// Function that requests the 16 bit input word from the slave (CPU1).
// This function is called from CPU2. It is called about every millisecond while homing, so it does not print.
// Returns the input word (0 to 65535), or -1 if the transfer failed.
int readIO() {
  uint16_t value = 0;

  // Tell CPU1 we want IO data
  Wire2.beginTransmission(0x40);
  Wire2.write(I2C_CMD_IO);
  if (Wire2.endTransmission() != 0) {
    return -1;
  }

  if (Wire2.requestFrom(0x40, sizeof(value)) == sizeof(value)) {
    Wire2.readBytes((byte*)&value, sizeof(value));
    return value;
  }

  return -1;
}

// Function that sends the bitmask of spreaders that failed to home to the slave (CPU1).
// This function is called from CPU2. Returns false if the transfer failed.
bool writeFaultMask(uint16_t mask) {
  Wire2.beginTransmission(0x40);
  Wire2.write(I2C_CMD_FAULT_MASK);
  Wire2.write((uint8_t)(mask & 0xFF));
  Wire2.write((uint8_t)(mask >> 8));
  return Wire2.endTransmission() == 0;
}

// Function that requests pattern gap data from the slave (CPU1). 
// This function is called from CPU2.
bool readGapPatterns() {
    Wire2.beginTransmission(0x40);
    Wire2.write(2);
    Wire2.endTransmission();

    // Serial.print("Requesting (");
    // Serial.print(sizeof(receivePacket));
    // Serial.println(" bytes)... ");

    if (Wire2.requestFrom(0x40, sizeof(receivePacket))) {
        Wire2.readBytes((byte*)&receivePacket, sizeof(receivePacket));
        copyFromReceiveData();
        return true;
    }
    Serial.println("Gap pattern read over I2C failed");
    return false;
}

// Function that requests pattern selection number from the slave (CPU1). 
// This function is called from CPU2.
int readPattern() {
  int value = 0;

  // Tell CPU1 we want the pattern selection (command = 3)
  Wire2.beginTransmission(0x40);
  Wire2.write(3);
  Wire2.endTransmission();

//   Serial.print("Requesting ("); 
//   Serial.print(sizeof value); 
//   Serial.println(" bytes)... ");
  if (Wire2.requestFrom(0x40, sizeof(value))) {
    Wire2.readBytes((byte*)&value, sizeof(value));
    return value;
  }

  Serial.println("Pattern selection read over I2C failed");
  return -1;  
}
