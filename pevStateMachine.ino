

/* The Charging State Machine for the car */

#define PEV_STATE_NotYetInitialized 0
#define PEV_STATE_Connecting 1
#define PEV_STATE_Connected 2
#define PEV_STATE_WaitForSupportedApplicationProtocolResponse 3
#define PEV_STATE_WaitForSessionSetupResponse 4
#define PEV_STATE_WaitForServiceDiscoveryResponse 5
#define PEV_STATE_WaitForServicePaymentSelectionResponse 6
#define PEV_STATE_WaitForContractAuthenticationResponse 7
#define PEV_STATE_WaitForChargeParameterDiscoveryResponse 8
#define PEV_STATE_WaitForCableCheckResponse 9
#define PEV_STATE_WaitForPreChargeResponse 10
#define PEV_STATE_WaitForPowerDeliveryResponse 11
#define PEV_STATE_WaitForCurrentDemandResponse 12
#define PEV_STATE_WaitForWeldingDetectionResponse 13
#define PEV_STATE_WaitForSessionStopResponse 14
#define PEV_STATE_ChargingFinished 15
#define PEV_STATE_SequenceTimeout 99

const uint8_t exiDemoSupportedApplicationProtocolRequestIoniq[]={0x80, 0x00, 0xdb, 0xab, 0x93, 0x71, 0xd3, 0x23, 0x4b, 0x71, 0xd1, 0xb9, 0x81, 0x89, 0x91, 0x89, 0xd1, 0x91, 0x81, 0x89, 0x91, 0xd2, 0x6b, 0x9b, 0x3a, 0x23, 0x2b, 0x30, 0x02, 0x00, 0x00, 0x04, 0x00, 0x40 };


uint16_t pev_cyclesInState;
uint8_t pev_state=PEV_STATE_NotYetInitialized;
uint8_t pev_isUserStopRequest=0;

void showBuffer(const uint8_t *buffer, uint8_t len) {
 String s;
 uint8_t i;
 s = "(" + String(len) + " bytes)= ";
 for (i=0; i<len; i++) {
   s = s + String(buffer[i], HEX) + " ";
 } 
 addToTrace(s);
}

void addV2GTPHeaderAndTransmit(const uint8_t *exiBuffer, uint8_t exiBufferLen) {
  // takes the bytearray with exidata, and adds a header to it, according to the Vehicle-to-Grid-Transport-Protocol
  // V2GTP header has 8 bytes
  // 1 byte protocol version
  // 1 byte protocol version inverted
  // 2 bytes payload type
  // 4 byte payload length
  tcpPayload[0] = 0x01; // # version
  tcpPayload[1] = 0xfe; // # version inverted
  tcpPayload[2] = 0x80; // # payload type. 0x8001 means "EXI data"
  tcpPayload[3] = 0x01; // # 
  tcpPayload[4] = (uint8_t)(exiBufferLen >> 24); // length 4 byte.
  tcpPayload[5] = (uint8_t)(exiBufferLen >> 16);
  tcpPayload[6] = (uint8_t)(exiBufferLen >> 8);
  tcpPayload[7] = (uint8_t)exiBufferLen;
  log_v("Step2 %d", exiBufferLen);
  showBuffer(tcpPayload, 8);  
  if (exiBufferLen+8<TCP_PAYLOAD_LEN) {   
      memcpy(&tcpPayload[8], exiBuffer, exiBufferLen);    
      tcpPayloadLen = 8 + exiBufferLen; /* 8 byte V2GTP header, plus the EXI data */
      tcp_transmit();
  } else {
      addToTrace("Error: EXI does not fit into tcpPayload.");     
  }
}

/**** State functions ***************/
void pev_stateFunctionConnecting(void) {
  if (pev_cyclesInState<30) { // # The first second in the state just do nothing.
    return;
  }
  //evseIp = self.addressManager.getSeccIp() # the chargers IP address which was announced in SDP
  //seccTcpPort = self.addressManager.getSeccTcpPort() # the chargers TCP port which was announced in SDP
  if (tcp_isClosed()) tcp_connect(); /* This is a NOT blocking call. It just initiates the connection, but we must wait a second until the
                Handshake is done. */
  if ((pev_cyclesInState>60)) {
            // Bad case: Connection did not work. May happen if we are too fast and the charger needs more
            // time until the socket is ready. Or the charger is defective. Or somebody pulled the plug.
            // No matter what is the reason, we just try again and again. What else would make sense?
            addToTrace("Connection failed. Will try again.");
            pevStateMachine_ReInit(); // stay in same state, reset the cyclesInState and try again
            return;
  }
  if (tcp_isConnected()) { 
            // Good case: We are connected. Change to the next state.
            addToTrace("connected");
            //publishStatus("TCP connected")
            pev_isUserStopRequest = 0;
            pev_enterState(PEV_STATE_Connected);
            return;
  }
}

void stateFunctionConnected(void) {
        // We have a freshly established TCP channel. We start the V2GTP/EXI communication now.
        // We just use the initial request message from the Ioniq. It contains one entry: DIN.
        addToTrace("Sending the initial SupportedApplicationProtocolReq");
        addV2GTPHeaderAndTransmit(exiDemoSupportedApplicationProtocolRequestIoniq, sizeof(exiDemoSupportedApplicationProtocolRequestIoniq));
        //hardwareInterface.resetSimulation()
        pev_enterState(PEV_STATE_WaitForSupportedApplicationProtocolResponse);
}

void pev_testExiSend(void) {
    addToTrace("Step1");
    addV2GTPHeaderAndTransmit(exiDemoSupportedApplicationProtocolRequestIoniq, sizeof(exiDemoSupportedApplicationProtocolRequestIoniq));  
}

void stateFunctionWaitForSupportedApplicationProtocolResponse(void) {
        if (tcp_rxdataLen>V2GTP_HEADER_SIZE) {
            addToTrace("In state WaitForSupportedApplicationProtocolResponse, received:");
            showBuffer(tcp_rxdata, tcp_rxdataLen);
            //showBuffer(myreceivebuffer, 74+tcp_rxdataLen );           
            //exidata = removeV2GTPHeader(self.rxData)
            global_stream1.data = &tcp_rxdata[V2GTP_HEADER_SIZE];         
            global_stream1.size = tcp_rxdataLen - V2GTP_HEADER_SIZE; 
            projectExiConnector_decode_appHandExiDocument();
            if (aphsDoc.supportedAppProtocolRes_isUsed) {
			        /* it is a response */
			        addToTrace("supportedAppProtocolRes");
			        sprintf(gResultString, "ResponseCode %d, SchemaID_isUsed %d, SchemaID %d",
				          aphsDoc.supportedAppProtocolRes.ResponseCode,
				          aphsDoc.supportedAppProtocolRes.SchemaID_isUsed,
				          aphsDoc.supportedAppProtocolRes.SchemaID);
              addToTrace(gResultString);
            }              
            tcp_rxdataLen = 0;
            //strConverterResult = self.exiDecode(exidata, "Dh") # Decode Handshake-response
            //self.addToTrace(strConverterResult)
            //if (strConverterResult.find("supportedAppProtocolRes")>0) {
            //    # todo: check the request content, and fill response parameters
            //    self.publishStatus("Schema negotiated")
            //    self.addToTrace("Will send SessionSetupReq")
            //    msg = addV2GTPHeader(self.exiEncode("EDA")) # EDA for Encode, Din, SessionSetupReq
            //    self.addToTrace("responding " + prettyHexMessage(msg))
            //    self.Tcp.transmit(msg)
            pev_enterState(PEV_STATE_WaitForSessionSetupResponse);
        }
        if (pev_isTooLong()) {
            pev_enterState(PEV_STATE_SequenceTimeout);
        }
}

void stateFunction(void) {
  /* dummy, todo */  
}

void pev_enterState(uint8_t n) {
  addToTrace("[PEV] from " + String(pev_state) + " entering " + String(n));
  pev_state = n;
  pev_cyclesInState = 0;
}

uint8_t pev_isTooLong(void) {
  uint8_t limit;
  // The timeout handling function.
  limit = 30; // number of call cycles until timeout
  if (pev_state==PEV_STATE_WaitForCableCheckResponse) {
    limit = 30*30; // # CableCheck may need some time. Wait at least 30s.
  }
  if (pev_state==PEV_STATE_WaitForPreChargeResponse) {
    limit = 30*30; // # PreCharge may need some time. Wait at least 30s.
  }
  return (pev_cyclesInState > limit);
}

/******* The statemachine dispatcher *******************/
void pev_runFsm(void) {
 if (isEthLinkUp==0) {
        /* If we have no ethernet link to the modem, nothing to do here. Just wait for the link. */
        if (pev_state!=PEV_STATE_NotYetInitialized) pev_enterState(PEV_STATE_NotYetInitialized);   
        return;
 }
 switch (pev_state) {
    case PEV_STATE_Connecting: pev_stateFunctionConnecting(); break;
    case PEV_STATE_Connected: stateFunctionConnected();  break;
    case PEV_STATE_WaitForSupportedApplicationProtocolResponse: stateFunctionWaitForSupportedApplicationProtocolResponse(); break;
    case PEV_STATE_WaitForSessionSetupResponse: stateFunction(); break;
    case PEV_STATE_WaitForServiceDiscoveryResponse: stateFunction(); break;
    case PEV_STATE_WaitForServicePaymentSelectionResponse: stateFunction(); break;
    case PEV_STATE_WaitForContractAuthenticationResponse: stateFunction(); break;
    case PEV_STATE_WaitForChargeParameterDiscoveryResponse: stateFunction(); break;
    case PEV_STATE_WaitForCableCheckResponse: stateFunction(); break;
    case PEV_STATE_WaitForPreChargeResponse: stateFunction(); break;
    case PEV_STATE_WaitForPowerDeliveryResponse: stateFunction(); break;
    case PEV_STATE_WaitForCurrentDemandResponse: stateFunction(); break;
    case PEV_STATE_WaitForWeldingDetectionResponse: stateFunction(); break;
    case PEV_STATE_WaitForSessionStopResponse: stateFunction(); break;
    case PEV_STATE_ChargingFinished: stateFunction(); break;
    case PEV_STATE_SequenceTimeout: stateFunction(); break;
 }
}

/************ public interfaces *****************************************/
/* The init function for the PEV charging state machine. */
void pevStateMachine_Init(void) {
  pev_state=PEV_STATE_NotYetInitialized;
}

void pevStateMachine_ReInit(void) {
  addToTrace("re-initializing fsmPev");
  tcp_Disconnect();
  //hardwareInterface.setStateB()
  //hardwareInterface.setPowerRelayOff()
  //hardwareInterface.setRelay2Off()
  //isBulbOn = False
  //cyclesLightBulbDelay = 0
  pev_state = PEV_STATE_Connecting;
  pev_cyclesInState = 0;
}

/* The cyclic main function of the PEV charging state machine.
   Called each 30ms. */
void pevStateMachine_Mainfunction(void) {
    // run the state machine:
    pev_cyclesInState += 1; // for timeout handling, count how long we are in a state
    pev_runFsm();
}
