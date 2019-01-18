# arduino-solar-sketch

## Synopsis

This application controls fans and high power relays.  It supports USB
queries so the power usage and state of devices can be monitored and displayed with tools such as Prometheus
and Grafana.  The USB interface lets monitoring systems and external programs override fan and power switch behaviors.

Not comaptable with Arduino UNO.  An ATMEGA2560 board is used due to larger program size and ArduinoSTL dependency.

Save or "PERSIST" functionality changed significantly.  The new implementation does not save explicit values.  Instead it stores
a list of SETUP commands.  These commands are run on startup or RESET.


Available commands:
`GET|INCLUDE|EXCLUDE|SET|SETUP|RESET|VERBOSE`

#### TODO
1. Provide Arduino circuit diagram   

## USB Command Examples

See [arduino-solar-client](https://github.com/pkcinna01/arduino-solar-client) for an example of sendind data over USB.

Get the state of each device and power meter.
```
get,devices
```

Turn off all fans, then turn on all fans, then resume automatic control of fans.
```
set,device,*,constraint/mode,pass
set,device,*,constraint/mode,fail
set,device,*,constraint/mode,test
```

Change the temperature thresholds for the fans... first find device named "Bench" and update all fans (wildcard) to turn on when
temp exceeds 90 degrees fahrenheit and off when temp goes below 85.
```
TODO
```

Calibrate the voltage used to measure device voltage and power consumption.  The second argument is the filter 
on the power meter name and the third argument is the measured voltage of the Arduino or power source used.
```
TODO
```

## Installation

Use Arduino IDE to upload the sketch onto an Arduino UNO.


