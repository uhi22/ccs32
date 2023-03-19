
/* Connection Manager */

/* This module is informed by the several state machines in case of good connection.
   It calculates an overall ConnectionLevel.
   This ConnectionLevel is provided to the state machines, so that each state machine
   has the possiblity to decide whether it needs to do something or just stays silent.
   
   The basic rule is, that a good connection on higher layer (e.g. TCP) implicitely
   confirms the good connection on lower layer (e.g. Modem presence). This means,
   the lower-layer state machine can stay silent as long as the upper layers are working
   fine.
*/

uint8_t connMgr_timerModemLocal;
uint8_t connMgr_timerModemRemote;
uint8_t connMgr_timerSlac;
uint8_t connMgr_timerSDP;
uint8_t connMgr_timerTCP;
uint8_t connMgr_timerAppl;
uint8_t connMgr_ConnectionLevel;
uint8_t connMgr_ConnectionLevelOld;
uint8_t connMgr_cycles;

#define CONNMGR_TIMER_MAX (5*33) /* 5 seconds until an OkReport is forgotten. */

uint8_t connMgr_getConnectionLevel(void) {
    return connMgr_ConnectionLevel;
}

void connMgr_printDebugInfos(void) {
    String s;
    s = "[CONNMGR] " + String(connMgr_timerModemLocal) + " "
                     + String(connMgr_timerModemRemote) + " "
                     + String(connMgr_timerSlac) + " "
                     + String(connMgr_timerSDP) + " "
                     + String(connMgr_timerTCP) + " "
                     + String(connMgr_timerAppl) + " "
                     + " --> " + String(connMgr_ConnectionLevel);
    addToTrace(s);
}

void connMgr_Mainfunction(void) {
    /* count all the timers down */
    if (connMgr_timerModemLocal>0) connMgr_timerModemLocal--;
    if (connMgr_timerModemRemote>0) connMgr_timerModemRemote--;
    if (connMgr_timerSlac>0) connMgr_timerSlac--;
    if (connMgr_timerSDP>0) connMgr_timerSDP--;
    if (connMgr_timerTCP>0) connMgr_timerTCP--;
    if (connMgr_timerAppl>0) connMgr_timerAppl--;
    
    /* Based on the timers, calculate the connectionLevel. */
    if      (connMgr_timerAppl>0) {        connMgr_ConnectionLevel=CONNLEVEL_100_APPL_RUNNING; }
    else if (connMgr_timerTCP>0) {         connMgr_ConnectionLevel=CONNLEVEL_80_TCP_RUNNING; }
    else if (connMgr_timerSDP>0) {         connMgr_ConnectionLevel=CONNLEVEL_50_SDP_DONE; }
    else if (connMgr_timerModemRemote>0) { connMgr_ConnectionLevel=CONNLEVEL_20_TWO_MODEMS_FOUND; }
    else if (connMgr_timerSlac>0) {        connMgr_ConnectionLevel=CONNLEVEL_15_SLAC_ONGOING; }
    else if (connMgr_timerModemLocal>0) {  connMgr_ConnectionLevel=CONNLEVEL_10_ONE_MODEM_FOUND; }
    else {connMgr_ConnectionLevel=0;}

    if (isEthLinkUp==0) {
      /* If we have no ethernet link to the modem. So we can immediately say, there is NO connection at all. */
      connMgr_ConnectionLevel=0;      
    }    

    if (connMgr_ConnectionLevelOld!=connMgr_ConnectionLevel) {
      addToTrace("[CONNMGR] ConnectionLevel changed from " + String(connMgr_ConnectionLevelOld) + " to " + String(connMgr_ConnectionLevel));
      connMgr_ConnectionLevelOld = connMgr_ConnectionLevel;
    }
    if ((connMgr_cycles % 30)==0) connMgr_printDebugInfos();
    connMgr_cycles++;
}

void connMgr_ModemFinderOk(uint8_t numberOfFoundModems) {
  if (numberOfFoundModems>=1) {
    connMgr_timerModemLocal = CONNMGR_TIMER_MAX;
  }
  if (numberOfFoundModems>=2) {
    connMgr_timerModemRemote = CONNMGR_TIMER_MAX;
  }
}

void connMgr_SlacOk(void) {
    connMgr_timerSlac = CONNMGR_TIMER_MAX;
}

void connMgr_SdpOk(void) {
    connMgr_timerSDP = CONNMGR_TIMER_MAX;
}

void connMgr_TcpOk(void) {
    connMgr_timerTCP = CONNMGR_TIMER_MAX;
}

void connMgr_ApplOk(void) {
    connMgr_timerAppl = CONNMGR_TIMER_MAX;
}


