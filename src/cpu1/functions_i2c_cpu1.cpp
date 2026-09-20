#include <i2c_driver.h>
#include <i2c_driver_wire.h>

#include "functions_i2c.h"
#include "variables_cpu1.h"

// I2C functions of CPU1, which is the slave (address 0x40). CPU2 is the master.

// Copy values from each gap pattern to a single array to be used for I2C communications
void copyToTransmitData() {
    for (int r = 0; r < NUM_PATTERNS; r++) {
        for (int c = 0; c < STRIDE_GAPS; c++) {
            transmitPacket.patternData[r][c] = gapArrays[r][c];
        }
    }

    transmitPacket.stepperSpeed = stepperSpeed;
}

// Called (in interrupt context) when CPU2 requests data. The answer depends on the last command received.
void onI2CRequest() {
    if (i2cCommand == I2C_CMD_IO) {
        // Send IO data
        Wire2.write((byte*)&inputData, sizeof(inputData));
    }
    else if (i2cCommand == I2C_CMD_GAPS) {
        // Send full gap pattern block
        copyToTransmitData();
        Wire2.write((byte*)&transmitPacket, sizeof(transmitPacket));
    }
    else if (i2cCommand == I2C_CMD_MANUAL) {
        // The manual command: mode switches, jog coils, spreader, and the inputs the push-along logic needs
        ManualCommand mc;
        mc.flags = manualFlagsShared;
        mc.spreader = manualSpreaderShared;
        mc.inputs = inputData;
        Wire2.write((byte*)&mc, sizeof(mc));
    }
    else if (i2cCommand == I2C_CMD_PATTERN) {
        // Send pattern selection
        Wire2.write((byte*)&patternSelection, sizeof(patternSelection));
    }
}
