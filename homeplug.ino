/* Homeplug message handling */

#define CM_SET_KEY  0x6008
#define CM_GET_KEY  0x600C
#define CM_SC_JOIN  0x6010
#define CM_CHAN_EST  0x6014
#define CM_TM_UPDATE  0x6018
#define CM_AMP_MAP  0x601C
#define CM_BRG_INFO  0x6020
#define CM_CONN_NEW  0x6024
#define CM_CONN_REL  0x6028
#define CM_CONN_MOD  0x602C
#define CM_CONN_INFO  0x6030
#define CM_STA_CAP  0x6034
#define CM_NW_INFO  0x6038
#define CM_GET_BEACON  0x603C
#define CM_HFID  0x6040
#define CM_MME_ERROR  0x6044
#define CM_NW_STATS  0x6048
#define CM_SLAC_PARAM  0x6064
#define CM_START_ATTEN_CHAR  0x6068
#define CM_ATTEN_CHAR  0x606C
#define CM_PKCS_CERT  0x6070
#define CM_MNBC_SOUND  0x6074
#define CM_VALIDATE  0x6078
#define CM_SLAC_MATCH  0x607C
#define CM_SLAC_USER_DATA  0x6080
#define CM_ATTEN_PROFILE  0x6084
#define CM_GET_SW  0xA000

#define MMTYPE_REQ  0x0000
#define MMTYPE_CNF  0x0001
#define MMTYPE_IND  0x0002
#define MMTYPE_RSP  0x0003

#define STATE_INITIAL  0
#define STATE_MODEM_SEARCH_ONGOING  1
#define STATE_READY_FOR_SLAC        2
#define STATE_WAITING_FOR_MODEM_RESTARTED  3
#define STATE_WAITING_FOR_SLAC_PARAM_CNF   4
#define STATE_SLAC_PARAM_CNF_RECEIVED      5
#define STATE_BEFORE_START_ATTEN_CHAR      6
#define STATE_SOUNDING                     7
#define STATE_WAIT_FOR_ATTEN_CHAR_IND      8
#define STATE_ATTEN_CHAR_IND_RECEIVED      9
#define STATE_DELAY_BEFORE_MATCH           10
#define STATE_WAITING_FOR_SLAC_MATCH_CNF   11
#define STATE_WAITING_FOR_RESTART2         12
#define STATE_FIND_MODEMS2                 13
#define STATE_WAITING_FOR_SW_VERSIONS      14
#define STATE_READY_FOR_SDP                15
#define STATE_SDP                          16

#define iAmPev 1 /* This project is intended only for PEV mode at the moment. */
#define iAmEvse 0

const uint8_t MAC_BROADCAST[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

char strVersion[200];
uint8_t verLen;
uint8_t sourceMac[6];
uint8_t localModemMac[6];
uint8_t evseMac[6];
uint8_t NID[7];
uint8_t NMK[16];
uint8_t localModemCurrentKey[16];
uint8_t localModemFound;
uint8_t numberOfSoftwareVersionResponses;
uint8_t numberOfFoundModems;
uint8_t pevSequenceState;
uint16_t pevSequenceCyclesInState;
uint16_t pevSequenceDelayCycles;
uint8_t nRemainingStartAttenChar;
uint8_t remainingNumberOfSounds;
uint8_t AttenCharIndNumberOfSounds;
uint8_t SdpRepetitionCounter;
uint8_t isSDPDone;
uint8_t nEvseModemMissingCounter;

void addToTrace(String strTrace) {
  //Serial.println(strTrace);  
  log_v("%s", strTrace.c_str());  
}


/* stubs for later implementation */
#define addressManager_setEvseMac(x)
#define showStatus(x, y)
//#define ipv6_initiateSdpRequest()
#define callbackReadyForTcp(x)
#define addressManagerHasSeccIp() 0

/* Extracting the EtherType from a received message. */
uint16_t getEtherType(uint8_t *messagebufferbytearray) {
  uint16_t etherType=0;
  etherType=messagebufferbytearray[12]*256 + messagebufferbytearray[13];
  return etherType;
}

void fillSourceMac(const uint8_t *mac, uint8_t offset=6) {
 /* at offset 6 in the ethernet frame, we have the source MAC.
    we can give a different offset, to re-use the MAC also in the data area */
  memcpy(&mytransmitbuffer[offset], mac, 6); 
}


void fillDestinationMac(const uint8_t *mac, uint8_t offset=0) {
 /* at offset 0 in the ethernet frame, we have the destination MAC.
    we can give a different offset, to re-use the MAC also in the data area */
  memcpy(&mytransmitbuffer[offset], mac, 6); 
}

void cleanTransmitBuffer(void) {
  /* fill the complete ethernet transmit buffer with 0x00 */
  int i;
  for (i=0; i<MY_ETH_TRANSMIT_BUFFER_LEN; i++) {
    mytransmitbuffer[i]=0;
  }
}

void setNmkAt(uint8_t index) {
  // # sets the Network Membership Key (NMK) at a certain position in the transmit buffer
  uint8_t i;
  for (i=0; i<16; i++) { //for i in range(0, 16):
    mytransmitbuffer[index+i]=NMK[i]; // # NMK
  }
}

void setNidAt(uint8_t index) {
  // copies the network ID (NID, 7 bytes) into the wished position in the transmit buffer
  uint8_t i;
  for (i=0; i<7; i++) { // for i in range(0, 7):
    mytransmitbuffer[index+i]=NID[i];
  }
}

uint16_t getManagementMessageType(void) {
  /* calculates the MMTYPE (base value + lower two bits), see Table 11-2 of homeplug spec */
  return (myreceivebuffer[16]<<8) + myreceivebuffer[15];
}

void composeGetSwReq(void) {
	/* GET_SW.REQ request, as used by the win10 laptop */
    mytransmitbufferLen = 60;
    cleanTransmitBuffer();
    /* Destination MAC */
    fillDestinationMac(MAC_BROADCAST);
    /* Source MAC */
    fillSourceMac(myMAC);
    /* Protocol */
    mytransmitbuffer[12]=0x88; // # Protocol HomeplugAV
    mytransmitbuffer[13]=0xE1; //
    mytransmitbuffer[14]=0x00; // # version
    mytransmitbuffer[15]=0x00; // # GET_SW.REQ
    mytransmitbuffer[16]=0xA0; // # 
    mytransmitbuffer[17]=0x00; // # Vendor OUI
    mytransmitbuffer[18]=0xB0; // # 
    mytransmitbuffer[19]=0x52; // # 
}

void composeSlacParamReq(void) {
	//# SLAC_PARAM request, as it was recorded 2021-12-17 WP charger 2
    mytransmitbufferLen = 60;
    cleanTransmitBuffer();
    //# Destination MAC
    fillDestinationMac(MAC_BROADCAST);
    //# Source MAC
    fillSourceMac(myMAC);
    //# Protocol
    mytransmitbuffer[12]=0x88; // # Protocol HomeplugAV
    mytransmitbuffer[13]=0xE1; //
    mytransmitbuffer[14]=0x01; // # version
    mytransmitbuffer[15]=0x64; // # SLAC_PARAM.REQ
    mytransmitbuffer[16]=0x60; // # 
    mytransmitbuffer[17]=0x00; // # 2 bytes fragmentation information. 0000 means: unfragmented.
    mytransmitbuffer[18]=0x00; // # 
    mytransmitbuffer[19]=0x00; // # 
    mytransmitbuffer[20]=0x00; // #
    fillSourceMac(myMAC, 21); // # 21 to 28: 8 bytes runid. The Ioniq uses the PEV mac plus 00 00.
    mytransmitbuffer[27]=0x00; // # 
    mytransmitbuffer[28]=0x00; // # 
    //# rest is 00
}

void evaluateSlacParamCnf(void) {
    //# As PEV, we receive the first response from the charger.
    addToTrace("[PEVSLAC] received SLAC_PARAM.CNF");
    if (iAmPev) {
        if (pevSequenceState==STATE_WAITING_FOR_SLAC_PARAM_CNF) { // # we were waiting for the SlacParamCnf
            pevSequenceDelayCycles = 4; // # original Ioniq is waiting 200ms
            enterState(STATE_SLAC_PARAM_CNF_RECEIVED); // # enter next state. Will be handled in the cyclic runPevSequencer
		}
	}		
}

void composeStartAttenCharInd(void) {
    //# reference: see wireshark interpreted frame from ioniq
    mytransmitbufferLen = 60;
    cleanTransmitBuffer();
    //# Destination MAC
    fillDestinationMac(MAC_BROADCAST);
    //# Source MAC
    fillSourceMac(myMAC);
    //# Protocol
    mytransmitbuffer[12]=0x88; // # Protocol HomeplugAV
    mytransmitbuffer[13]=0xE1; //
    mytransmitbuffer[14]=0x01; // # version
    mytransmitbuffer[15]=0x6A; // # START_ATTEN_CHAR.IND
    mytransmitbuffer[16]=0x60; // # 
    mytransmitbuffer[17]=0x00; // # 2 bytes fragmentation information. 0000 means: unfragmented.
    mytransmitbuffer[18]=0x00; // # 
    mytransmitbuffer[19]=0x00; // # apptype
    mytransmitbuffer[20]=0x00; // # sectype
    mytransmitbuffer[21]=0x0a; // # number of sounds: 10
    mytransmitbuffer[22]=6; // # timeout N*100ms. Normally 6, means in 600ms all sounds must have been tranmitted.
                                       //# Todo: As long we are a little bit slow, lets give 1000ms instead of 600, so that the
                                       //# charger is able to catch it all.
    mytransmitbuffer[23]=0x01; // # response type 
    fillSourceMac(myMAC, 24); // # 24 to 29: sound_forwarding_sta, MAC of the PEV
    fillSourceMac(myMAC, 30); // # 30 to 37: runid, filled with MAC of PEV and two bytes 00 00
    //# rest is 00
}

void composeNmbcSoundInd(void) {
    //# reference: see wireshark interpreted frame from Ioniq
    uint8_t i;
    mytransmitbufferLen = 71;
    cleanTransmitBuffer();
    //Destination MAC
    fillDestinationMac(MAC_BROADCAST);
    //# Source MAC
    fillSourceMac(myMAC);
    //# Protocol
    mytransmitbuffer[12]=0x88; // # Protocol HomeplugAV
    mytransmitbuffer[13]=0xE1; //
    mytransmitbuffer[14]=0x01; // # version
    mytransmitbuffer[15]=0x76; // # NMBC_SOUND.IND
    mytransmitbuffer[16]=0x60; // # 
    mytransmitbuffer[17]=0x00; // # 2 bytes fragmentation information. 0000 means: unfragmented.
    mytransmitbuffer[18]=0x00; // # 
    mytransmitbuffer[19]=0x00; // # apptype
    mytransmitbuffer[20]=0x00; // # sectype
    mytransmitbuffer[21]=0x00; // # 21 to 37 sender ID, all 00
    mytransmitbuffer[38]=remainingNumberOfSounds; // # countdown. Remaining number of sounds. Starts with 9 and counts down to 0.
    fillSourceMac(myMAC, 39); // # 39 to 46: runid, filled with MAC of PEV and two bytes 00 00
    mytransmitbuffer[47]=0x00; // # 47 to 54: reserved, all 00
    //# 55 to 70: random number. All 0xff in the ioniq message.
    for (i=55; i<71; i++) { // i in range(55, 71):
        mytransmitbuffer[i]=0xFF;
    }
}

void evaluateAttenCharInd(void) {
  uint8_t i;  
  addToTrace("[PEVSLAC] received ATTEN_CHAR.IND");
  if (iAmPev==1) {
        //addToTrace("[PEVSLAC] received AttenCharInd in state " + str(pevSequenceState))
        if (pevSequenceState==STATE_WAIT_FOR_ATTEN_CHAR_IND) { // # we were waiting for the AttenCharInd
            //# todo: Handle the case when we receive multiple responses from different chargers.
            //#       Wait a certain time, and compare the attenuation profiles. Decide for the nearest charger.
            //# Take the MAC of the charger from the frame, and store it for later use.
            for (i=0; i<6; i++) {
                evseMac[i] = myreceivebuffer[6+i]; // # source MAC starts at offset 6
            }
            addressManager_setEvseMac(evseMac);
            AttenCharIndNumberOfSounds = myreceivebuffer[69];
            //addToTrace("[PEVSLAC] number of sounds reported by the EVSE (should be 10): " + str(AttenCharIndNumberOfSounds)) 
            composeAttenCharRsp();
            addToTrace("[PEVSLAC] transmitting ATTEN_CHAR.RSP...");
            myEthTransmit();               
            pevSequenceState=STATE_ATTEN_CHAR_IND_RECEIVED; // # enter next state. Will be handled in the cyclic runPevSequencer
		    }
	}
}

void composeAttenCharRsp(void) {
    //# reference: see wireshark interpreted frame from Ioniq
    mytransmitbufferLen = 70;
    cleanTransmitBuffer();
    //# Destination MAC
    fillDestinationMac(evseMac);
    //# Source MAC
    fillSourceMac(myMAC);
    //# Protocol
    mytransmitbuffer[12]=0x88; // # Protocol HomeplugAV
    mytransmitbuffer[13]=0xE1; //
    mytransmitbuffer[14]=0x01; // # version
    mytransmitbuffer[15]=0x6F; // # ATTEN_CHAR.RSP
    mytransmitbuffer[16]=0x60; // # 
    mytransmitbuffer[17]=0x00; // # 2 bytes fragmentation information. 0000 means: unfragmented.
    mytransmitbuffer[18]=0x00; // # 
    mytransmitbuffer[19]=0x00; // # apptype
    mytransmitbuffer[20]=0x00; // # sectype
    fillSourceMac(myMAC, 21); // # 21 to 26: source MAC
    fillDestinationMac(myMAC, 27); // # 27 to 34: runid. The PEV mac, plus 00 00.
    //# 35 to 51: source_id, all 00
    //# 52 to 68: resp_id, all 00
    //# 69: result. 0 is ok
}
		
void composeSlacMatchReq(void) {
    //# reference: see wireshark interpreted frame from Ioniq
    mytransmitbufferLen = 85;
    cleanTransmitBuffer();
    //# Destination MAC
    fillDestinationMac(evseMac);
    //# Source MAC
    fillSourceMac(myMAC);
    //# Protocol
    mytransmitbuffer[12]=0x88; // # Protocol HomeplugAV
    mytransmitbuffer[13]=0xE1; //
    mytransmitbuffer[14]=0x01; // # version
    mytransmitbuffer[15]=0x7C; // # SLAC_MATCH.REQ
    mytransmitbuffer[16]=0x60; // # 
    mytransmitbuffer[17]=0x00; // # 2 bytes fragmentation information. 0000 means: unfragmented.
    mytransmitbuffer[18]=0x00; // # 
    mytransmitbuffer[19]=0x00; // # apptype
    mytransmitbuffer[20]=0x00; // # sectype
    mytransmitbuffer[21]=0x3E; // # 21 to 22: length
    mytransmitbuffer[22]=0x00; // #
    //# 23 to 39: pev_id, all 00
    fillSourceMac(myMAC, 40); // # 40 to 45: PEV MAC
    //# 46 to 62: evse_id, all 00
    fillDestinationMac(evseMac, 63); // # 63 to 68: EVSE MAC
    fillSourceMac(myMAC, 69); // # 69 to 76: runid. The PEV mac, plus 00 00.
    //# 77 to 84: reserved, all 00        
}

void evaluateSlacMatchCnf(void) {
    String s;
    uint8_t i;
    //# The SLAC_MATCH.CNF contains the NMK and the NID.
    //# We extract this information, so that we can use it for the CM_SET_KEY afterwards.
    //# References: https://github.com/qca/open-plc-utils/blob/master/slac/evse_cm_slac_match.c
    //# 2021-12-16_HPC_säule1_full_slac.pcapng
    if (iAmEvse==1) {
            //# If we are EVSE, nothing to do. We have sent the match.CNF by our own.
            //# The SET_KEY was already done at startup.
    } else {
            addToTrace("[PEVSLAC] received SLAC_MATCH.CNF");
            s = "";
            for (i=0; i<7; i++) { // # NID has 7 bytes
                NID[i] = myreceivebuffer[85+i];
                s=s+String(NID[i], HEX)+ " ";
            }    
            addToTrace("[PEVSLAC] From SlacMatchCnf, got network ID (NID) " + s);      
            s = "";
            for (i=0; i<16; i++) {
                NMK[i] = myreceivebuffer[93+i];
                s=s+String(NMK[i], HEX)+ " ";
            }                
            addToTrace("[PEVSLAC] From SlacMatchCnf, got network membership key (NMK) " + s);
            //# use the extracted NMK and NID to set the key in the adaptor:
            composeSetKey();
            addToTrace("[PEVSLAC] transmitting CM_SET_KEY.REQ");
            myEthTransmit();
            if (pevSequenceState==STATE_WAITING_FOR_SLAC_MATCH_CNF) { //# we were waiting for finishing the SLAC_MATCH.CNF and SET_KEY.REQ
                enterState(STATE_WAITING_FOR_RESTART2);
            }
		}
}

void composeSetKey(void) {
	//# CM_SET_KEY.REQ request
    //# From example trace from catphish https://openinverter.org/forum/viewtopic.php?p=40558&sid=9c23d8c3842e95c4cf42173996803241#p40558
    //# Table 11-88 in the homeplug_av21_specification_final_public.pdf
    mytransmitbufferLen = 60;
    cleanTransmitBuffer();
    //# Destination MAC
    //#fillDestinationMac(MAC_DEVOLO_26)
    //#fillDestinationMac(MAC_TPLINK_E4)
    fillDestinationMac(MAC_BROADCAST);
    //# Source MAC
    fillSourceMac(myMAC);
    //# Protocol
    mytransmitbuffer[12]=0x88; // # Protocol HomeplugAV
    mytransmitbuffer[13]=0xE1; //
    mytransmitbuffer[14]=0x01; // # version
    mytransmitbuffer[15]=0x08; // # CM_SET_KEY.REQ
    mytransmitbuffer[16]=0x60; // # 
    mytransmitbuffer[17]=0x00; // # frag_index
    mytransmitbuffer[18]=0x00; // # frag_seqnum
    mytransmitbuffer[19]=0x01; // # 0 key info type

    mytransmitbuffer[20]=0xaa; // # 1 my nonce
    mytransmitbuffer[21]=0xaa; // # 2
    mytransmitbuffer[22]=0xaa; // # 3
    mytransmitbuffer[23]=0xaa; // # 4

    mytransmitbuffer[24]=0x00; // # 5 your nonce
    mytransmitbuffer[25]=0x00; // # 6
    mytransmitbuffer[26]=0x00; // # 7
    mytransmitbuffer[27]=0x00; // # 8
        
    mytransmitbuffer[28]=0x04; // # 9 nw info pid
        
    mytransmitbuffer[29]=0x00; // # 10 info prn
    mytransmitbuffer[30]=0x00; // # 11
    mytransmitbuffer[31]=0x00; // # 12 pmn
    mytransmitbuffer[32]=0x00; // # 13 cco cap
    setNidAt(33); // # 14-20 nid  7 bytes from 33 to 39
                  //          # Network ID to be associated with the key distributed herein.
                  //          # The 54 LSBs of this field contain the NID (refer to Section 3.4.3.1). The
                  //          # two MSBs shall be set to 0b00.
    mytransmitbuffer[40]=0x01; // # 21 peks (payload encryption key select) Table 11-83. 01 is NMK. We had 02 here, why???
                               //# with 0x0F we could choose "no key, payload is sent in the clear"
    setNmkAt(41); 
    #define variation 0
    mytransmitbuffer[41]+=variation; // # to try different NMKs
    //# and three remaining zeros
}

void evaluateSetKeyCnf(void) {
    //# The Setkey confirmation
    uint8_t result;
    //# In spec, the result 0 means "success". But in reality, the 0 means: did not work. When it works,
    //# then the LEDs are blinking (device is restarting), and the response is 1.
    addToTrace("[PEVSLAC] received SET_KEY.CNF");
    result = myreceivebuffer[19];
    if (result == 0) {
        addToTrace("[PEVSLAC] SetKeyCnf says 0, this would be a bad sign for local modem, but normal for remote.");
    } else {
        addToTrace("[PEVSLAC] SetKeyCnf says " + String(result) + ", this is formally 'rejected', but indeed ok.");
	  }
}

void composeGetKey(void) {
		//# CM_GET_KEY.REQ request
    //    # from https://github.com/uhi22/plctool2/blob/master/listen_to_eth.c
    //    # and homeplug_av21_specification_final_public.pdf
    mytransmitbufferLen = 60;
    cleanTransmitBuffer();
    //# Destination MAC
    fillDestinationMac(MAC_BROADCAST);
    //# Source MAC
    fillSourceMac(myMAC);
    //# Protocol
    mytransmitbuffer[12]=0x88; // # Protocol HomeplugAV
    mytransmitbuffer[13]=0xE1;
    mytransmitbuffer[14]=0x01; // # version
    mytransmitbuffer[15]=0x0C; // # CM_GET_KEY.REQ https://github.com/uhi22/plctool2/blob/master/plc_homeplug.h
    mytransmitbuffer[16]=0x60; // #
    mytransmitbuffer[17]=0x00; // # 2 bytes fragmentation information. 0000 means: unfragmented.
    mytransmitbuffer[18]=0x00; // #       
    mytransmitbuffer[19]=0x00; // # 0 Request Type 0=direct
    mytransmitbuffer[20]=0x01; // # 1 RequestedKeyType only "NMK" is permitted over the H1 interface.
                               //        #    value see HomeplugAV2.1 spec table 11-89. 1 means AES-128.
                                       
    setNidAt(21); //# NID starts here (table 11-91 Homeplug spec is wrong. Verified by accepted command.)
    mytransmitbuffer[28]=0xaa; // # 10-13 mynonce. The position at 28 is verified by the response of the devolo.
    mytransmitbuffer[29]=0xaa; // # 
    mytransmitbuffer[30]=0xaa; // # 
    mytransmitbuffer[31]=0xaa; // # 
    mytransmitbuffer[32]=0x04; // # 14 PID. According to  ISO15118-3 fix value 4, "HLE protocol"
    mytransmitbuffer[33]=0x00; // # 15-16 PRN Protocol run number
    mytransmitbuffer[34]=0x00; // # 
    mytransmitbuffer[35]=0x00; // # 17 PMN Protocol message number
}

void evaluateGetKeyCnf(void) {
	uint8_t i;
	uint8_t result;
	String strMac;
	String strResult;
	String strNID;
	String s;
    addToTrace("received GET_KEY.CNF");
    numberOfFoundModems += 1;
    for (i=0; i<6; i++) {
        sourceMac[i] = myreceivebuffer[6+i];
    }
    strMac = String(sourceMac[0], HEX) + ":" + String(sourceMac[1], HEX) + ":" + String(sourceMac[2], HEX) + ":" 
     + String(sourceMac[3], HEX) + ":" + String(sourceMac[4], HEX) + ":" + String(sourceMac[5], HEX);
    result = myreceivebuffer[19]; // # 0 in case of success
    if (result==0) {
            strResult="(OK)";
    } else {
            strResult="(NOK)";
	  }
    addToTrace("Modem #" + String(numberOfFoundModems) + " has " + strMac + " and result code is " + String(result) + strResult);
	  if (numberOfFoundModems>1) {
		  addToTrace("Info: NOK is normal for remote modems.");
	  }
            
    //    # We observed the following cases:
    //    # (A) Result=1 (NOK), NID all 00, key all 00: We requested the key with the wrong NID.
    //    # (B) Result=0 (OK), NID all 00, key non-zero: We used the correct NID for the request.
    //    #            It is the local TPlink adaptor. A fresh started non-coordinator, like the PEV side.
    //    # (C) Result=0 (OK), NID non-zero, key non-zero: We used the correct NID for the request.
    //    #            It is the local TPlink adaptor.
    //    # (D) Result=1 (NOK), NID non-zero, key all 00: It was a remote device. They are rejecting the GET_KEY.
    if (result==0) {
      //# The ok case is for sure the local modem. Let's store its data.
      memcpy(localModemMac, sourceMac, 6);
      s="";
			for (i=0; i<16; i++) { // NMK has 16 bytes
                localModemCurrentKey[i] = myreceivebuffer[41+i];
                s=s+String(localModemCurrentKey[i], HEX)+ " ";
			}
      addToTrace("The local modem has key " + s);
      //if (localModemCurrentKey == bytearray(NMKdevelopment)):
      //    addToTrace("This is the developer NMK.")
      //    isDeveloperLocalKey = 1
      //else:
      //    addToTrace("This is NOT the developer NMK.")            
      localModemFound=1;
	  }
    strNID = "";
    //    # The getkey response contains the Network ID (NID), even if the request was rejected. We store the NID,
    //    # to have it available for the next request. Use case: A fresh started, unconnected non-Coordinator
    //    # modem has the default-NID all 00. On the other hand, a fresh started coordinator has the
    //    # NID which he was configured before. We want to be able to cover both cases. That's why we
    //    # ask GET_KEY, it will tell the NID (even if response code is 1 (NOK), and we will use this
    //    # received NID for the next request. This will be ansered positive (for the local modem).
    for (i=0; i<7; i++) { // # NID has 7 bytes
        NID[i] = myreceivebuffer[29+i];
        strNID=strNID+String(NID[i], HEX)+ " ";
    }            
    addToTrace("From GetKeyCnf, got network ID (NID) " + strNID);
}

void sendTestFrame(void) {
  composeGetSwReq();
  myEthTransmit();
}

void evaluateGetSwCnf(void) {
    /* The GET_SW confirmation. This contains the software version of the homeplug modem.
    # Reference: see wireshark interpreted frame from TPlink, Ioniq and Alpitronic charger */
    uint8_t i, x;  
    String strMac;    
    addToTrace("[PEVSLAC] received GET_SW.CNF");
    numberOfSoftwareVersionResponses+=1;
    for (i=0; i<6; i++) {
        sourceMac[i] = myreceivebuffer[6+i];
    }
    strMac = String(sourceMac[0], HEX) + ":" + String(sourceMac[1], HEX) + ":" + String(sourceMac[2], HEX) + ":" 
     + String(sourceMac[3], HEX) + ":" + String(sourceMac[4], HEX) + ":" + String(sourceMac[5], HEX);
    verLen = myreceivebuffer[22];
    if ((verLen>0) && (verLen<0x30)) {
      for (i=0; i<verLen; i++) {
            x = myreceivebuffer[23+i];
            if (x<0x20) { x=0x20; } /* make unprintable character to space. */
            strVersion[i]=x;
      }
      strVersion[i] = 0; 
      //addToTrace(strMac);
      addToTrace("For " + strMac + " the software version is " + String(strVersion)); 
    }        
}

uint8_t isEvseModemFound(void) {
        //#return 0 # todo: look whether the MAC of the EVSE modem is in the list of detected modems
        return numberOfFoundModems>1;
}

void enterState(int n) {
    addToTrace("[PEVSLAC] from " + String(pevSequenceState) + " entering " + String(n));
    pevSequenceState = n;
    pevSequenceCyclesInState = 0;
}

int isTooLong(void) {
    //# The timeout handling function.
    return (pevSequenceCyclesInState > 500);
}

void runPevSequencer(void) {
    // # in PevMode, check whether homeplug modem is connected, run the SLAC and SDP
    pevSequenceCyclesInState+=1;
    if (pevSequenceState == STATE_INITIAL)   {
        /* We assume that the modem is present, and go directly into SLAC, without modem search. */
        enterState(STATE_READY_FOR_SLAC);
        return;
    } 
    if (pevSequenceState==STATE_READY_FOR_SLAC) {
            showStatus("Starting SLAC", "pevState");
            addToTrace("[PEVSLAC] Sending SLAC_PARAM.REQ...");
            composeSlacParamReq();
            myEthTransmit();                
            enterState(STATE_WAITING_FOR_SLAC_PARAM_CNF);
            return;
	  }
    if (pevSequenceState==STATE_WAITING_FOR_SLAC_PARAM_CNF) { // # Waiting for slac_param confirmation.
            if (pevSequenceCyclesInState>=33) {
                // # No response for 1s, this is an error.
                addToTrace("[PEVSLAC] Timeout while waiting for SLAC_PARAM.CNF");
                enterState(STATE_INITIAL);
			      }
            //# (the normal state transition is done in the reception handler)
            return;
	  }
    if (pevSequenceState==STATE_SLAC_PARAM_CNF_RECEIVED) { // # slac_param confirmation was received.
            pevSequenceDelayCycles = 1; // # 1*30=30ms as preparation for the next state.
                                            // # Between the SLAC_PARAM.CNF and the first START_ATTEN_CHAR.IND the Ioniq waits 100ms.
                                            // # The allowed time TP_match_sequence is 0 to 100ms.
                                            // # Alpitronic and ABB chargers are more tolerant, they worked with a delay of approx
                                            // # 250ms. In contrast, Supercharger and Compleo do not respond anymore if we
                                            // # wait so long.
            nRemainingStartAttenChar = 3; // # There shall be 3 START_ATTEN_CHAR messages.
            enterState(STATE_BEFORE_START_ATTEN_CHAR);
            return;
	  }
    if (pevSequenceState==STATE_BEFORE_START_ATTEN_CHAR) { // # received SLAC_PARAM.CNF. Multiple transmissions of START_ATTEN_CHAR.                
            if (pevSequenceDelayCycles>0) {
                pevSequenceDelayCycles-=1;
                return;
			      }
            //# The delay time is over. Let's transmit.
            if (nRemainingStartAttenChar>0) {
                nRemainingStartAttenChar-=1;
                composeStartAttenCharInd();
                addToTrace("[PEVSLAC] transmitting START_ATTEN_CHAR.IND...");
                myEthTransmit();      
                pevSequenceDelayCycles = 0; // # original from ioniq is 20ms between the START_ATTEN_CHAR. Shall be 20ms to 50ms. So we set to 0 and the normal 30ms call cycle is perfect.
                return;
            } else {
                //# all three START_ATTEN_CHAR.IND are finished. Now we send 10 MNBC_SOUND.IND
                pevSequenceDelayCycles = 0; // # original from ioniq is 40ms after the last START_ATTEN_CHAR.IND.
                                            //  # Shall be 20ms to 50ms. So we set to 0 and the normal 30ms call cycle is perfect.
                remainingNumberOfSounds = 10; // # We shall transmit 10 sound messages.
                enterState(STATE_SOUNDING);
			      }
            return;
	  }
    if (pevSequenceState==STATE_SOUNDING) { // # Multiple transmissions of MNBC_SOUND.IND.                
            if (pevSequenceDelayCycles>0) {
                pevSequenceDelayCycles-=1;
                return;
			      }
            if (remainingNumberOfSounds>0) {
                remainingNumberOfSounds-=1;
                composeNmbcSoundInd();
                addToTrace("[PEVSLAC] transmitting MNBC_SOUND.IND..."); // # original from ioniq is 40ms after the last START_ATTEN_CHAR.IND
                myEthTransmit();
                if (remainingNumberOfSounds==0) {
                    enterState(STATE_WAIT_FOR_ATTEN_CHAR_IND); // # move fast to the next state, so that a fast response is catched in the correct state
				        }
                pevSequenceDelayCycles = 0; // # original from ioniq is 20ms between the messages.
                                            // # Shall be 20ms to 50ms. So we set to 0 and the normal 30ms call cycle is perfect.
                return;                
            }            
	  }
    if (pevSequenceState==STATE_WAIT_FOR_ATTEN_CHAR_IND) { // # waiting for ATTEN_CHAR.IND
            //# todo: it is possible that we receive this message from multiple chargers. We need
            //# to select the charger with the loudest reported signals.
            if (isTooLong()) {
                enterState(STATE_INITIAL);
			      }
            return;
            //#(the normal state transition is done in the reception handler)
	  }
    if (pevSequenceState==STATE_ATTEN_CHAR_IND_RECEIVED) { //  # ATTEN_CHAR.IND was received and the
                                                                //    # nearest charger decided and the 
                                                                //    # ATTEN_CHAR.RSP was sent.
            enterState(STATE_DELAY_BEFORE_MATCH);
            pevSequenceDelayCycles = 30; // # original from ioniq is 860ms to 980ms from ATTEN_CHAR.RSP to SLAC_MATCH.REQ
            return;
	  }
    if (pevSequenceState==STATE_DELAY_BEFORE_MATCH) { // # Waiting time before SLAC_MATCH.REQ
            if (pevSequenceDelayCycles>0) {
                pevSequenceDelayCycles-=1;
                return;
			      }
            composeSlacMatchReq();
            showStatus("SLAC match", "pevState");
            addToTrace("[PEVSLAC] transmitting SLAC_MATCH.REQ...");
            myEthTransmit();
            enterState(STATE_WAITING_FOR_SLAC_MATCH_CNF);
            return;
	  }
    if (pevSequenceState==STATE_WAITING_FOR_SLAC_MATCH_CNF) { // # waiting for SLAC_MATCH.CNF
            if (isTooLong()) {
                enterState(STATE_INITIAL);
                return;
			      }
            pevSequenceDelayCycles = 100; // # 3s reset wait time (may be a little bit too short, need a retry)
            //# (the normal state transition is done in the receive handler of SLAC_MATCH.CNF,
            //# including the transmission of SET_KEY.REQ)
            return;
	  }
    if (pevSequenceState==STATE_WAITING_FOR_RESTART2) { //  # SLAC is finished, SET_KEY.REQ was 
                                                        //         # transmitted. The homeplug modem makes
                                                        //         # the reset and we need to wait until it
                                                        //         # is up with the new key.
            if (pevSequenceDelayCycles>0) {
                pevSequenceDelayCycles-=1;
                return;
            }
            addToTrace("[PEVSLAC] Checking whether the pairing worked, by GET_KEY.REQ...");
            numberOfFoundModems = 0; // # reset the number, we want to count the modems newly.
            composeGetKey();
            myEthTransmit();                
            enterState(STATE_FIND_MODEMS2);
            return;
    }
    if (pevSequenceState==STATE_FIND_MODEMS2) { // # Waiting for the modems to answer.
            if (pevSequenceCyclesInState>=10) { //
                //# It was sufficient time to get the answers from the modems.
                addToTrace("[PEVSLAC] It was sufficient time to get the answers from the modems.");
                //# Let's see what we received.
                if (!isEvseModemFound()) {
                    nEvseModemMissingCounter+=1;
                    addToTrace("[PEVSLAC] No EVSE seen (yet). Still waiting for it.");
                    //# At the Alpitronic we measured, that it takes 7s between the SlacMatchResponse and
                    //# the chargers modem reacts to GetKeyRequest. So we should wait here at least 10s.
                    if (nEvseModemMissingCounter>10) {
                            // # We lost the connection to the EVSE modem. Back to the beginning.
                            addToTrace("[PEVSLAC] We lost the connection to the EVSE modem. Back to the beginning.");
                            enterState(STATE_INITIAL);
                            return;
                    }
                    // # The EVSE modem is (shortly) not seen. Ask again.
                    pevSequenceDelayCycles=30;
                    enterState(STATE_WAITING_FOR_RESTART2);
                    return;
                }
                //# The EVSE modem is present (or we are simulating)
                addToTrace("[PEVSLAC] EVSE is up, pairing successful.");
                nEvseModemMissingCounter=0;
                pevSequenceDelayCycles=0;
                composeGetSwReq();
                addToTrace("[PEVSLAC] Requesting SW versions from the modems...");
                myEthTransmit();
                enterState(STATE_WAITING_FOR_SW_VERSIONS);
            }                
            return;
        }        
        if (pevSequenceState==STATE_WAITING_FOR_SW_VERSIONS) {
            if (pevSequenceCyclesInState>=2) { // # 2 cycles = 60ms are more than sufficient.
                //# Measured: The local modem answers in less than 1ms. The remote modem in ~5ms.
                //# It was sufficient time to get the answers from the modems.
                addToTrace("[PEVSLAC] It was sufficient time to get the SW versions from the modems.");
                enterState(STATE_READY_FOR_SDP);
            }    
            return;
        }
        if (pevSequenceState==STATE_READY_FOR_SDP) {
            //# The AVLN is established, we have at least two modems in the network.
            //# If we did not SDP up to now, let's do it.
            if (isSDPDone) {
                //# SDP is already done. No need to do it again. We are finished for the normal case.
                //# But we want to check whether the connection is still alive, so we start the
                //# modem-search from time to time.
                pevSequenceDelayCycles = 300; // e.g. 10s
                enterState(STATE_WAITING_FOR_RESTART2);
                return;
            }                
            // SDP was not done yet. Now we start it.
            showStatus("SDP ongoing", "pevState");
            addToTrace("[PEVSLAC] SDP was not done yet. Now we start it.");
            // Next step is to discover the chargers communication controller (SECC) using discovery protocol (SDP).
            pevSequenceDelayCycles=0;
            SdpRepetitionCounter = 50; // prepare the number of retries for the SDP. The more the better.
            enterState(STATE_SDP);
            return;
        }        
        if (pevSequenceState==STATE_SDP) { // SDP request transmission and waiting for SDP response.
            if (addressManagerHasSeccIp()) {
                // we received an SDP response, and can start the high-level communication
                showStatus("SDP finished", "pevState");
                addToTrace("[PEVSLAC] Now we know the chargers IP.");
                isSDPDone = 1;
                callbackReadyForTcp(1);
                // Continue with checking the connection, for the case somebody pulls the plug.
                pevSequenceDelayCycles = 300; // e.g. 10s
                enterState(STATE_WAITING_FOR_RESTART2);
                return;
            }
            if (pevSequenceDelayCycles>0) {
                // just waiting until next action
                pevSequenceDelayCycles-=1;
                return;
            }                
            if (SdpRepetitionCounter>0) {
                //# Reference: The Ioniq waits 4.1s from the slac_match.cnf to the SDP request.
                //# Here we send the SdpRequest. Maybe too early, but we will retry if there is no response.
                ipv6_initiateSdpRequest();
                SdpRepetitionCounter-=1;
                pevSequenceDelayCycles = 15; // e.g. half-a-second delay until re-try of the SDP
                enterState(STATE_SDP); // stick in the same state
                return;
            }
            // All repetitions are over, no SDP response was seen. Back to the beginning.    
            addToTrace("[PEVSLAC] ERROR: Did not receive SDP response. Starting from the beginning.");
            enterState(STATE_INITIAL);
            return;
        }
        //# invalid state is reached. As robustness measure, go to initial state.
        enterState(STATE_INITIAL);
    
}

void evaluateReceivedHomeplugPacket(void) {
  switch (getManagementMessageType()) {
    case CM_GET_KEY + MMTYPE_CNF:  evaluateGetKeyCnf(); break;   
    //case CM_SLAC_MATCH + MMTYPE_REQ:     evaluateSlacMatchReq(); break;
    case CM_SLAC_MATCH + MMTYPE_CNF:     evaluateSlacMatchCnf(); break;
    //case CM_SLAC_PARAM + MMTYPE_REQ:     evaluateSlacParamReq()
    case CM_SLAC_PARAM + MMTYPE_CNF:     evaluateSlacParamCnf(); break;
    //case CM_MNBC_SOUND + MMTYPE_IND:     evaluateMnbcSoundInd();
    case CM_ATTEN_CHAR + MMTYPE_IND:     evaluateAttenCharInd(); break;
    case CM_SET_KEY + MMTYPE_CNF:     evaluateSetKeyCnf(); break;
    case CM_GET_SW + MMTYPE_CNF:     evaluateGetSwCnf(); break;
  }
}

void homeplugInit(void) {
  pevSequenceState = STATE_READY_FOR_SLAC;
  pevSequenceCyclesInState = 0;
  pevSequenceDelayCycles = 0;
  numberOfSoftwareVersionResponses = 0;
  numberOfFoundModems = 0;
}
