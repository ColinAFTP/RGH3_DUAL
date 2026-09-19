#ifndef FUNCTIONS_COMMS_H
#define FUNCTIONS_COMMS_H

void modbusSetup();                     // This function sets up all the Modbus coils, registers, inputs, etc.
void ethernetConnect();                 // This function handles Ethernet client connections
void patternCheck();                    // This function checks which pattern is selected from the PLC
void relayCheck();                      // This function checks for relay control updates from the PLC
void updateTicker(word tickerData);     // This function updates the ticker value in the holding register
void speedCheck();                      // This function checks for speed data updates from the PLC 
void updateInputs();                    // This function updates the input data in the holding register
void patternUpdateCheck();              // This functions checks for pattern updates from the PLC

#endif
