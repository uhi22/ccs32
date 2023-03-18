
/* Hardware Interface module */

void hardwareInterface_simulatePreCharge(void) {
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
  return 88;
}

uint8_t hardwareInterface_getIsAccuFull(void) { return 0; }

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
}