



#define NEXT_TCP 0x06 /* the next protocol is TCP */

#define TCP_FLAG_SYN 0x02
#define TCP_FLAG_ACK 0x10

#define TCP_TRANSMIT_PACKET_LEN 100
uint8_t TcpTransmitPacketLen;
uint8_t TcpTransmitPacket[TCP_TRANSMIT_PACKET_LEN];

#define TCPIP_TRANSMIT_PACKET_LEN 100
uint8_t TcpIpRequestLen;
uint8_t TcpIpRequest[TCPIP_TRANSMIT_PACKET_LEN];

#define TCP_STATE_CLOSED 0
#define TCP_STATE_SYN_SENT 1
#define TCP_STATE_ESTABLISHED 2

uint8_t tcpState = TCP_STATE_CLOSED;

uint32_t TcpSeqNr=200; /* a "random" start sequence number */
uint32_t TcpAckNr;

void evaluateTcpPacket(void) {
  uint8_t flags;
  /* todo: check the ports and IP addresses */
  flags = myreceivebuffer[67];
  if ((flags == TCP_FLAG_SYN+TCP_FLAG_ACK) && (tcpState == TCP_STATE_SYN_SENT)) {
    tcpState = TCP_STATE_ESTABLISHED;
    /* todo: send the ACK */
    addToTrace("[TCP] connected.");
  }
}

void tcp_connect(void) {
    uint8_t i;
    uint16_t lenInclChecksum;
    uint16_t checksum;
	addToTrace("[TCP] connecting");

    // # TCP header needs at least 24 bytes:
    // 2 bytes source port
    // 2 bytes destination port
	// 4 bytes sequence number
	// 4 bytes ack number
	// 4 bytes DO/RES/Flags/Windowsize
	// 2 bytes checksum
	// 2 bytes urgentPointer
	// n*4 bytes options/fill (empty for the ACK frame and payload frames)
    TcpTransmitPacket[0] = (uint8_t)(evccPort >> 8); /* source port */
    TcpTransmitPacket[1] = (uint8_t)(evccPort);
    TcpTransmitPacket[2] = (uint8_t)(seccTcpPort >> 8); /* destination port */
    TcpTransmitPacket[3] = (uint8_t)(seccTcpPort);
	
    TcpTransmitPacket[4] = (uint8_t)(TcpSeqNr>>24); /* sequence number */
    TcpTransmitPacket[5] = (uint8_t)(TcpSeqNr>>16);
    TcpTransmitPacket[6] = (uint8_t)(TcpSeqNr>>8);
    TcpTransmitPacket[7] = (uint8_t)(TcpSeqNr);
	
    TcpTransmitPacket[8] = (uint8_t)(TcpAckNr>>24); /* ack number */
    TcpTransmitPacket[9] = (uint8_t)(TcpAckNr>>16);
    TcpTransmitPacket[10] = (uint8_t)(TcpAckNr>>8);
    TcpTransmitPacket[11] = (uint8_t)(TcpAckNr);

	TcpTransmitPacket[13] = TCP_FLAG_SYN;
	#define TCP_RECEIVE_WINDOW 1000 /* number of octetts we are able to receive */
    TcpTransmitPacket[14] = (uint8_t)(TCP_RECEIVE_WINDOW>>8);
    TcpTransmitPacket[15] = (uint8_t)(TCP_RECEIVE_WINDOW);
	
    // checksum will be calculated afterwards
    TcpTransmitPacket[16] = 0;
    TcpTransmitPacket[17] = 0;
	
    TcpTransmitPacket[18] = 0; /* 16 bit urgentPointer. Always zero in our case. */
    TcpTransmitPacket[19] = 0;
	
    TcpTransmitPacket[20] = 0x02; /* options: 12 bytes, just copied from the Win10 notebook trace */
    TcpTransmitPacket[21] = 0x04;
    TcpTransmitPacket[22] = 0x05;
    TcpTransmitPacket[23] = 0xA0;

    TcpTransmitPacket[24] = 0x01;
    TcpTransmitPacket[25] = 0x03;
    TcpTransmitPacket[26] = 0x03;
    TcpTransmitPacket[27] = 0x08;

    TcpTransmitPacket[28] = 0x01;
    TcpTransmitPacket[29] = 0x01;
    TcpTransmitPacket[30] = 0x04;
    TcpTransmitPacket[31] = 0x02;

	TcpTransmitPacketLen = 32; /* only the TCP header, no data */
  TcpTransmitPacket[12] = (TcpTransmitPacketLen/4) << 4; /* DataOffser = 8 * 32 bit. Reserved=0. */
	
    checksum = calculateUdpAndTcpChecksumForIPv6(TcpTransmitPacket, TcpTransmitPacketLen, EvccIp, SeccIp, NEXT_TCP); 
    TcpTransmitPacket[16] = (uint8_t)(checksum >> 8);
    TcpTransmitPacket[17] = (uint8_t)(checksum);
    tcp_packRequestIntoIp();
    tcpState = TCP_STATE_SYN_SENT;
}
        
void tcp_packRequestIntoIp(void) {
        // # embeds the (SDP) request into the lower-layer-protocol: IP, Ethernet
        uint8_t i;
        uint16_t plen;
        TcpIpRequestLen = TcpTransmitPacketLen + 8 + 16 + 16; // # IP6 header needs 40 bytes:
                                                    //  #   4 bytes traffic class, flow
                                                    //  #   2 bytes destination port
                                                    //  #   2 bytes length (incl checksum)
                                                    //  #   2 bytes checksum
        TcpIpRequest[0] = 0x60; // # traffic class, flow
        TcpIpRequest[1] = 0; 
        TcpIpRequest[2] = 0;
        TcpIpRequest[3] = 0;
        plen = TcpTransmitPacketLen; // length of the payload. Without headers.
        TcpIpRequest[4] = plen >> 8;
        TcpIpRequest[5] = plen & 0xFF;
        TcpIpRequest[6] = NEXT_TCP; // next level protocol, 0x06 = TCP in this case
        TcpIpRequest[7] = 0x0A; // hop limit
        // We are the PEV. So the EvccIp is our own link-local IP address.
        //EvccIp = addressManager_getLinkLocalIpv6Address("bytearray");
        for (i=0; i<16; i++) {
            TcpIpRequest[8+i] = EvccIp[i]; // source IP address
        }            
        for (i=0; i<16; i++) {
            TcpIpRequest[24+i] = SeccIp[i]; // destination IP address
        }
        for (i=0; i<TcpTransmitPacketLen; i++) {
            TcpIpRequest[40+i] = TcpTransmitPacket[i];
        }            
        //#showAsHex(TcpIpRequest, "TcpIpRequest ")
        tcp_packRequestIntoEthernet();
}

void tcp_packRequestIntoEthernet(void) {
        //# packs the IP packet into an ethernet packet
        uint8_t i;        
        mytransmitbufferLen = TcpIpRequestLen + 6 + 6 + 2; // # Ethernet header needs 14 bytes:
                                                       // #  6 bytes destination MAC
                                                       // #  6 bytes source MAC
                                                       // #  2 bytes EtherType
        //# fill the destination MAC with the MAC of the charger
        fillDestinationMac(evseMac);
        fillSourceMac(myMAC); // bytes 6 to 11 are the source MAC
        mytransmitbuffer[12] = 0x86; // # 86dd is IPv6
        mytransmitbuffer[13] = 0xdd;
        for (i=0; i<TcpIpRequestLen; i++) {
            mytransmitbuffer[14+i] = TcpIpRequest[i];
        }
        myEthTransmit();
}

