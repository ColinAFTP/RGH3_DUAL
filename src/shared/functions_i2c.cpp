#include <i2c_driver.h>
#include <i2c_driver_wire.h>

#include "functions_i2c.h"
#include "variables.h"

// Copy values from each gap pattern to a single array to be used for I2C communications
void copyToTransmitData() {
  for (int r = 0; r < NUM_PATTERNS; r++) {
      for (int c = 0; c < NUM_GAPS; c++) {
          transmitData[r][c] = arrays[r][c];
      }
  }
}

// Copy values from a single array used for I2C communications to each gap pattern
void copyFromReceiveData() {
  for (int r = 0; r < NUM_PATTERNS; r++) {
      for (int c = 0; c < NUM_GAPS; c++) {
          arrays[r][c] = receiveData[r][c];
      }
  }
}

void onI2CRequest() {
    if (i2cCommand == 1) {
        // Send IO data
        Wire2.write((byte*)&inputData, sizeof(inputData));
    }
    else if (i2cCommand == 2) {
        // Send full gap pattern block
        copyToTransmitData();
        Wire2.write((byte*)&transmitData, sizeof(transmitData));
    }
    if (i2cCommand == 3) {
        // Send pattern selection
        Wire2.write((byte*)&patternSelection, sizeof(patternSelection));
    }
}

// // Function that executes whenever data is requested by master (CPU2).
// // This function is registered as an event, see setup() on CPU1.
// void writeGapPatterns() {
//   Serial.print("Sending ("); 
//   Serial.print(sizeof transmitData);
//   Serial.println(" bytes)");

//   // Example: fill arrays with test values
//   for (int r = 0; r < NUM_PATTERNS; r++) {
//       for (int c = 0; c < NUM_GAPS; c++) {
//           arrays[r][c] = (r * 10) + c;  // unique values
//       }
//   }

//   copyToTransmitData();

//   // // Print transmitData to verify
//   // for (int r = 0; r < NUM_PATTERNS; r++) {
//   //     Serial.print("Array ");
//   //     Serial.print(r);
//   //     Serial.print(": ");

//   //     for (int c = 0; c < NUM_GAPS; c++) {
//   //         Serial.print(transmitData[r][c]);
//   //         Serial.print(" ");
//   //     }
//   //     Serial.println();
//   // }

//   Wire2.write((byte*) &transmitData, sizeof transmitData);
// }

// Function that requests IO data from the slave (CPU1). 
// This function is called from CPU2.
int readIO() {
  int value = 0;

  // Tell CPU1 we want IO data (command = 1)
  Wire2.beginTransmission(0x40);
  Wire2.write(1);
  Wire2.endTransmission();

  Serial.print("Requesting ("); 
  Serial.print(sizeof value); 
  Serial.println(" bytes)... ");
  if (Wire2.requestFrom(0x40, sizeof(value))) {
    Wire2.readBytes((byte*)&value, sizeof(value));
    return value;
  }

  Serial.println("IO data read over I2C failed");
  return -1;  
}

// Function that requests pattern gap data from the slave (CPU1). 
// This function is called from CPU2.
void readGapPatterns() {
  // Tell CPU1 we want gap patterns (command = 2)
  Wire2.beginTransmission(0x40);
  Wire2.write(2);
  Wire2.endTransmission();

  Serial.print("Requesting ("); 
  Serial.print(sizeof receiveData); 
  Serial.println(" bytes)... ");
  if (Wire2.requestFrom(0x40, sizeof receiveData)) {
    Wire2.readBytes((byte*) &receiveData, sizeof receiveData);
    copyFromReceiveData();

    // // Print arrays to verify
    // for (int r = 0; r < NUM_PATTERNS; r++) {
    //     Serial.print("Array ");
    //     Serial.print(r);
    //     Serial.print(": ");

    //     for (int c = 0; c < NUM_GAPS; c++) {
    //         Serial.print(arrays[r][c]);
    //         Serial.print(" ");
    //     }
    //     Serial.println();
    // }

  } 
  else {
    Serial.println("Gap pattern read over I2C failed");
  }

}

// Function that requests pattern selection number from the slave (CPU1). 
// This function is called from CPU2.
int readPattern() {
  int value = 0;

  // Tell CPU1 we want the pattern selection (command = 3)
  Wire2.beginTransmission(0x40);
  Wire2.write(3);
  Wire2.endTransmission();

  Serial.print("Requesting ("); 
  Serial.print(sizeof value); 
  Serial.println(" bytes)... ");
  if (Wire2.requestFrom(0x40, sizeof(value))) {
    Wire2.readBytes((byte*)&value, sizeof(value));
    return value;
  }

  Serial.println("Pattern selection read over I2C failed");
  return -1;  
}
