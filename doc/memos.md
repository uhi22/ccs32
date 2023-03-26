
# Hardware

WT32-ETH01: This is an ESP32 based board with an LAN8720A Ethernet port.

Description: http://www.wireless-tag.com/portfolio/wt32-eth01/

Data sheet: http://www.wireless-tag.com/wp-content/uploads/2022/10/WT32-ETH01_datasheet_V1.3-en.pdf

Pins:
TXD for display is IO17 according to the data sheet of the WT32-ETH01.
RXD (unused) is IO5 according to the data sheet of the WT32-ETH01.

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
Copied the newly built libesp_eth.a from C:\UwesTechnik\espExamples\hello_world\build\esp-idf\esp_eth\libesp_eth.a
into `C:\Users\uwemi\AppData\Local\Arduino15\packages\esp32\hardware\esp32\2.0.7\tools\sdk\esp32\lib`.
In Arduino IDE, built, download. Result: The build works, the patch is visible. --> Possibility to debug and fix the ethernet issue.

Finding: The emac_esp32_rx_task() allocates for each received ethernet package a buffer. It fills this buffer by calling emac_esp32_receive(),
and if this works, gives the buffer to emac->eth->stack_input(). It is not clear, which instance is responsible for freeing this buffer afterwards.
Instrumented the code, and using ESP.getFreeHeap() found out, that each call of the emac_esp32_rx_task() eats ~1500 bytes of heap, without
freeing them.

What does stack_input() do with the buffer?
(Searching in C:\esp-idf-v4.4.4\components)
There is a function eth_stack_input() in esp_eth.c, which calls eth_driver->stack_input if its not NULL. Otherwise frees the buffer -> ok.
The eth_driver->stack_input is set in esp_eth_update_input_path() just as the second parameter. Means: That's the handler of the application.
Conclusion: The receive handler of the application is responsible to free the buffer.

After adding the free(buffer) in the applications rx handler, the issue with the eated heap is solved.

But still, there is
```
    [292360][V][ccs32.ino:49] addToTrace(): [PEVSLAC] received GET_SW.CNF
    Guru Meditation Error: Core  1 panic'ed (Unhandled debug exception). 
    Debug exception reason: Stack canary watchpoint triggered (emac_rx) 
    Core  1 register dump:
    PC      : 0x4008d0db  PS      : 0x00060036  A0      : 0x8008bb48  A1      : 0x3ffb4440  
    A2      : 0x3ffbf338  A3      : 0xb33fffff  A4      : 0x0000cdcd  A5      : 0x00060023  
    A6      : 0x00060023  A7      : 0x0000abab  A8      : 0xb33fffff  A9      : 0xffffffff  
    A10     : 0x00060020  A11     : 0x00000001  A12     : 0x800844b6  A13     : 0x3ffbf28c  
    A14     : 0x007bf338  A15     : 0x003fffff  SAR     : 0x00000004  EXCCAUSE: 0x00000001  
    EXCVADDR: 0x00000000  LBEG    : 0x40088549  LEND    : 0x40088559  LCOUNT  : 0xfffffffd  
    
    Backtrace: 0x4008d0d8:0x3ffb4440 0x4008bb45:0x3ffb4480 0x4008a400:0x3ffb44b0 0x4008a3b0:0xa5a5a5a5 |<-CORRUPTED
```
or
```
    [217353][V][ccs32.ino:49] addToTrace(): [ModemFinder] Starting modem search
    Guru Meditation Error: Core  1 panic'ed (Unhandled debug exception). 
    Debug exception reason: Stack canary watchpoint triggered (emac_rx) 
    Core  1 register dump:
    PC      : 0x4008d0db  PS      : 0x00060036  A0      : 0x8008bb48  A1      : 0x3ffb4440  
    A2      : 0x3ffbf338  A3      : 0xb33fffff  A4      : 0x0000cdcd  A5      : 0x00060023  
    A6      : 0x00060023  A7      : 0x0000abab  A8      : 0xb33fffff  A9      : 0xffffffff  
    A10     : 0x00000003  A11     : 0x00060023  A12     : 0x800844b6  A13     : 0x3ffbf28c  
    A14     : 0x007bf338  A15     : 0x003fffff  SAR     : 0x00000004  EXCCAUSE: 0x00000001  
    EXCVADDR: 0x00000000  LBEG    : 0x40088549  LEND    : 0x40088559  LCOUNT  : 0xfffffffd  
    
    Backtrace: 0x4008d0d8:0x3ffb4440 0x4008bb45:0x3ffb4480 0x4008a400:0x3ffb44b0 0x4008a3b0:0xa5a5a5a5 |<-CORRUPTED
```

Similar issue is discussed here: https://esp32.com/viewtopic.php?t=26603
After increasing the stack size mac_config.rx_task_stack_size = 4096, it seems to be more stable, but sporadically we get

```
    [566293][V][ccs32.ino:54] addToTrace(): (15 bytes)= 80 9a 2 0 40 80 c1 1 41 81 c2 10 c0 0 0 
    Guru Meditation Error: Core  1 panic'ed (LoadProhibited). Exception was unhandled.
    Core  1 register dump:
    PC      : 0x4012e9b4  PS      : 0x00060a30  A0      : 0x800ee0c8  A1      : 0x3ffb2090  
    A2      : 0x3ffc37a4  A3      : 0x00000000  A4      : 0x00000022  A5      : 0x00000001  
    A6      : 0x00000022  A7      : 0x3ffaf0a0  A8      : 0x00000000  A9      : 0x3ffb2070  
    A10     : 0x3ffc37a4  A11     : 0x3ffaf0e0  A12     : 0x00000022  A13     : 0x3ffaf0c3  
    A14     : 0x00000000  A15     : 0x00000000  SAR     : 0x00000019  EXCCAUSE: 0x0000001c  
    EXCVADDR: 0x0000000c  LBEG    : 0x40088098  LEND    : 0x400880a3  LCOUNT  : 0x00000000  
    Backtrace: 0x4012e9b1:0x3ffb2090 0x400ee0c5:0x3ffb20b0 0x400d2f22:0x3ffb20d0 0x400d3105:0x3ffb2120 0x400d7acd:0x3ffb21b0 0x400d7eb2:0x3ffb2210 0x400d7f42:0x3ffb2230 0x400d7f66:0x3ffb2250 0x400d7f8e:0x3ffb2270 0x400ef039:0x3ffb2290
```
and again
```
    Guru Meditation Error: Core  1 panic'ed (LoadProhibited). Exception was unhandled.
    Core  1 register dump:
    PC      : 0x4012e9b4  PS      : 0x00060230  A0      : 0x800ee0c8  A1      : 0x3ffb20d0  
    A2      : 0x3ffc37a4  A3      : 0x00000000  A4      : 0x00000021  A5      : 0x00000001  
    A6      : 0x00000021  A7      : 0x3ffaf040  A8      : 0x00000000  A9      : 0x3ffb20b0  
    A10     : 0x3ffc37a4  A11     : 0x3ffaf080  A12     : 0x00000021  A13     : 0x3ffaf062  
    A14     : 0x0000000a  A15     : 0x00000000  SAR     : 0x00000019  EXCCAUSE: 0x0000001c  
    EXCVADDR: 0x0000000c  LBEG    : 0x40088098  LEND    : 0x400880a3  LCOUNT  : 0x00000000  
    Backtrace: 0x4012e9b1:0x3ffb20d0 0x400ee0c5:0x3ffb20f0 0x400d2f22:0x3ffb2110 0x400d3105:0x3ffb2160 0x400d3247:0x3ffb21f0 0x400d7f69:0x3ffb2250 0x400d7f8e:0x3ffb2270 0x400ef039:0x3ffb2290
```
in the case when we disconnect and reconnect the Ethernet on Laptop side.

And
```
    [482970][V][ccs32.ino:54] addToTrace(): [CONNMGR] 0 0 0 0 150 151  --> 100
    [483270][V][ccs32.ino:54] addToTrace(): [PEVSLAC] from 48 entering 0
    Guru Meditation Error: Core  1 panic'ed (LoadProhibited). Exception was unhandled.
    Core  1 register dump:
    PC      : 0x4012e9b4  PS      : 0x00060c30  A0      : 0x800ee0c8  A1      : 0x3ffb20d0  
    A2      : 0x3ffc37a4  A3      : 0x00000000  A4      : 0x0000001e  A5      : 0x00000001  
    A6      : 0x0000001e  A7      : 0x3ffaf040  A8      : 0x00000000  A9      : 0x3ffb20b0  
    A10     : 0x3ffc37a4  A11     : 0x3ffaf0a0  A12     : 0x0000001e  A13     : 0x3ffaf05f  
    A14     : 0x00000000  A15     : 0x303a3830  SAR     : 0x0000001d  EXCCAUSE: 0x0000001c  
    EXCVADDR: 0x0000000c  LBEG    : 0x40088098  LEND    : 0x400880a3  LCOUNT  : 0x00000000  
    Backtrace: 0x4012e9b1:0x3ffb20d0 0x400ee0c5:0x3ffb20f0 0x400d2f22:0x3ffb2110 0x400d3105:0x3ffb2160 0x400d3247:0x3ffb21f0 0x400d7f69:0x3ffb2250 0x400d7f8e:0x3ffb2270 0x400ef039:0x3ffb2290
```
in case we unpower and repower the chargers modem multiple times.



## Is it possible to extract the function names out of a backtrace?
From https://esp32.com/viewtopic.php?t=23567:
xtensa-esp32-elf-addr2line -e path_to_your.elf 0x400d144c:0x3ffb1e60 0x400d6a1d:0x3ffb1fb0 0x4008b1a6:0x3ffb1fd0

xtensa-esp32-elf-addr2line -e  C:\UwesTechnik\espExamples\hello_world\build\hello_world.elf 0x400d4b68:0x3ffb1e60

Where is the elf for the Arduino? Here:
C:\Users\uwemi\AppData\Local\Temp\arduino\sketches\EBDAD9BC378D70AF6CFE030E00280F82\ccs32.ino.elf

So our complete command line is:
xtensa-esp32-elf-addr2line -e C:\Users\uwemi\AppData\Local\Temp\arduino\sketches\EBDAD9BC378D70AF6CFE030E00280F82\ccs32.ino.elf 0x4008d0d8:0x3ffb4440 0x4008bb45:0x3ffb4480 0x4008a400:0x3ffb44b0 0x4008a3b0:0xa5a5a5a5

And this leads to

```
    C:\esp-idf-v4.4.4>xtensa-esp32-elf-addr2line -e C:\Users\uwemi\AppData\Local\Temp\arduino\sketches\EBDAD9BC378D70AF6CFE030E00280F82\ccs32.ino.elf 0x4008d0d8:0x3ffb4440 0x4008bb45:0x3ffb4480 0x4008a400:0x3ffb44b0 0x4008a3b0:0xa5a5a5a5
    /Users/ficeto/Desktop/ESP32/ESP32S2/esp-idf-public/components/esp_hw_support/include/soc/compare_set.h:25
    /Users/ficeto/Desktop/ESP32/ESP32S2/esp-idf-public/components/freertos/port/xtensa/include/freertos/portmacro.h:578
    /Users/ficeto/Desktop/ESP32/ESP32S2/esp-idf-public/components/freertos/port/xtensa/portasm.S:436
    /Users/ficeto/Desktop/ESP32/ESP32S2/esp-idf-public/components/freertos/port/xtensa/portasm.S:231
```



## Is it possible to share global variables between the ethernet driver and the application?
Yes. Simply in the driver define them, and in the application declare them as external. Works fine.
 

### How to measure free heap space?
Serial.println(ESP.getFreeHeap())

### How does the eth driver work?
* lan87xx_init() -> esp_eth_phy_802_3_basic_phy_init(). This makes power on (eth_phy_802_3_pwrctl()) and reset (eth_phy_802_3_reset())
* emac_hal_start() enables the DMA for rx and tx: emac_ll_start_stop_dma_receive() and emac_ll_start_stop_dma_transmit()

* esp_emac_alloc_driver_obj() creates a task xTaskCreatePinnedToCore(emac_esp32_rx_task, "emac_rx",....)
* The emac_esp32_rx_task() calls emac_esp32_receive()
* emac_esp32_receive() calls emac_hal_receive_frame()
* emac_hal_receive_frame() (in components/hal/emac_hal.c) uses the data which was filled by DMA and fills it into buffer.


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
 
