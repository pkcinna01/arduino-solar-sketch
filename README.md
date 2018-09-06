# arduino-solar-sketch

## Synopsis

This application controls fans and high power relays.  It supports USB
queries so the power usage and state of devices can be monitored and displayed with tools such as Prometheus
and Grafana.  The USB interface lets monitoring systems and external programs override fan behavior. 

This is a rewrite which is not compatible with Arduino UNO.  A MEGA2560 Arduino board is now used which allows more
memory intesive libraries and code like ArduinoSTL.  The available commands will change to reflect new device managment.

Available commands:
`VERSION|GET|SET_FAN_MODE|SET_FAN_THRESHOLDS|SET_POWER_METER_VCC|SET_OUTPUT_FORMAT`

#### TODO
1. Provide Arduino circuit diagram   

## USB Command Examples

See [arduino-solar-client](https://github.com/pkcinna01/arduino-solar-client) for an example of sendind data over USB.

Get the state of each device and power meter.
```
GET
```

Turn off all fans, then turn on all fans, then resume automatic control of fans.  Use PERSIST instead of TRANSIENT
if Arduino is to remember fan mode after reboot.
```
SET_FAN_MODE,ON,TRANSIENT
SET_FAN_MODE,OFF,TRANSIENT
SET_FAN_MODE,AUTO,TRANSIENT
```

Change the temperature thresholds for the fans... first find device named "Bench" and update all fans (wildcard) to turn on when
temp exceeds 90 degrees fahrenheit and off when temp goes below 85.
```
SET_FAN_THRESHOLDS,Bench,*,90,85,PERSIST
```

Calibrate the voltage used to measure device voltage and power consumption.  The second argument is the filter 
on the power meter name and the third argument is the measured voltage of the Arduino or power source used.
```
SET_POWER_METER_VCC,*,4.89
```
 
## Motivation

Used to efficiently cool several solar charge controllers in an off grid system.


## Installation

Use Arduino IDE to upload the sketch onto an Arduino UNO.


