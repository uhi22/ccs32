

const uint8_t broadcastIPv6[16] = { 0xff, 0x02, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};
/* our link-local IPv6 address. Todo: do we need to calculate this from the MAC? Or just use a "random"? */
/* For the moment, just use the address from the Win10 notebook, and change the last byte from 0x0e to 0x1e. */
const uint8_t EvccIp[16] = {0xfe, 0x80, 0, 0, 0, 0, 0, 0, 0xc6, 0x90, 0x83, 0xf3, 0xfb, 0xcb, 0x98, 0x1e};
uint8_t SeccIp[16]; /* the IP address of the charger */
uint16_t seccTcpPort; /* the port number of the charger */
uint8_t sourceIp[16];
uint16_t evccPort=59218; /* some "random port" */
uint16_t sourceport;
uint16_t destinationport;
uint16_t udplen;
uint16_t udpsum;

#define NEXT_UDP 0x11 /* next protocol is UDP */

#define UDP_PAYLOAD_LEN 100
uint8_t udpPayload[UDP_PAYLOAD_LEN];
uint16_t udpPayloadLen;

#define V2G_FRAME_LEN 100
uint8_t v2gtpFrameLen;
uint8_t v2gtpFrame[V2G_FRAME_LEN];

#define UDP_REQUEST_LEN 100
uint8_t UdpRequestLen;
uint8_t UdpRequest[UDP_REQUEST_LEN];

#define IP_REQUEST_LEN 100
uint8_t IpRequestLen;
uint8_t IpRequest[IP_REQUEST_LEN];


void evaluateUdpPayload(void) {
  uint16_t v2gptPayloadType;
  uint32_t v2gptPayloadLen;
  uint8_t i;
  if ((destinationport == 15118) or (sourceport == 15118)) { // port for the SECC
    if ((udpPayload[0]==0x01) and (udpPayload[1]==0xFE)) { //# protocol version 1 and inverted
                //# it is a V2GTP message                
                //showAsHex(udpPayload, "V2GTP ")
                v2gptPayloadType = udpPayload[2] * 256 + udpPayload[3];
                //# 0x8001 EXI encoded V2G message (Will NOT come with UDP. Will come with TCP.)
                //# 0x9000 SDP request message (SECC Discovery)
                //# 0x9001 SDP response message (SECC response to the EVCC)
                if (v2gptPayloadType == 0x9001) {
                    //# it is a SDP response from the charger to the car
                    addToTrace("it is a SDP response from the charger to the car");
                    v2gptPayloadLen = (((uint32_t)udpPayload[4])<<24)  + 
                                      (((uint32_t)udpPayload[5])<<16) +
                                      (((uint32_t)udpPayload[6])<<8) +
                                      udpPayload[7];
                    if (v2gptPayloadLen == 20) {
                            //# 20 is the only valid length for a SDP response.
                            addToTrace("[PEV] Received SDP response");
                            //# at byte 8 of the UDP payload starts the IPv6 address of the charger.
                            for (i=0; i<16; i++) {
                                SeccIp[i] = udpPayload[8+i]; // 16 bytes IP address of the charger
                            }
                            //# Extract the TCP port, on which the charger will listen:
                            seccTcpPort = (((uint16_t)(udpPayload[8+16]))<<8) + udpPayload[8+16+1];
                            isSDPDone = 1;
                    }
                } else {    
                  addToTrace("v2gptPayloadType " + String(v2gptPayloadType, HEX) + " not supported");
                }                  
    }
  }                
}

void ipv6_evaluateReceivedPacket(void) {
  //# The evaluation function for received ipv6 packages.
  uint16_t nextheader; 
  uint16_t i; 
  if (myreceivebufferLen>60) {
            //# extract the source ipv6 address
            memcpy(sourceIp, &myreceivebuffer[22], 16);
            nextheader = myreceivebuffer[20];
            if (nextheader == 0x11) { //  it is an UDP frame
                sourceport = myreceivebuffer[54] * 256 + myreceivebuffer[55];
                destinationport = myreceivebuffer[56] * 256 + myreceivebuffer[57];
                udplen = myreceivebuffer[58] * 256 + myreceivebuffer[59];
                udpsum = myreceivebuffer[60] * 256 + myreceivebuffer[61];
                //# udplen is including 8 bytes header at the begin
                if (udplen>8) {
                    udpPayloadLen = udplen-8;
                    //# print("udplen=" + str(udplen))
                    //# print("myreceivebuffer len=" + str(len(myreceivebuffer)))
                    for (i=0; i<udplen-8; i++) {
                        //#print("index " + str(i) + " " + hex(myreceivebuffer[62+i]))
                        udpPayload[i] = myreceivebuffer[62+i];
                    }
                    evaluateUdpPayload();
                }                      
            }
            if (nextheader == 0x06) { // # it is an TCP frame
                // evaluateTcpPacket();
            }
  }
}

void ipv6_initiateSdpRequest(void) {
            //# We are the car. We want to find out the IPv6 address of the charger. We
            //# send a SECC Discovery Request.
            //# The payload is just two bytes: 10 00.
            //# First step is, to pack this payload into a V2GTP frame.
            addToTrace("[PEV] initiating SDP request");
            v2gtpFrameLen = 8 + 2; // # 8 byte header plus 2 bytes payload
            v2gtpFrame[0] = 0x01; // # version
            v2gtpFrame[1] = 0xFE; // # version inverted
            v2gtpFrame[2] = 0x90; // # 9000 means SDP request message
            v2gtpFrame[3] = 0x00;
            v2gtpFrame[4] = 0x00;
            v2gtpFrame[5] = 0x00;
            v2gtpFrame[6] = 0x00;
            v2gtpFrame[7] = 0x02; // # payload size
            v2gtpFrame[8] = 0x10; // # payload
            v2gtpFrame[9] = 0x00; // # payload
            //# Second step: pack this into an UDP frame.
            ipv6_packRequestIntoUdp();            
}

void ipv6_packRequestIntoUdp(void) {
        //# embeds the (SDP) request into the lower-layer-protocol: UDP
        //# Reference: wireshark trace of the ioniq car
        uint8_t i;
        uint16_t lenInclChecksum;
        uint16_t checksum;
        UdpRequestLen = v2gtpFrameLen + 8; // # UDP header needs 8 bytes:
                                           //           #   2 bytes source port
                                           //           #   2 bytes destination port
                                           //           #   2 bytes length (incl checksum)
                                           //           #   2 bytes checksum
        UdpRequest[0] = evccPort >> 8;
        UdpRequest[1] = evccPort  & 0xFF;
        UdpRequest[2] = 15118 >> 8;
        UdpRequest[3] = 15118 & 0xFF;
        
        lenInclChecksum = UdpRequestLen;
        UdpRequest[4] = lenInclChecksum >> 8;
        UdpRequest[5] = lenInclChecksum & 0xFF;
        // checksum will be calculated afterwards
        UdpRequest[6] = 0;
        UdpRequest[7] = 0;
        for (i=0; i<v2gtpFrameLen; i++) {
            UdpRequest[8+i] = v2gtpFrame[i];
        }
        //#showAsHex(UdpRequest, "UDP request ")
        //broadcastIPv6 = [ 0xff, 0x02, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1]
        //# The content of buffer is ready. We can calculate the checksum. see https://en.wikipedia.org/wiki/User_Datagram_Protocol
        checksum = calculateUdpAndTcpChecksumForIPv6(UdpRequest, UdpRequestLen, EvccIp, broadcastIPv6, NEXT_UDP); 
        UdpRequest[6] = checksum >> 8;
        UdpRequest[7] = checksum & 0xFF;        
        ipv6_packRequestIntoIp();
}
        
void ipv6_packRequestIntoIp(void) {
        // # embeds the (SDP) request into the lower-layer-protocol: IP, Ethernet
        uint8_t i;
        uint16_t plen;
        IpRequestLen = UdpRequestLen + 8 + 16 + 16; // # IP6 header needs 40 bytes:
                                                    //  #   4 bytes traffic class, flow
                                                    //  #   2 bytes destination port
                                                    //  #   2 bytes length (incl checksum)
                                                    //  #   2 bytes checksum
        IpRequest[0] = 0x60; // # traffic class, flow
        IpRequest[1] = 0; 
        IpRequest[2] = 0;
        IpRequest[3] = 0;
        plen = UdpRequestLen; // length of the payload. Without headers.
        IpRequest[4] = plen >> 8;
        IpRequest[5] = plen & 0xFF;
        IpRequest[6] = 0x11; // next level protocol, 0x11 = UDP in this case
        IpRequest[7] = 0x0A; // hop limit
        // We are the PEV. So the EvccIp is our own link-local IP address.
        for (i=0; i<16; i++) {
            IpRequest[8+i] = EvccIp[i]; // source IP address
        }            
        for (i=0; i<16; i++) {
            IpRequest[24+i] = broadcastIPv6[i]; // destination IP address
        }
        for (i=0; i<UdpRequestLen; i++) {
            IpRequest[40+i] = UdpRequest[i];
        }            
        //#showAsHex(IpRequest, "IpRequest ")
        ipv6_packRequestIntoEthernet();
}

void ipv6_packRequestIntoEthernet(void) {
        //# packs the IP packet into an ethernet packet
        uint8_t i;        
        mytransmitbufferLen = IpRequestLen + 6 + 6 + 2; // # Ethernet header needs 14 bytes:
                                                       // #  6 bytes destination MAC
                                                       // #  6 bytes source MAC
                                                       // #  2 bytes EtherType
        //# fill the destination MAC with the IPv6 multicast
        mytransmitbuffer[0] = 0x33;
        mytransmitbuffer[1] = 0x33;
        mytransmitbuffer[2] = 0x00;
        mytransmitbuffer[3] = 0x00;
        mytransmitbuffer[4] = 0x00;
        mytransmitbuffer[5] = 0x01;
        fillSourceMac(myMAC); // bytes 6 to 11 are the source MAC
        mytransmitbuffer[12] = 0x86; // # 86dd is IPv6
        mytransmitbuffer[13] = 0xdd;
        for (i=0; i<IpRequestLen; i++) {
            mytransmitbuffer[14+i] = IpRequest[i];
        }
        myEthTransmit();
}

