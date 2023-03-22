/* CCS Charging with WT32-ETH01 and HomePlug modem */
/* This is the main Arduino file of the project. */
/* Developed in Arduino IDE 2.0.4 */

/* Modularization concept:
- The ccs32.ino is the main Arduino file of the project.
- Some other .ino files are present, and the Arduino IDE will merge all the .ino into a
  single cpp file before compiling. That's why, all the .ino share the same global context.
- Some other files are "hidden" in the src folder. These are not shown in the Arduino IDE,
  but the Arduino IDE "knows" them and will compile and link them. You may want to use
  an other editor (e.g. Notepad++) for editing them.
- We define "global variables" as the data sharing concept. The two header files "ccs32_globals.h"
  and "projectExiConnector.h" declare the public data and public functions.
- Using a mix of cpp and c modules works; the only requirement is to use the special "extern C" syntax in
  the header files.
*/

#include "ccs32_globals.h"
#include "src/exi/projectExiConnector.h"

/**********************************************************/

#define LED 2 /* The IO2 is used for an LED. This LED is externally added to the WT32-ETH01 board. */
uint32_t currentTime;
uint32_t lastTime1s;
uint32_t lastTime30ms;
uint32_t nCycles30ms;
uint8_t ledState;

/**********************************************************/
/* The logging macros and functions */
#undef log_v
#undef log_e
#define log_v(format, ...) log_printf(ARDUHAL_LOG_FORMAT(V, format), ##__VA_ARGS__)
#define log_e(format, ...) log_printf(ARDUHAL_LOG_FORMAT(E, format), ##__VA_ARGS__)

void addToTrace_chararray(char *s) {
  log_v("%s", s);  
}

void addToTrace(String strTrace) {
  //Serial.println(strTrace);  
  log_v("%s", strTrace.c_str());  
}

/**********************************************************/
/* stubs for later implementation */
#define publishStatus(x)
#define showStatus(x, y)

/**********************************************************/
/* The tasks */

/* This task runs each 30ms. */
void task30ms(void) {
  nCycles30ms++;
  connMgr_Mainfunction(); /* ConnectionManager */
  modemFinder_Mainfunction();
  runSlacSequencer();
  runSdpStateMachine();
  tcp_Mainfunction();
  pevStateMachine_Mainfunction();
}

/* This task runs once a second. */
void task1s(void) {
  if (ledState==0) {
    digitalWrite(LED,HIGH);
    //Serial.println("LED on");
    ledState = 1;
  } else {
    digitalWrite(LED,LOW);
    //Serial.println("LED off");
    ledState = 0;
  }
  //log_v("nTotalEthReceiveBytes=%ld, nCycles30ms=%ld", nTotalEthReceiveBytes, nCycles30ms);
  //log_v("nTotalEthReceiveBytes=%ld, nMaxInMyEthernetReceiveCallback=%d, nTcpPacketsReceived=%d", nTotalEthReceiveBytes, nMaxInMyEthernetReceiveCallback, nTcpPacketsReceived);
  //log_v("nTotalTransmittedBytes=%ld", nTotalTransmittedBytes);
  //pev_testExiSend(); /* just for testing, send some EXI data. */
  //tcp_testSendData(); /* just for testing, send something with TCP. */
  //sendTestFrame(); /* just for testing, send something on the Ethernet. */
  Serial.println(ESP.getFreeHeap());
}

/**********************************************************/
/* The Arduino standard entry points */

void setup() {
  // Set pin mode
  pinMode(LED,OUTPUT);
  Serial.begin(115200);
  Serial.println("CCS32 Started.");
  if (!initEth()) {
    log_v("Error: Ethernet init failed.");
  }
  homeplugInit();
  pevStateMachine_Init();
  /* The time for the tasks starts here. */
  currentTime = millis();
  lastTime30ms = currentTime;
  lastTime1s = currentTime;
  log_v("Setup finished.");
}

void loop() {
  /* a simple scheduler which calls the cyclic tasks depending on system time */
  currentTime = millis();
  if ((currentTime - lastTime30ms)>30) {
    lastTime30ms += 30;
    task30ms();
  }
  if ((currentTime - lastTime1s)>1000) {
    lastTime1s += 1000;
    task1s();
  }
}
