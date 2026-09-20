#include <Arduino.h>

#include "constants.h"
#include "functions_watchdog.h"

// Uses the 16 bit watchdog WDOG1. Its timeout is (WT + 1) x 0.5 s, and the counter is serviced by writing 0x5555 and then 0xAAAA to WSR.
// The watchdog cannot be stopped once it has been enabled.

static_assert(WATCHDOG_TIMEOUT_MS >= 500 && WATCHDOG_TIMEOUT_MS <= 128000 && WATCHDOG_TIMEOUT_MS % 500 == 0,
              "WATCHDOG_TIMEOUT_MS must be a multiple of 500 ms between 500 ms and 128 s");

bool watchdogCausedReset() {
  // WRSR keeps the source of the last watchdog related reset. TOUT = the watchdog timed out.
  return (WDOG1_WRSR & WDOG_WRSR_TOUT) != 0;
}

void watchdogStart() {
  // WDE = enable. SRS and WDA must be 1: writing 0 to them would assert a reset immediately.
  WDOG1_WCR = WDOG_WCR_WT(WATCHDOG_TIMEOUT_MS / 500 - 1) | WDOG_WCR_WDE | WDOG_WCR_SRS | WDOG_WCR_WDA;
  watchdogFeed();
}

void watchdogFeed() {
  WDOG1_WSR = 0x5555;
  WDOG1_WSR = 0xAAAA;
}
