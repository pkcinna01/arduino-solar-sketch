# arduino-solar-sketch

## Synopsis

This application controls fans based on temperature sensors and fan on/off temp thresholds.  It also allows USB
queries so the state of the fans and temperature sensors can be monitored and displayed with tools such as Prometheus
and Grafana.  The USB interface lets monitoring alerts and external programs control the fans or let this Arduino
program control them.

Available commands:
`VERSION|GET|SET_FAN_MODE|SET_FAN_THRESHOLDS|SET_POWER_METER_VCC|SET_OUTPUT_FORMAT`

#### TODO
2. Provide Arduino circuit diagram   

## USB Command Examples

See [arduino-solar-client](https://github.com/pkcinna01/arduino-solar-client) for example of sendind data over USB.

List state of each device including the temp sensors and fans.  This can be used for monitoring and alerts.
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

## Motivation

Used to efficiently cool several solar charge controllers in an off grid system.


## Installation

Use Arduino IDE to upload the sketch onto an Arduino UNO.


