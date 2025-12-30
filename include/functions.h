//#include <Arduino.h>
#include <i2c_driver.h>
#include <i2c_driver_wire.h>
#include <variables.h>

// Copy values from each gap pattern to a single array to be used for I2C communications
void copyToTransmitData() {
    int index = 0;

    for (int r = 0; r < numPatterns; r++) {
        for (int c = 0; c < numGaps; c++) {
            transmitData[index++] = arrays[r][c];
        }
    }
}

// Copy values from a single array used for I2C communications to each gap pattern
void copyFromReceiveData() {
    int index = 0;

    for (int r = 0; r < numPatterns; r++) {
        for (int c = 0; c < numGaps; c++) {
            arrays[r][c] = receiveData[index++];
        }
    }
}

// function that executes whenever data is requested by master
// this function is registered as an event, see setup()
// Event
void writeGapPatterns() {
  Serial.print("Sending ("); 
  Serial.print(sizeof transmitData);
  Serial.println(" bytes)");

  // Example: fill arrays with test values
  for (int r = 0; r < numPatterns; r++) {
      for (int c = 0; c < numGaps; c++) {
          arrays[r][c] = (r * 10) + c;  // unique values
      }
  }

  copyToTransmitData();

  // Print transmitData to verify
  for (int r = 0; r < numPatterns; r++) {
      Serial.print("Array ");
      Serial.print(r);
      Serial.print(": ");

      for (int c = 0; c < numGaps; c++) {
          Serial.print(transmitData[r*numGaps+c]);
          Serial.print(" ");
      }
      Serial.println();
  }

  Wire2.write((byte*) &transmitData, sizeof transmitData);
}



void readGapPatterns() {
  Serial.print("Requesting ("); 
  Serial.print(sizeof receiveData); 
  Serial.println(" bytes)... ");
  if (Wire2.requestFrom(0x40, sizeof receiveData))
  {
    Wire2.readBytes((byte*) &receiveData, sizeof receiveData);

    copyFromReceiveData();

    // Print arrays to verify
    for (int r = 0; r < numPatterns; r++) {
        Serial.print("Array ");
        Serial.print(r);
        Serial.print(": ");

        for (int c = 0; c < numGaps; c++) {
            Serial.print(arrays[r][c]);
            Serial.print(" ");
        }
        Serial.println();
    }

  } 
  else {
    Serial.println("could not connect");
  }

}
