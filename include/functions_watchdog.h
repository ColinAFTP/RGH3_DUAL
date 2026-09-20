#ifndef FUNCTIONS_WATCHDOG_H
#define FUNCTIONS_WATCHDOG_H

// Hardware watchdog (i.MX RT1062 WDOG1) for both CPUs. If watchdogFeed() is not called for WATCHDOG_TIMEOUT_MS the CPU resets itself,
// so a hung main loop can never leave the outputs frozen for good.

bool watchdogCausedReset();             // True if the last reset was a watchdog timeout. Call once, early in setup(), before watchdogStart()
void watchdogStart();                   // Start the watchdog. Call at the END of setup(), when every long wait is finished
void watchdogFeed();                    // Call at least every WATCHDOG_TIMEOUT_MS: once per pass of loop() and inside any long wait

#endif
