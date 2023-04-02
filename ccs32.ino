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
/* extern variables for debugging */
//extern uint32_t uwe_rxMallocAccumulated;
//extern uint32_t uwe_rxCounter;
/**********************************************************/

#define PIN_LED 2 /* The IO2 is used for an LED. This LED is externally added to the WT32-ETH01 board. */
#define PIN_STATE_C 4 /* The IO4 is used to change the CP line to state C. High=StateC, Low=StateB */ 
#define PIN_POWER_RELAIS 35 /* IO35 for the power relay */
uint32_t currentTime;
uint32_t lastTime1s;
uint32_t lastTime30ms;
uint32_t nCycles30ms;
uint8_t ledState;
uint32_t initialHeapSpace;
uint32_t eatenHeapSpace;
String globalLine1;
String globalLine2;
String globalLine3;
uint16_t counterForDisplayUpdate;


void sanityCheck(String info) {
  int r;
  r= hardwareInterface_sanityCheck();
  r=r | homeplug_sanityCheck();
  if (eatenHeapSpace>10000) {
    /* if something is eating the heap, this is a fatal error. */
    addToTrace("ERROR: Sanity check failed due to heap space check.");
    r = -10;
  }
  if (r!=0) {
      addToTrace(String("ERROR: Sanity check failed ") + String(r) + " " + info);
      delay(2000); /* Todo: we should make a reset here. */
  }
}

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

void showAsHex(uint8_t *arr, uint16_t len, char *info) {
 char strTmp[10];
 #define MAX_RESULT_LEN 700
 char strResult[MAX_RESULT_LEN];
 uint16_t i;
 sprintf(strResult, "%s has %d bytes:", info, len);
 for (i=0; i<len; i++) {
  sprintf(strTmp, "%02hx ", arr[i]);
  if (strlen(strResult)<MAX_RESULT_LEN-10) {  
    strcat(strResult, strTmp);
  } else {
    /* does not fit. Just ignore the remaining bytes. */
  }
 }
 addToTrace_chararray(strResult);
} 

/**********************************************************/
/* The global status printer */
void publishStatus(String line1, String line2 = "", String line3 = "") {
  globalLine1=line1;
  globalLine2=line2;
  globalLine3=line3;
}  

void cyclicLcdUpdate(void) {
  uint32_t t;
  uint16_t minutes, seconds;
  String strMinutes, strSeconds, strLine3extended;
  if (counterForDisplayUpdate>0) {
    counterForDisplayUpdate--;  
  } else {
    /* show the uptime in the third line */  
    t = millis()/1000;
    minutes = t / 60;
    seconds = t - (minutes*60);
    strMinutes = String(minutes);
    strSeconds = String(seconds);  
    if (strMinutes.length()<2) strMinutes = "0" + strMinutes;
    if (strSeconds.length()<2) strSeconds = "0" + strSeconds;
    strLine3extended = globalLine3 + " " + strMinutes + ":" + strSeconds;
    hardwareInterface_showOnDisplay(globalLine1, globalLine2, strLine3extended);
    counterForDisplayUpdate=15; /* 15*30ms=450ms until forced cyclic update of the LCD */  
  }
}

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
  cyclicLcdUpdate();
  sanityCheck("cyclic30ms");
}

/* This task runs once a second. */
void task1s(void) {
  if (ledState==0) {
    digitalWrite(PIN_LED,HIGH);
    //Serial.println("LED on");
    ledState = 1;
  } else {
    digitalWrite(PIN_LED,LOW);
    //Serial.println("LED off");
    ledState = 0;
  }
  //log_v("nTotalEthReceiveBytes=%ld, nCycles30ms=%ld", nTotalEthReceiveBytes, nCycles30ms);
  //log_v("nTotalEthReceiveBytes=%ld, nMaxInMyEthernetReceiveCallback=%d, nTcpPacketsReceived=%d", nTotalEthReceiveBytes, nMaxInMyEthernetReceiveCallback, nTcpPacketsReceived);
  //log_v("nTotalTransmittedBytes=%ld", nTotalTransmittedBytes);
  //pev_testExiSend(); /* just for testing, send some EXI data. */
  //tcp_testSendData(); /* just for testing, send something with TCP. */
  //sendTestFrame(); /* just for testing, send something on the Ethernet. */
  eatenHeapSpace = initialHeapSpace - ESP.getFreeHeap();
  //Serial.println("EatenHeapSpace=" + String(eatenHeapSpace) + " uwe_rxCounter=" + String(uwe_rxCounter) + " uwe_rxMallocAccumulated=" + String(uwe_rxMallocAccumulated) );
  if (eatenHeapSpace>1000) {
    /* if we lost more than 1000 bytes on heap, print a waring message: */
    Serial.println("WARNING: EatenHeapSpace=" + String(eatenHeapSpace));
  }
}

/**********************************************************/
/* The Arduino standard entry points */

void setup() {
  // Set pin mode
  pinMode(PIN_LED,OUTPUT);
  pinMode(PIN_STATE_C, OUTPUT);
  pinMode(PIN_POWER_RELAIS, OUTPUT);
  Serial.begin(115200);
  Serial.println("CCS32 Started.");
  if (!initEth()) {
    log_v("Error: Ethernet init failed.");
  }
  homeplugInit();
  pevStateMachine_Init();
  hardwareInterface_initDisplay();
  /* The time for the tasks starts here. */
  currentTime = millis();
  lastTime30ms = currentTime;
  lastTime1s = currentTime;
  log_v("Setup finished.");
  initialHeapSpace=ESP.getFreeHeap();
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
