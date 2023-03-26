

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
uint8_t pev_DelayCycles;
uint8_t pev_state=PEV_STATE_NotYetInitialized;
uint8_t pev_isUserStopRequest=0;
uint16_t pev_numberOfContractAuthenticationReq;
uint16_t pev_numberOfChargeParameterDiscoveryReq;
uint16_t pev_numberOfCableCheckReq;
uint8_t pev_wasPowerDeliveryRequestedOn;
uint8_t pev_isBulbOn;
uint16_t pev_cyclesLightBulbDelay;


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
  tcpPayload[0] = 0x01; // version
  tcpPayload[1] = 0xfe; // version inverted
  tcpPayload[2] = 0x80; // payload type. 0x8001 means "EXI data"
  tcpPayload[3] = 0x01; // 
  tcpPayload[4] = (uint8_t)(exiBufferLen >> 24); // length 4 byte.
  tcpPayload[5] = (uint8_t)(exiBufferLen >> 16);
  tcpPayload[6] = (uint8_t)(exiBufferLen >> 8);
  tcpPayload[7] = (uint8_t)exiBufferLen;
  if (exiBufferLen+8<TCP_PAYLOAD_LEN) {
      memcpy(&tcpPayload[8], exiBuffer, exiBufferLen);
      tcpPayloadLen = 8 + exiBufferLen; /* 8 byte V2GTP header, plus the EXI data */
      log_v("Step3 %d", tcpPayloadLen);
      showBuffer(tcpPayload, tcpPayloadLen);
      tcp_transmit();
  } else {
      addToTrace("Error: EXI does not fit into tcpPayload.");
  }
}

void encodeAndTransmit(void) {
  /* calls the EXI encoder, adds the V2GTP header and sends the result to ethernet */
  //addToTrace("before: g_errn=" + String(g_errn));
  //addToTrace("global_streamEncPos=" + String(global_streamEncPos));
  global_streamEncPos = 0;
  projectExiConnector_encode_DinExiDocument();
  //addToTrace("after: g_errn=" + String(g_errn));
  //addToTrace("global_streamEncPos=" + String(global_streamEncPos));
  addV2GTPHeaderAndTransmit(global_streamEnc.data, global_streamEncPos);
}

void routeDecoderInputData(void) {
  /* connect the data from the TCP to the exiDecoder */
  /* The TCP receive data consists of two parts: 1. The V2GTP header and 2. the EXI stream.
     The decoder wants only the EXI stream, so we skip the V2GTP header.
     In best case, we would check also the consistency of the V2GTP header here.
  */
  global_streamDec.data = &tcp_rxdata[V2GTP_HEADER_SIZE];
  global_streamDec.size = tcp_rxdataLen - V2GTP_HEADER_SIZE;
  //addToTrace("The decoder will see:");  
  showBuffer(global_streamDec.data, global_streamDec.size);
  /* We have something to decode, this is a good sign that the connection is fine.
     Inform the ConnectionManager that everything is fine. */
  connMgr_ApplOk();
}

/********* EXI creation functions ************************/
void pev_sendChargeParameterDiscoveryReq(void) {
    struct dinDC_EVChargeParameterType *cp;
    projectExiConnector_prepare_DinExiDocument();
    dinDocEnc.V2G_Message.Body.ChargeParameterDiscoveryReq_isUsed = 1u;
    init_dinChargeParameterDiscoveryReqType(&dinDocEnc.V2G_Message.Body.ChargeParameterDiscoveryReq);
    dinDocEnc.V2G_Message.Body.ChargeParameterDiscoveryReq.EVRequestedEnergyTransferType = dinEVRequestedEnergyTransferType_DC_extended;
    cp = &dinDocEnc.V2G_Message.Body.ChargeParameterDiscoveryReq.DC_EVChargeParameter;
    cp->DC_EVStatus.EVReady = 1; /* todo: what ever this means */
    cp->DC_EVStatus.EVRESSSOC = hardwareInterface_getSoc(); /* Todo: Take the SOC from the BMS. Scaling is 1%. */
    cp->EVMaximumCurrentLimit.Multiplier = 0; /* -3 to 3. The exponent for base of 10. */
    cp->EVMaximumCurrentLimit.Unit = dinunitSymbolType_A;
    cp->EVMaximumCurrentLimit.Value = 100;
    cp->EVMaximumVoltageLimit.Multiplier = 0; /* -3 to 3. The exponent for base of 10. */
    cp->EVMaximumVoltageLimit.Unit = dinunitSymbolType_V;
    cp->EVMaximumVoltageLimit.Value = 398;
    dinDocEnc.V2G_Message.Body.ChargeParameterDiscoveryReq.DC_EVChargeParameter_isUsed = 1;
    encodeAndTransmit();
}
        
void pev_sendCableCheckReq(void) {
    projectExiConnector_prepare_DinExiDocument();
    dinDocEnc.V2G_Message.Body.CableCheckReq_isUsed = 1u;
    init_dinCableCheckReqType(&dinDocEnc.V2G_Message.Body.CableCheckReq);
    #define st dinDocEnc.V2G_Message.Body.CableCheckReq.DC_EVStatus
      st.EVReady = 1; /* 1 means true. We are ready. */
      st.EVErrorCode = dinDC_EVErrorCodeType_NO_ERROR;
      st.EVRESSSOC = hardwareInterface_getSoc(); /* Scaling is 1%. */
    #undef st
    encodeAndTransmit();
}

void pev_sendPreChargeReq(void) {
    projectExiConnector_prepare_DinExiDocument();
    dinDocEnc.V2G_Message.Body.PreChargeReq_isUsed = 1u;
    init_dinPreChargeReqType(&dinDocEnc.V2G_Message.Body.PreChargeReq);
    #define st dinDocEnc.V2G_Message.Body.PreChargeReq.DC_EVStatus
      st.EVReady = 1; /* 1 means true. We are ready. */
      st.EVErrorCode = dinDC_EVErrorCodeType_NO_ERROR;
      st.EVRESSSOC = hardwareInterface_getSoc(); /* The SOC. Scaling is 1%. */
    #undef st
    #define tvolt dinDocEnc.V2G_Message.Body.PreChargeReq.EVTargetVoltage
      tvolt.Multiplier = 0; /* -3 to 3. The exponent for base of 10. */
      tvolt.Unit = dinunitSymbolType_V;
      tvolt.Unit_isUsed = 1;
      tvolt.Value = hardwareInterface_getAccuVoltage(); /* The precharge target voltage. Scaling is 1V. */
    #undef tvolt
    #define tcurr dinDocEnc.V2G_Message.Body.PreChargeReq.EVTargetCurrent
      tcurr.Multiplier = 0; /* -3 to 3. The exponent for base of 10. */
      tcurr.Unit = dinunitSymbolType_A;
      tcurr.Unit_isUsed = 1;
      tcurr.Value = 1; /* 1A for precharging */
    #undef tcurr
    encodeAndTransmit();
}

void pev_sendPowerDeliveryReq(uint8_t isOn) {
  projectExiConnector_prepare_DinExiDocument();
  dinDocEnc.V2G_Message.Body.PowerDeliveryReq_isUsed = 1u;
  init_dinPowerDeliveryReqType(&dinDocEnc.V2G_Message.Body.PowerDeliveryReq);
  dinDocEnc.V2G_Message.Body.PowerDeliveryReq.ReadyToChargeState = isOn; /* 1=ON, 0=OFF */
  dinDocEnc.V2G_Message.Body.PowerDeliveryReq.DC_EVPowerDeliveryParameter_isUsed = 1;
  dinDocEnc.V2G_Message.Body.PowerDeliveryReq.DC_EVPowerDeliveryParameter.DC_EVStatus.EVReady = 1; /* 1 means true. We are ready. */
  dinDocEnc.V2G_Message.Body.PowerDeliveryReq.DC_EVPowerDeliveryParameter.DC_EVStatus.EVErrorCode = dinDC_EVErrorCodeType_NO_ERROR;
  dinDocEnc.V2G_Message.Body.PowerDeliveryReq.DC_EVPowerDeliveryParameter.DC_EVStatus.EVRESSSOC = hardwareInterface_getSoc();
  dinDocEnc.V2G_Message.Body.PowerDeliveryReq.DC_EVPowerDeliveryParameter.ChargingComplete = 0; /* boolean. Charging not finished. */
  encodeAndTransmit();
}

void pev_sendCurrentDemandReq(void) {
  projectExiConnector_prepare_DinExiDocument();
  dinDocEnc.V2G_Message.Body.CurrentDemandReq_isUsed = 1u;
  init_dinCurrentDemandReqType(&dinDocEnc.V2G_Message.Body.CurrentDemandReq);
  // DC_EVStatus
  #define st dinDocEnc.V2G_Message.Body.CurrentDemandReq.DC_EVStatus
    st.EVReady = 1; /* 1 means true. We are ready. */
    st.EVErrorCode = dinDC_EVErrorCodeType_NO_ERROR;
    st.EVRESSSOC = hardwareInterface_getSoc();
  #undef st	
  // EVTargetVoltage
  #define tvolt dinDocEnc.V2G_Message.Body.CurrentDemandReq.EVTargetVoltage
    tvolt.Multiplier = 0;  /* -3 to 3. The exponent for base of 10. */
    tvolt.Unit = dinunitSymbolType_V;
    tvolt.Unit_isUsed = 1;
    tvolt.Value = hardwareInterface_getChargingTargetVoltage(); /* The charging target. Scaling is 1V. */
  #undef tvolt
  // EVTargetCurrent
  #define tcurr dinDocEnc.V2G_Message.Body.CurrentDemandReq.EVTargetCurrent
    tcurr.Multiplier = 0;  /* -3 to 3. The exponent for base of 10. */
    tcurr.Unit = dinunitSymbolType_A;
    tcurr.Unit_isUsed = 1;
    tcurr.Value = hardwareInterface_getChargingTargetCurrent(); /* The charging target current. Scaling is 1A. */
  #undef tcurr
  dinDocEnc.V2G_Message.Body.CurrentDemandReq.ChargingComplete = 0; /* boolean. Todo: Do we need to take this from command line? Or is it fine
    that the PEV just sends a PowerDeliveryReq with STOP, if it decides to stop the charging? */
  dinDocEnc.V2G_Message.Body.CurrentDemandReq.BulkChargingComplete_isUsed = 1u;
  dinDocEnc.V2G_Message.Body.CurrentDemandReq.BulkChargingComplete = 0u; /* not complete */
  dinDocEnc.V2G_Message.Body.CurrentDemandReq.RemainingTimeToFullSoC_isUsed = 1u;
  dinDocEnc.V2G_Message.Body.CurrentDemandReq.RemainingTimeToFullSoC.Multiplier = 0;  /* -3 to 3. The exponent for base of 10. */
  dinDocEnc.V2G_Message.Body.CurrentDemandReq.RemainingTimeToFullSoC.Unit = dinunitSymbolType_s;
  dinDocEnc.V2G_Message.Body.CurrentDemandReq.RemainingTimeToFullSoC.Unit_isUsed = 1;
  dinDocEnc.V2G_Message.Body.CurrentDemandReq.RemainingTimeToFullSoC.Value = 1200; /* seconds */

  dinDocEnc.V2G_Message.Body.CurrentDemandReq.RemainingTimeToBulkSoC_isUsed = 1u;
  dinDocEnc.V2G_Message.Body.CurrentDemandReq.RemainingTimeToBulkSoC.Multiplier = 0;  /* -3 to 3. The exponent for base of 10. */
  dinDocEnc.V2G_Message.Body.CurrentDemandReq.RemainingTimeToBulkSoC.Unit = dinunitSymbolType_s;
  dinDocEnc.V2G_Message.Body.CurrentDemandReq.RemainingTimeToBulkSoC.Unit_isUsed = 1;
  dinDocEnc.V2G_Message.Body.CurrentDemandReq.RemainingTimeToBulkSoC.Value = 600; /* seconds */
  encodeAndTransmit();    
}

void pev_sendWeldingDetectionReq(void) {
  projectExiConnector_prepare_DinExiDocument();
  dinDocEnc.V2G_Message.Body.WeldingDetectionReq_isUsed = 1u;
  init_dinWeldingDetectionReqType(&dinDocEnc.V2G_Message.Body.WeldingDetectionReq);
  #define st dinDocEnc.V2G_Message.Body.WeldingDetectionReq.DC_EVStatus
    st.EVReady = 1; /* 1 means true. We are ready. */
    st.EVErrorCode = dinDC_EVErrorCodeType_NO_ERROR;
    st.EVRESSSOC = hardwareInterface_getSoc();
  #undef st	
  encodeAndTransmit();  
}

/**** State functions ***************/

void stateFunctionConnected(void) {
  // We have a freshly established TCP channel. We start the V2GTP/EXI communication now.
  // We just use the initial request message from the Ioniq. It contains one entry: DIN.
  addToTrace("Checkpoint400: Sending the initial SupportedApplicationProtocolReq");
  addV2GTPHeaderAndTransmit(exiDemoSupportedApplicationProtocolRequestIoniq, sizeof(exiDemoSupportedApplicationProtocolRequestIoniq));
  hardwareInterface_resetSimulation();
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
    routeDecoderInputData();
    projectExiConnector_decode_appHandExiDocument();
    tcp_rxdataLen = 0; /* mark the input data as "consumed" */
    if (aphsDoc.supportedAppProtocolRes_isUsed) {
        /* it is the correct response */
        addToTrace("supportedAppProtocolRes");
        sprintf(gResultString, "ResponseCode %d, SchemaID_isUsed %d, SchemaID %d",
                      aphsDoc.supportedAppProtocolRes.ResponseCode,
                      aphsDoc.supportedAppProtocolRes.SchemaID_isUsed,
                      aphsDoc.supportedAppProtocolRes.SchemaID);
        addToTrace(gResultString);
        publishStatus("Schema negotiated");
        addToTrace("Checkpoint403: Schema negotiated. And Checkpoint500: Will send SessionSetupReq");
        projectExiConnector_prepare_DinExiDocument();
        dinDocEnc.V2G_Message.Body.SessionSetupReq_isUsed = 1u;
        init_dinSessionSetupReqType(&dinDocEnc.V2G_Message.Body.SessionSetupReq);
        /* In the session setup request, the session ID zero means: create a new session.
            The format (len 8, all zero) is taken from the original Ioniq behavior. */
        dinDocEnc.V2G_Message.Header.SessionID.bytes[0] = 0;
        dinDocEnc.V2G_Message.Header.SessionID.bytes[1] = 0;
        dinDocEnc.V2G_Message.Header.SessionID.bytes[2] = 0;
        dinDocEnc.V2G_Message.Header.SessionID.bytes[3] = 0;
        dinDocEnc.V2G_Message.Header.SessionID.bytes[4] = 0;
        dinDocEnc.V2G_Message.Header.SessionID.bytes[5] = 0;
        dinDocEnc.V2G_Message.Header.SessionID.bytes[6] = 0;
        dinDocEnc.V2G_Message.Header.SessionID.bytes[7] = 0;
        dinDocEnc.V2G_Message.Header.SessionID.bytesLen = 8;
        encodeAndTransmit();
        pev_enterState(PEV_STATE_WaitForSessionSetupResponse);
    }
  }
  if (pev_isTooLong()) {
    pev_enterState(PEV_STATE_SequenceTimeout);
  }
}

void stateFunctionWaitForSessionSetupResponse(void) {
  if (tcp_rxdataLen>V2GTP_HEADER_SIZE) {
    addToTrace("In state WaitForSessionSetupResponse, received:");
    showBuffer(tcp_rxdata, tcp_rxdataLen);
    routeDecoderInputData();
    projectExiConnector_decode_DinExiDocument();
    tcp_rxdataLen = 0; /* mark the input data as "consumed" */
    addToTrace("after decoding: g_errn=" + String(g_errn));
    addToTrace("global_streamDecPos=" + String(global_streamDecPos));
    if (dinDocDec.V2G_Message.Body.SessionSetupRes_isUsed) {
      memcpy(sessionId, dinDocDec.V2G_Message.Header.SessionID.bytes, SESSIONID_LEN);
      sessionIdLen = dinDocDec.V2G_Message.Header.SessionID.bytesLen; /* store the received SessionID, we will need it later. */
      addToTrace("Checkpoint506: The Evse decided for SessionId");
      showBuffer(sessionId, sessionIdLen);
      publishStatus("Session established");
      addToTrace("Will send ServiceDiscoveryReq");
      projectExiConnector_prepare_DinExiDocument();
      dinDocEnc.V2G_Message.Body.ServiceDiscoveryReq_isUsed = 1u;
      init_dinServiceDiscoveryReqType(&dinDocEnc.V2G_Message.Body.ServiceDiscoveryReq);
      encodeAndTransmit();
      pev_enterState(PEV_STATE_WaitForServiceDiscoveryResponse);
    }
  }
  if (pev_isTooLong()) {
    pev_enterState(PEV_STATE_SequenceTimeout);
  }
}

void stateFunctionWaitForServiceDiscoveryResponse(void) {
  if (tcp_rxdataLen>V2GTP_HEADER_SIZE) {
    addToTrace("In state WaitForServiceDiscoveryResponse, received:");
    showBuffer(tcp_rxdata, tcp_rxdataLen);
    routeDecoderInputData();
    projectExiConnector_decode_DinExiDocument();
    tcp_rxdataLen = 0; /* mark the input data as "consumed" */
    if (dinDocDec.V2G_Message.Body.ServiceDiscoveryRes_isUsed) {
      publishStatus("ServDisc done");
      addToTrace("Will send ServicePaymentSelectionReq");
      projectExiConnector_prepare_DinExiDocument();
      dinDocEnc.V2G_Message.Body.ServicePaymentSelectionReq_isUsed = 1u;
      init_dinServicePaymentSelectionReqType(&dinDocEnc.V2G_Message.Body.ServicePaymentSelectionReq);
      /* the mandatory fields in ISO are SelectedPaymentOption and SelectedServiceList. Same in DIN. */
      dinDocEnc.V2G_Message.Body.ServicePaymentSelectionReq.SelectedPaymentOption = dinpaymentOptionType_ExternalPayment; /* not paying per car */
      dinDocEnc.V2G_Message.Body.ServicePaymentSelectionReq.SelectedServiceList.SelectedService.array[0].ServiceID = 1; /* todo: what ever this means. The Ioniq uses 1. */
      dinDocEnc.V2G_Message.Body.ServicePaymentSelectionReq.SelectedServiceList.SelectedService.arrayLen = 1; /* just one element in the array */ 
      encodeAndTransmit();
      pev_enterState(PEV_STATE_WaitForServicePaymentSelectionResponse);
    }
  }
  if (pev_isTooLong()) {
    pev_enterState(PEV_STATE_SequenceTimeout);
  }
}

void stateFunctionWaitForServicePaymentSelectionResponse(void) {
  if (tcp_rxdataLen>V2GTP_HEADER_SIZE) {
    addToTrace("In state WaitForServicePaymentSelectionResponse, received:");
    showBuffer(tcp_rxdata, tcp_rxdataLen);
    routeDecoderInputData();
    projectExiConnector_decode_DinExiDocument();
    tcp_rxdataLen = 0; /* mark the input data as "consumed" */
    if (dinDocDec.V2G_Message.Body.ServicePaymentSelectionRes_isUsed) {
      publishStatus("ServPaySel done");
      addToTrace("Checkpoint530: Will send ContractAuthenticationReq");
      projectExiConnector_prepare_DinExiDocument();
      dinDocEnc.V2G_Message.Body.ContractAuthenticationReq_isUsed = 1u;
      init_dinContractAuthenticationReqType(&dinDocEnc.V2G_Message.Body.ContractAuthenticationReq);
      /* no other fields are manatory */      
      encodeAndTransmit();
      pev_numberOfContractAuthenticationReq = 1; // This is the first request.
      pev_enterState(PEV_STATE_WaitForContractAuthenticationResponse);
    }
  }
  if (pev_isTooLong()) {
    pev_enterState(PEV_STATE_SequenceTimeout);
  }
}

void stateFunctionWaitForContractAuthenticationResponse(void) {
  if (pev_cyclesInState<30) { // The first second in the state just do nothing.
    return;
  }
  if (tcp_rxdataLen>V2GTP_HEADER_SIZE) {
    addToTrace("In state WaitForContractAuthenticationResponse, received:");
    showBuffer(tcp_rxdata, tcp_rxdataLen);
    routeDecoderInputData();
    projectExiConnector_decode_DinExiDocument();
    tcp_rxdataLen = 0; /* mark the input data as "consumed" */
    if (dinDocDec.V2G_Message.Body.ContractAuthenticationRes_isUsed) {
        // In normal case, we can have two results here: either the Authentication is needed (the user
        // needs to authorize by RFID card or app, or something like this.
        // Or, the authorization is finished. This is shown by EVSEProcessing=Finished.
        if (dinDocDec.V2G_Message.Body.ContractAuthenticationRes.EVSEProcessing == dinEVSEProcessingType_Finished) {             
            publishStatus("Auth finished");
            addToTrace("Checkpoint538: Auth is Finished. Will send ChargeParameterDiscoveryReq");
            pev_sendChargeParameterDiscoveryReq();
            pev_numberOfChargeParameterDiscoveryReq = 1; // first message
            pev_enterState(PEV_STATE_WaitForChargeParameterDiscoveryResponse);
        } else {
            // Not (yet) finished.
            if (pev_numberOfContractAuthenticationReq>=120) { // approx 120 seconds, maybe the user searches two minutes for his RFID card...
                addToTrace("Authentication lasted too long. Giving up.");
                pev_enterState(PEV_STATE_SequenceTimeout);
            } else {
                // Try again.
                pev_numberOfContractAuthenticationReq += 1; // count the number of tries.
                publishStatus("Waiting f Auth");
                addToTrace("Not (yet) finished. Will again send ContractAuthenticationReq #" + String(pev_numberOfContractAuthenticationReq));
                encodeAndTransmit();
                // We just stay in the same state, until the timeout elapses.
                pev_enterState(PEV_STATE_WaitForContractAuthenticationResponse);
            }
        }
    }
  }
  if (pev_isTooLong()) {
    pev_enterState(PEV_STATE_SequenceTimeout);
  }
}

void stateFunctionWaitForChargeParameterDiscoveryResponse(void) {
  if (pev_cyclesInState<30) { // The first second in the state just do nothing.
    return;
  }
  if (tcp_rxdataLen>V2GTP_HEADER_SIZE) {
    addToTrace("In state WaitForChargeParameterDiscoveryResponse, received:");
    showBuffer(tcp_rxdata, tcp_rxdataLen);
    routeDecoderInputData();
    projectExiConnector_decode_DinExiDocument();
    tcp_rxdataLen = 0; /* mark the input data as "consumed" */
    if (dinDocDec.V2G_Message.Body.ChargeParameterDiscoveryRes_isUsed) {
        // We can have two cases here:
        // (A) The charger needs more time to show the charge parameters.
        // (B) The charger finished to tell the charge parameters.
        if (dinDocDec.V2G_Message.Body.ChargeParameterDiscoveryRes.EVSEProcessing == dinEVSEProcessingType_Finished) {
            publishStatus("ChargeParams discovered");
            addToTrace("Checkpoint550: It is Finished. Will change to state C and send CableCheckReq.");
            // pull the CP line to state C here:
            hardwareInterface_setStateC();
            pev_sendCableCheckReq();
            pev_numberOfCableCheckReq = 1; // This is the first request.
            pev_enterState(PEV_STATE_WaitForCableCheckResponse);
        } else {
            // Not (yet) finished.
            if (pev_numberOfChargeParameterDiscoveryReq>=20) { // approx 20 seconds, should be sufficient for the charger to find its parameters...
                addToTrace("ChargeParameterDiscovery lasted too long. " + String(pev_numberOfChargeParameterDiscoveryReq) + " Giving up.");
                pev_enterState(PEV_STATE_SequenceTimeout);
            } else {
                // Try again.
                pev_numberOfChargeParameterDiscoveryReq += 1; // count the number of tries.
                publishStatus("disc ChargeParams");
                addToTrace("Not (yet) finished. Will again send ChargeParameterDiscoveryReq #" + String(pev_numberOfChargeParameterDiscoveryReq));
                pev_sendChargeParameterDiscoveryReq();
                // we stay in the same state
                pev_enterState(PEV_STATE_WaitForChargeParameterDiscoveryResponse); 
            }
        }
    }
  }
  if (pev_isTooLong()) {
    pev_enterState(PEV_STATE_SequenceTimeout);
  }
}

void stateFunctionWaitForCableCheckResponse(void) {
  uint8_t rc, proc;
  if (pev_cyclesInState<30) { // The first second in the state just do nothing.
    return;
  }
  if (tcp_rxdataLen>V2GTP_HEADER_SIZE) {
    addToTrace("In state WaitForCableCheckResponse, received:");
    showBuffer(tcp_rxdata, tcp_rxdataLen);
    routeDecoderInputData();
    projectExiConnector_decode_DinExiDocument();
    tcp_rxdataLen = 0; /* mark the input data as "consumed" */
    if (dinDocDec.V2G_Message.Body.CableCheckRes_isUsed) {
        rc = dinDocDec.V2G_Message.Body.CableCheckRes.ResponseCode;
        proc = dinDocDec.V2G_Message.Body.CableCheckRes.EVSEProcessing;
        addToTrace("The CableCheck result is " + String(rc) + " " + String(proc));
        // We have two cases here:
        // 1) The charger says "cable check is finished and cable ok", by setting ResponseCode=OK and EVSEProcessing=Finished.
        // 2) Else: The charger says "need more time or cable not ok". In this case, we just run into timeout and start from the beginning.
        if ((proc==dinEVSEProcessingType_Finished) && (rc==dinresponseCodeType_OK)) {
            publishStatus("CbleChck done");
            addToTrace("The EVSE says that the CableCheck is finished and ok.");
            addToTrace("Will send PreChargeReq");
            pev_sendPreChargeReq();
            pev_enterState(PEV_STATE_WaitForPreChargeResponse);
        } else {
            if (pev_numberOfCableCheckReq>30) { // approx 30s should be sufficient for cable check
                addToTrace("CableCheck lasted too long. " + String(pev_numberOfCableCheckReq) + " Giving up.");
                pev_enterState(PEV_STATE_SequenceTimeout);
            } else {    
                // cable check not yet finished or finished with bad result -> try again
                pev_numberOfCableCheckReq += 1;
                publishStatus("CbleChck ongoing", String(hardwareInterface_getInletVoltage()) + "V");
                addToTrace("Will again send CableCheckReq");
                pev_sendCableCheckReq();
                // stay in the same state
                pev_enterState(PEV_STATE_WaitForCableCheckResponse);
            }
        }
    }
  }
  if (pev_isTooLong()) {
    pev_enterState(PEV_STATE_SequenceTimeout);
  }
}

void stateFunctionWaitForPreChargeResponse(void) {
  hardwareInterface_simulatePreCharge();
  if (pev_DelayCycles>0) {
    pev_DelayCycles-=1;
    return;
  }
  if (tcp_rxdataLen>V2GTP_HEADER_SIZE) {
    addToTrace("In state WaitForPreChargeResponse, received:");
    showBuffer(tcp_rxdata, tcp_rxdataLen);
    routeDecoderInputData();
    projectExiConnector_decode_DinExiDocument();
    tcp_rxdataLen = 0; /* mark the input data as "consumed" */
    if (dinDocDec.V2G_Message.Body.PreChargeRes_isUsed) {
        addToTrace("PreCharge aknowledge received.");
        //s = "U_Inlet " + String(hardwareInterface.getInletVoltage()) + "V, "
        //s= s + "U_Accu " + String(hardwareInterface.getAccuVoltage()) + "V"
        //addToTrace(s);
        if (abs(hardwareInterface_getInletVoltage()-hardwareInterface_getAccuVoltage()) < PARAM_U_DELTA_MAX_FOR_END_OF_PRECHARGE) {
            addToTrace("Difference between accu voltage and inlet voltage is small. Sending PowerDeliveryReq.");
            publishStatus("PreCharge done");
            if (isLightBulbDemo) {
                // For light-bulb-demo, nothing to do here.
                addToTrace("This is a light bulb demo. Do not turn-on the relay at end of precharge.");
            } else {
                // In real-world-case, turn the power relay on.
                hardwareInterface_setPowerRelayOn();
            }
            pev_wasPowerDeliveryRequestedOn=1;
            pev_sendPowerDeliveryReq(1); /* 1 is ON */
            pev_enterState(PEV_STATE_WaitForPowerDeliveryResponse);
        } else {
            publishStatus("PreChrge ongoing", String(hardwareInterface_getInletVoltage()) + "V");
            addToTrace("Difference too big. Continuing PreCharge.");
            pev_sendPreChargeReq();
            pev_DelayCycles=15; // wait with the next evaluation approx half a second
        }
    }
  }
  if (pev_isTooLong()) {
    pev_enterState(PEV_STATE_SequenceTimeout);
  }
}

void stateFunctionWaitForPowerDeliveryResponse(void) {
  if (tcp_rxdataLen>V2GTP_HEADER_SIZE) {
    addToTrace("In state WaitForPowerDeliveryRes, received:");
    showBuffer(tcp_rxdata, tcp_rxdataLen);
    routeDecoderInputData();
    projectExiConnector_decode_DinExiDocument();
    tcp_rxdataLen = 0; /* mark the input data as "consumed" */
    if (dinDocDec.V2G_Message.Body.PowerDeliveryRes_isUsed) {
        if (pev_wasPowerDeliveryRequestedOn) {
            publishStatus("PwrDelvy ON success");
            addToTrace("Checkpoint700: Starting the charging loop with CurrentDemandReq");
            pev_sendCurrentDemandReq();
            pev_enterState(PEV_STATE_WaitForCurrentDemandResponse);
        } else {
            /* We requested "OFF". So we turn-off the Relay and continue with the Welding detection. */
            publishStatus("PwrDelvry OFF success");
            addToTrace("Turning off the relay and starting the WeldingDetection");
            hardwareInterface_setPowerRelayOff();
            hardwareInterface_setRelay2Off();
            pev_isBulbOn = 0;
            pev_sendWeldingDetectionReq();
            pev_enterState(PEV_STATE_WaitForWeldingDetectionResponse);
        }
    }
  }
  if (pev_isTooLong()) {
    pev_enterState(PEV_STATE_SequenceTimeout);
  }
}

void stateFunctionWaitForCurrentDemandResponse(void) {
  if (tcp_rxdataLen>V2GTP_HEADER_SIZE) {
    addToTrace("In state WaitForCurrentDemandRes, received:");
    showBuffer(tcp_rxdata, tcp_rxdataLen);
    routeDecoderInputData();
    projectExiConnector_decode_DinExiDocument();
    tcp_rxdataLen = 0; /* mark the input data as "consumed" */
    if (dinDocDec.V2G_Message.Body.CurrentDemandRes_isUsed) {
        /* as long as the accu is not full and no stop-demand from the user, we continue charging */
        if (hardwareInterface_getIsAccuFull() || pev_isUserStopRequest) {
            if (hardwareInterface_getIsAccuFull()) {
                publishStatus("Accu full");
                addToTrace("Accu is full. Sending PowerDeliveryReq Stop.");
            } else {
                publishStatus("User req stop");
                addToTrace("User requested stop. Sending PowerDeliveryReq Stop.");
            }
            pev_wasPowerDeliveryRequestedOn=0;
            pev_sendPowerDeliveryReq(0);
            pev_enterState(PEV_STATE_WaitForPowerDeliveryResponse);
        } else {
            /* continue charging loop */
            hardwareInterface_simulateCharging();
            publishStatus("Charging", String(hardwareInterface_getInletVoltage()) + "V", String(hardwareInterface_getSoc()) + "%");
            pev_sendCurrentDemandReq();
            pev_enterState(PEV_STATE_WaitForCurrentDemandResponse);
        }
    }
  }
  if (isLightBulbDemo) {
      if (pev_cyclesLightBulbDelay<=33*2) {
          pev_cyclesLightBulbDelay+=1;
      } else {
          if (!pev_isBulbOn) {
              addToTrace("This is a light bulb demo. Turning-on the bulb when 2s in the main charging loop.");
              hardwareInterface_setPowerRelayOn();   
              hardwareInterface_setRelay2On();
              pev_isBulbOn = 1;
          }
      }
  }
  if (pev_isTooLong()) {
    pev_enterState(PEV_STATE_SequenceTimeout);
  }
}

void stateFunctionWaitForWeldingDetectionResponse(void) {
  if (tcp_rxdataLen>V2GTP_HEADER_SIZE) {
    addToTrace("In state WaitForWeldingDetectionRes, received:");
    showBuffer(tcp_rxdata, tcp_rxdataLen);
    routeDecoderInputData();
    projectExiConnector_decode_DinExiDocument();
    tcp_rxdataLen = 0; /* mark the input data as "consumed" */
    if (dinDocDec.V2G_Message.Body.WeldingDetectionRes_isUsed) {
        /* todo: add real welding detection here, run in welding detection loop until finished. */
        publishStatus("WldingDet done");
        addToTrace("Sending SessionStopReq");
        projectExiConnector_prepare_DinExiDocument();
        dinDocEnc.V2G_Message.Body.SessionStopReq_isUsed = 1u;
        init_dinSessionStopType(&dinDocEnc.V2G_Message.Body.SessionStopReq);
        /* no other fields are manatory */      
        encodeAndTransmit();
        pev_enterState(PEV_STATE_WaitForSessionStopResponse);
    }
  }
  if (pev_isTooLong()) {
      pev_enterState(PEV_STATE_SequenceTimeout);
  }
}

void stateFunctionWaitForSessionStopResponse(void) {
  if (tcp_rxdataLen>V2GTP_HEADER_SIZE) {
    addToTrace("In state WaitForSessionStopRes, received:");
    showBuffer(tcp_rxdata, tcp_rxdataLen);
    routeDecoderInputData();
    projectExiConnector_decode_DinExiDocument();
    tcp_rxdataLen = 0; /* mark the input data as "consumed" */
    if (dinDocDec.V2G_Message.Body.SessionStopRes_isUsed) {
        // req -508
        // Todo: close the TCP connection here.
        // Todo: Unlock the connector lock.
        publishStatus("Stopped normally");
        hardwareInterface_setStateB();
        addToTrace("Charging is finished");
        pev_enterState(PEV_STATE_ChargingFinished);
    }
  }
  if (pev_isTooLong()) {
    pev_enterState(PEV_STATE_SequenceTimeout);
  }
}

void stateFunctionChargingFinished(void) {
  /* charging is finished. Nothing to do. Just stay here, until we get re-initialized after a new SLAC/SDP. */      
}

void stateFunctionSequenceTimeout(void) {
  // Here we end, if we run into a timeout in the state machine. This is an error case, and
  // we should re-initalize and try again to get a communication.
  // Todo: Maybe we want even inform the pyPlcHomeplug to do a new SLAC.
  // For the moment, we just re-establish the TCP connection.
  publishStatus("ERROR Timeout");
  pevStateMachine_ReInit();
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
    limit = 30*30; // CableCheck may need some time. Wait at least 30s.
  }
  if (pev_state==PEV_STATE_WaitForPreChargeResponse) {
    limit = 30*30; // PreCharge may need some time. Wait at least 30s.
  }
  return (pev_cyclesInState > limit);
}

/******* The statemachine dispatcher *******************/
void pev_runFsm(void) {
 if (connMgr_getConnectionLevel()<CONNLEVEL_80_TCP_RUNNING) {
        /* If we have no TCP to the charger, nothing to do here. Just wait for the link. */
        if (pev_state!=PEV_STATE_NotYetInitialized) pev_enterState(PEV_STATE_NotYetInitialized);   
        return;
 }
 if (connMgr_getConnectionLevel()==CONNLEVEL_80_TCP_RUNNING) {
        /* We have a TCP connection. This is the trigger for us. */
        if (pev_state==PEV_STATE_NotYetInitialized) pev_enterState(PEV_STATE_Connected);   
 }
 switch (pev_state) {
    case PEV_STATE_Connected: stateFunctionConnected();  break;
    case PEV_STATE_WaitForSupportedApplicationProtocolResponse: stateFunctionWaitForSupportedApplicationProtocolResponse(); break;
    case PEV_STATE_WaitForSessionSetupResponse: stateFunctionWaitForSessionSetupResponse(); break;
    case PEV_STATE_WaitForServiceDiscoveryResponse: stateFunctionWaitForServiceDiscoveryResponse(); break;
    case PEV_STATE_WaitForServicePaymentSelectionResponse: stateFunctionWaitForServicePaymentSelectionResponse(); break;
    case PEV_STATE_WaitForContractAuthenticationResponse: stateFunctionWaitForContractAuthenticationResponse(); break;
    case PEV_STATE_WaitForChargeParameterDiscoveryResponse: stateFunctionWaitForChargeParameterDiscoveryResponse(); break;
    case PEV_STATE_WaitForCableCheckResponse: stateFunctionWaitForCableCheckResponse(); break;
    case PEV_STATE_WaitForPreChargeResponse: stateFunctionWaitForPreChargeResponse(); break;
    case PEV_STATE_WaitForPowerDeliveryResponse: stateFunctionWaitForPowerDeliveryResponse(); break;
    case PEV_STATE_WaitForCurrentDemandResponse: stateFunctionWaitForCurrentDemandResponse(); break;
    case PEV_STATE_WaitForWeldingDetectionResponse: stateFunctionWaitForWeldingDetectionResponse(); break;
    case PEV_STATE_WaitForSessionStopResponse: stateFunctionWaitForSessionStopResponse(); break;
    case PEV_STATE_ChargingFinished: stateFunctionChargingFinished(); break;
    case PEV_STATE_SequenceTimeout: stateFunctionSequenceTimeout(); break;
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
