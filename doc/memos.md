
# Hardware

WT32-ETH01: This is an ESP32 based board with an LAN8720A Ethernet port.

Decsription: http://www.wireless-tag.com/portfolio/wt32-eth01/

Data sheet: http://www.wireless-tag.com/wp-content/uploads/2022/10/WT32-ETH01_datasheet_V1.3-en.pdf


# Development environment

- Arduino IDE, revision 2.0.4
- Selected board: "WT32-ETH01 Ethernet Module"
- The board does not have an internal LED (besides the TX and RX and power). That's why we connect an LED on IO2.
- Programming via TX0 and RX0.
- To enter bootloader, set IO0 to ground while releasing the reset (="EN") pin.

# How to send and receive raw ethernet frames?

Where are the libraries stored?
C:\Users\uwemi\AppData\Local\Arduino15\packages\esp32\hardware\esp32\2.0.7\libraries

Ethernet example:
C:\Users\uwemi\AppData\Local\Arduino15\packages\esp32\hardware\esp32\2.0.7\libraries\Ethernet\examples\ETH_LAN8720
But this is not pure ethernet. It is a TCP client example via Ethernet. Conclusion: We do not want this.

Approach for making an own ethernet handler software: Pick the interesting parts of the libraries, and use the low-level-interfaces from esp_eth.h to talk to the ethernet driver.

- ESP_IDF_VERSION_MAJOR > 3 is true.

Where is the function esp_eth_phy_new_lan87xx() implemented?
In the lib C:\Users\uwemi\AppData\Local\Arduino15\packages\esp32\hardware\esp32\2.0.7\tools\sdk\esp32\lib\libesp_eth.a. No source code found for this in the arduino installation. But it is on Github, and in the
Espressif IDF, see below.

How to configure the ethernet to call a callback function like in WiFiGeneric.cpp _arduino_event_cb? Answer: By calling esp_event_handler_instance_register().

How will the received data arrive in the application? Answer: Via a receive callback, which
needs to be registers with esp_eth_update_input_path().

How can the application transmit an Ethernet frame? Answer: By calling esp_eth_transmit() and giving the buffer and length as parameters.

Why does the board not receive all frames of the Ethernet? Answer: There seems to be a filter,
which makes sure, that only messages are forwarded to the application, which are either
broadcasts, or they match the own MAC. The own MAC is the one which is available via
esp_eth_ioctl(eth_handle, ETH_CMD_G_MAC_ADDR, myMAC). Maybe there is the possibility to 
receive all frames, when we switch to promiscuous mode by esp_eth_ioctl(..., ETH_CMD_S_PROMISCUOUS, ...), this is not yet tested.

## Ethernet issues

### emac_esp32_transmit(229): insufficient TX buffer size.
This happens sporadically even if only transmitted 60 bytes SlacParam.Req. Strange.

### esp.emac: no mem for receive buffer
E (23219) esp.emac: no mem for receive buffer
E (23433) esp.emac: no mem for receive buffer
This stops the charging process, because no response from Charger is seen anymore.

Also discussed in
https://github.com/cmbahadir/opcua-esp32/issues/42
and
https://github.com/espressif/esp-idf/issues/9594
and
https://github.com/espressif/esp-idf/issues/9308
Maybe the fix https://github.com/espressif/esp-idf/commit/785f154f5652b353c460ec47cb90aa404207918c needs to be introduced.
Also https://github.com/espressif/esp-idf/commit/6fff81d9700c6f615b711f7581329eea3566f32a contains receive-buffer-improvements.
Need to check which version is used in the Arduino package.

### Where is the esp_eth_driver code?
On https://github.com/espressif/esp-idf/ there is in components/esp_eth/src/esp_eth.c an implementation of
e.g. esp_eth_driver_install() and esp_eth_transmit(). So far so good. But esp_eth_phy_new_lan8720() is not available there.
At least we have esp_eth_phy_new_lan87xx(), and this is mentioned in `C:\Users\uwemi\AppData\Local\Arduino15\packages\esp32\hardware\esp32\2.0.7\tools\sdk\esp32\include\esp_eth\include\esp_eth_phy.h`
as a backward-compatibility-name for esp_eth_phy_new_lan87xx().

After installing the Espressif IDF into C:\esp-idf, the ethernet driver is also here:
C:\esp-idf\components\esp_eth\
And in this, we find `emac_esp32_rx_task()`, which raises `ESP_LOGE(TAG, "no mem for receive buffer")`

After building the helloworld example according to
https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/windows-setup.html#get-started-windows-first-steps,
`C:\UwesTechnik\espExamples\hello_world>idf.py build`
we find a new built lib here:
`C:\UwesTechnik\espExamples\hello_world\build\esp-idf\esp_eth\libesp_eth.a`

Is it possible to simply copy this into the Arduino path
`C:\Users\uwemi\AppData\Local\Arduino15\packages\esp32\hardware\esp32\2.0.7\tools\sdk\esp32\lib`?

In the Arduino IDE, this leads to multiple linker error due to missing symbols, e.g. undefined reference to emac_hal_transmit_multiple_buf_frame. Means: The libs in the Arduino path and the latest IDF libs are not compatible.

The solution may be here: https://github.com/espressif/arduino-esp32/releases It says, that the esp-arduino2.0.7 (which we use), is based on ESP-IDF 4.4.4. This is older than the IDF 5.0 which we installed for testing.

Installed the IDF4.4.4 with the original windows installer.
Added some debug output into C:\esp-idf-v4.4.4\components\esp_eth\src\esp_eth_mac_esp.c, function emac_esp32_rx_task().
Built the example `C:\UwesTechnik\espExamples\hello_world>idf.py build`
Copied the newly built libesp_eth.a from C:\Users\uwemi\AppData\Local\Arduino15\packages\esp32\hardware\esp32\2.0.7\tools\sdk\esp32\lib
into `C:\Users\uwemi\AppData\Local\Arduino15\packages\esp32\hardware\esp32\2.0.7\tools\sdk\esp32\lib`.
In Arduino IDE, built, download. Result: The build works, the patch is visible. --> Possibility to debug and fix the ethernet issue.

### How to measure free heap space?
Serial.println(ESP.getFreeHeap())

### How does the eth driver work?
* lan87xx_init() -> esp_eth_phy_802_3_basic_phy_init(). This makes power on (eth_phy_802_3_pwrctl()) and reset (eth_phy_802_3_reset())


# Logging

How to print an info to serial line inside a library, e.g. in ETH.c?
log_printf(ARDUHAL_LOG_FORMAT(I, "This is also an info."));
shows a log information including the time since boot, the file name, line number, function name:
[    11][I][ETH.cpp:325] begin(): This is also an info.

# Time measurement
How to know the time since startup? esp_timer_get_time() Get time in microseconds since boot

# Project structuring / modularization

## Multiple .ino Files
It is possible to have multiple .ino files in one sketch. The IDE copies all .ino files into one single .cpp file
before compiling. The first .ino is the "main" .ino, which has the same name as the sketch folder. The other .ino
are following in alphabetical order. This means: No #include is necessary, because from compilers point of view all .ino
are just one single .cpp in the end.

## Subfolders
The Arduino IDE will compile and link also files which are placed in the sketch directory in a folder "src", also
recursively.
See https://forum.arduino.cc/t/subfolders-in-sketch-folder/564852

# Protocol Investigations

## Neighbor Discovery

When trying to TCP-connect to the Win10 notebook without Neighbor Discovery before, the notebook sends NeighSol, and does
not response on the TCP. It looks like, that the Neighbor Discovery is a precondition for TCP.
https://en.wikipedia.org/wiki/Neighbor_Discovery_Protocol

How the Neighbor Discovery works with other combinations of chargers?

         PEV Win10       EVSE SuperCharger:
  98.41s   --> SDP.Req --> 
  98.42s   <-- NeighSol <--
  98.42s   --> NeighAdv -->
  98.43s   <-- SDP.Res <-- 
  
         PEV Win10       EVSE alpitronics:
  24.93s   --> SDP.Req --> 
  24.94s   <-- NeighSol <--
  24.94s   --> NeighAdv -->
  24.94s   <-- SDP.Res <-- 
  
         PEV WT32       EVSE Win10
           --> SDP.Req --> 
           <-- SDP.Res <-- 
           --> TCP.SYN --> 
           <-- NeighSol <--
           <-- NeighSol <--
           <-- NeighSol <--		   
           (reaction NeighAdv not yet implemented)
		 
## TCP Details

http://tcpipguide.com/free/t_TCPOperationalOverviewandtheTCPFiniteStateMachineF-2.htm
https://en.wikipedia.org/wiki/Transmission_Control_Protocol

STATE_CLOSED: Active Open, Send SYN: A client begins connection setup by sending a SYN message, and also sets up a TCB for this connection. It then transitions to the SYN-SENT state.

STATE_SYN_SENT: Receive SYN+ACK, Send ACK: If the device that sent the SYN receives both an acknowledgment to its SYN and also a SYN from the other device, it acknowledges the SYN received and then moves straight to the ESTABLISHED state.

First message:
 - SYN
 - SEQNR=A
 - ACKNR=0
 
Second message:
 - SYN, ACK
 - SEQNR=B
 - ACKNR=A+1

Third message:
 - ACK
 - SEQNR = last received ACKNR = A+1
 - ACKNR = last received SEQNR = B+1

Fourth message (which is the first data message):
 - PSH, ACK
 - SEQNR = last received ACKNR = A+1
 - ACKNR = last received SEQNR = B+1 
 - data 

Fifth message (which is the confirmation of the first data message):
 - ACK
 - SEQNR = last received ACKNR = A+1
 - ACKNR =  last received SEQNR + lengthOfData
 
