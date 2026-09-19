#include <i2c_driver.h>
#include <i2c_driver_wire.h>

#include "functions_i2c.h"
#include "variables.h"

// I2C functions of CPU2, which is the master. CPU1 is the slave (address 0x40).

// Copy values from the received I2C packet to the gap pattern array and the stepper speed
void copyFromReceiveData() {
    for (int r = 0; r < NUM_PATTERNS; r++) {
        for (int c = 0; c < STRIDE_GAPS; c++) {
            gapArrays[r][c] = receivePacket.patternData[r][c];
        }
    }

    stepperSpeed = receivePacket.stepperSpeed;
}

// Function that requests the 16 bit input word from the slave (CPU1). 
// It is called about every millisecond while homing, so it does not print.
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
// Returns false if the transfer failed.
bool writeFaultMask(uint16_t mask) {
  Wire2.beginTransmission(0x40);
  Wire2.write(I2C_CMD_FAULT_MASK);
  Wire2.write((uint8_t)(mask & 0xFF));
  Wire2.write((uint8_t)(mask >> 8));
  return Wire2.endTransmission() == 0;
}

// Function that requests all pattern gap data and the stepper speed from the slave (CPU1). 
// Returns false if the I2C transfer failed (the old data is kept).
bool readGapPatterns() {
    Wire2.beginTransmission(0x40);
    Wire2.write(I2C_CMD_GAPS);
    Wire2.endTransmission();

    if (Wire2.requestFrom(0x40, sizeof(receivePacket))) {
        Wire2.readBytes((byte*)&receivePacket, sizeof(receivePacket));
        copyFromReceiveData();
        return true;
    }
    Serial.println("Gap pattern read over I2C failed");
    return false;
}

// Function that requests the pattern selection number from the slave (CPU1). 
// Returns the pattern (0 = home, 1 to NUM_PATTERNS), or -1 if the transfer failed.
int readPattern() {
  int value = 0;

  // Tell CPU1 we want the pattern selection
  Wire2.beginTransmission(0x40);
  Wire2.write(I2C_CMD_PATTERN);
  Wire2.endTransmission();

  if (Wire2.requestFrom(0x40, sizeof(value))) {
    Wire2.readBytes((byte*)&value, sizeof(value));
    return value;
  }

  Serial.println("Pattern selection read over I2C failed");
  return -1;  
}
