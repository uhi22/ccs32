
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
In the lib libesp_eth.a. No source code found for this.

How to configure the ethernet to call a callback function like in WiFiGeneric.cpp _arduino_event_cb? Answer: By calling esp_event_handler_instance_register().

How will the received data arrive in the application? Answer: Via a receive callback, which
needs to be registers with esp_eth_update_input_path().

How can the application transmit an Ethernet frame? Answer: By calling esp_eth_transmit() and giving the buffer and length as parameters.

Why does the board not receive all frames of the Ethernet? Answer: There seems to be a filter,
which makes sure, that only messages are forwarded to the application, which are either
broadcasts, or they match the own MAC. The own MAC is the one which is available via
esp_eth_ioctl(eth_handle, ETH_CMD_G_MAC_ADDR, myMAC). Maybe there is the possibility to 
receive all frames, when we switch to promiscuous mode by esp_eth_ioctl(..., ETH_CMD_S_PROMISCUOUS, ...), this is not yet tested.

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

