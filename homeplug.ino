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


const uint8_t MAC_BROADCAST[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

char strVersion[200];
uint8_t verLen;
uint8_t sourceMac[6];
uint8_t numberOfSoftwareVersionResponses;

//#define addToTrace(format, ...) log_v(format, ##__VA_ARGS__)
#define addToTrace(format, ...) Serial.println(format)

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

void sendTestFrame(void) {
  composeGetSwReq();
  myEthTransmit();
}

void evaluateGetSwCnf(void) {
        /* The GET_SW confirmation. This contains the software version of the homeplug modem.
        # Reference: see wireshark interpreted frame from TPlink, Ioniq and Alpitronic charger */
        uint8_t i, x;  
        String strMac;    
        addToTrace("received GET_SW.CNF");
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

void evaluateReceivedHomeplugPacket(void) {
  switch (getManagementMessageType()) {
    //case CM_GET_KEY + MMTYPE_CNF:  evaluateGetKeyCnf(); break;   
    //case CM_SLAC_MATCH + MMTYPE_REQ:     evaluateSlacMatchReq(); break;
    //case CM_SLAC_MATCH + MMTYPE_CNF:     evaluateSlacMatchCnf
    //case CM_SLAC_PARAM + MMTYPE_REQ:     evaluateSlacParamReq()
    //case CM_SLAC_PARAM + MMTYPE_CNF:     evaluateSlacParamCnf
    //case CM_MNBC_SOUND + MMTYPE_IND:     evaluateMnbcSoundInd();
    //case CM_ATTEN_CHAR + MMTYPE_IND:     evaluateAttenCharInd();
    //case CM_SET_KEY + MMTYPE_CNF:     evaluateSetKeyCnf();
    case CM_GET_SW + MMTYPE_CNF:     evaluateGetSwCnf(); break;
  }
}
