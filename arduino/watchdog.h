#ifndef _ARDUINO_WATCHDOG_H
#define _ARDUINO_WATCHDOG_H

#include <avr/wdt.h>
namespace arduino
{
namespace watchdog
{
const auto KeepAliveMs = WDTO_8S;
bool resetRequested = false;

void enable()
{
    wdt_enable(KeepAliveMs);
}

void keepAlive()
{
    while (resetRequested); // this will lock program and prevent wdt_reset which will cause reboot
    wdt_reset();
}
} // namespace watchdog
} // namespace arduino

#endif