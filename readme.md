# CCS Charging Experiments with ESP32

## Goal
This project intends to use the ESP32-based WT32-ETH01 board together
with a tpLink Homeplug modem containing an AR7420, to build the vehicle
side of a CCS charging system for electrical cars.

The basis of this port is the python-variant in
https://github.com/uhi22/pyPLC

## Benefits of this solution
- much faster startup time (2s) compared to the Raspberry or Notebook
- much less current consumption compared to the Raspberry or Notebook
- more compact design

## Drawbacks
- no log file for offline analysis

## Change history / functional status

### 2023-03-11 Reading the HomePlug software versions works
Initial Arduino sketch, which is able to send and receive HomePlug
messages on the Ethernet port of the WT-ETH01. Debug console is
the Arduino IDE, via the RX0/TX0, which are also used for programming.
As test, the sketch sends GetSoftwareVersion requests and shows
the related responses on the debug console.

## Todos
- porting of all required homeplug stuff from python to C.
- porting the SLAC state machine
- porting the Syslog message transmission
- porting the UDP and SDP
- integrating TCP
- integrating the EXI decoder/encoder
- porting the pev state machine