# CCS Charging Experiments with ESP32

## Goal
This project intends to use the ESP32-based WT32-ETH01 board together
with a tpLink Homeplug modem containing an AR7420, to build the vehicle
side of a CCS charging system for electrical cars.

The basis of this port is the python-variant in
https://github.com/uhi22/pyPLC

## News / Change history / functional status

### 2023-04-05 Charging loop on Alpitronics works
- After fixing several topics regarding the NeighborSolicitation, the SDP succeeds, and we come to the charging loop on alpitronics hypercharger.

### 2023-03-25 LC Display works
- The three-line-OLED display (see https://github.com/uhi22/SerialToOLED) shows the charging progress.
- Simulated charging works with simulated EVSE (Win10, pyPlc).
- The main crash reason was a heap leakage because receive buffer was not free'd. This is fixed.
- Sporadically still crashes: `panic'ed (Unhandled debug exception). Debug exception reason: Stack canary watchpoint triggered (emac_rx)`

### 2023-03-19 PEV state machine until Charging Loop
With simulated EVSE (Win10, pyPlc) on the opposite end, the WT32-ETH01 makes the SLAC, SDP, NeighborDiscovery, TCP
and the charging state machine. It runs until the charging loop.
Unfortunately, the controller is not really stable. Several ethernet issues are
visible:
- Sometimes the eth raises `ETHERNET_EVENT_DISCONNECTED`.
- Sporadically we get `emac_esp32_transmit(229): insufficient TX buffer size`
- Often `esp.emac: no mem for receive buffer`
- Sporadically a complete crash: `panic'ed (Unhandled debug exception). Debug exception reason: Stack canary watchpoint triggered (emac_rx)`

### 2023-03-11 Reading the HomePlug software versions works
Initial Arduino sketch, which is able to send and receive HomePlug
messages on the Ethernet port of the WT-ETH01. Debug console is
the Arduino IDE, via the RX0/TX0, which are also used for programming.
As test, the sketch sends GetSoftwareVersion requests and shows
the related responses on the debug console.


## Benefits of this solution
- much faster startup time (2s) compared to the Raspberry or Notebook
- much less current consumption compared to the Raspberry or Notebook
- more compact design

## Drawbacks
- no log file for offline analysis

## Todos
[x] porting of all required homeplug stuff from python to C.
[x] porting the SLAC state machine
[ ] porting the Syslog message transmission
[x] porting the UDP and SDP
[x] integrating TCP
[x] integrating the EXI decoder/encoder
[x] porting the pev state machine