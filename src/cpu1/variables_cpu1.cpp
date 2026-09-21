#include <Arduino.h>

#include "variables_cpu1.h"

// Variables used for the IO signals
FastShiftIn* FSI = nullptr;
uint16_t inputData = 0;
uint16_t inputDataRaw = 0;
uint16_t inputDataPrevious = 0;

// Variables used for comms
EthernetServer ethernetServer(502);
EthernetClient ethernetClient;
ModbusTCPServer modbusServer;
int patternSelection = 0;
int patternSelectionPrevious = 0;
word relayData = 0;
word relayDataPrevious = 0;
word relayOutputShown = 0;
bool relayTestActive = false;
int speedData = 0;
int speedDataPrevious = 0;
uint32_t lastModbusRequestMs = 0;
char plcRemoteText[24] = "";
volatile uint8_t manualFlagsShared = 0;
volatile uint8_t manualSpreaderShared = 0;
uint32_t tickerTime = 0;
uint16_t secondTicker = 0;
bool bootLoadGaps = false;

// Status calculated by feedbackCheck()
bool statusHome = false;
bool statusAtTarget = false;
bool statusFault = false;

// Status packets received from CPU2
StatusPacket statusRing[STATUS_RING_SIZE];
volatile uint8_t statusRingHead = 0;
volatile uint8_t statusRingTail = 0;

bool statusRefused = false;
uint8_t statusRefusedReason = 0;
uint8_t faultTypeShown = 0;
bool cpu1Refused = false;
uint8_t cpu1RefusedReason = 0;
bool atTargetBlank = false;
uint32_t atTargetBlankStart = 0;
