
/* Hardware Interface module */

#include <HardwareSerial.h>
HardwareSerial mySerial(1); /* 0 would be the Serial which is used for Serial.print. 1 is the Serial2. 
                               The pins can be freely chosen, see below. */
/* TXD for display is IO17 according to the data sheet of the WT32-ETH01. */
/* RXD (unused) is IO5 according to the data sheet of the WT32-ETH01. */
#define TX2_PIN 17
#define RX2_PIN 5


void hardwareInterface_showOnDisplay(String s1, String s2, String s3) {
  /* shows three lines on the display */
  String s;
  /* To address the display, each line starts with "lc" (short for "LCD").
     Each line ends with an carriage return ("\n"). */
  s = "lc" + s1 + "\n" + "lc" + s2 + "\n" + "lc" + s3 + "\n";
  mySerial.println(s);
}

void hardwareInterface_initDisplay(void) {
  mySerial.begin(19200, SERIAL_8N1, RX2_PIN, TX2_PIN);
  //s = "lc" + s1 + "\n" + "lc" + s2 + "\n" + "lc" + s3 + "\n"  
  mySerial.println("lcHello :-)\nlcfrom\nlcWT32-ETH");
}

uint16_t hwIf_simulatedSoc_0p1;

void hardwareInterface_simulatePreCharge(void) {
}

void hardwareInterface_simulateCharging(void) {
  if (hwIf_simulatedSoc_0p1<1000) {
    /* simulate increasing SOC */
    hwIf_simulatedSoc_0p1++;
  }
}

int16_t hardwareInterface_getInletVoltage(void) {
  return 219;
}

int16_t hardwareInterface_getAccuVoltage(void) {
  return 222;
}

int16_t hardwareInterface_getChargingTargetVoltage(void) {
  return 229;
}

int16_t hardwareInterface_getChargingTargetCurrent(void) {
  return 5;
}

uint8_t hardwareInterface_getSoc(void) {
  /* SOC in percent */
  return hwIf_simulatedSoc_0p1/10;
}

uint8_t hardwareInterface_getIsAccuFull(void) {
    return (hwIf_simulatedSoc_0p1/10)>95;
}

void hardwareInterface_setPowerRelayOn(void) {
    
}

void hardwareInterface_setPowerRelayOff(void) {
}

void hardwareInterface_setRelay2On(void) {
}

void hardwareInterface_setRelay2Off(void) {
}

void hardwareInterface_setStateB(void) {
}

void hardwareInterface_setStateC(void) {
}

void hardwareInterface_resetSimulation(void) {
    hwIf_simulatedSoc_0p1 = 200; /* 20% */
}