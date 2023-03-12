

const uint8_t broadcastIPv6[16] = { 0xff, 0x02, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};
/* our link-local IPv6 address. Todo: do we need to calculate this from the MAC? Or just use a "random"? */
/* For the moment, just use the address from the Win10 notebook, and change the last byte from 0x0e to 0x1e. */
const uint8_t EvccIp[16] = {0xfe, 0x80, 0, 0, 0, 0, 0, 0, 0xc6, 0x90, 0x83, 0xf3, 0xfb, 0xcb, 0x98, 0x1e};

#define V2G_FRAME_LEN 100
uint8_t v2gtpFrameLen;
uint8_t v2gtpFrame[V2G_FRAME_LEN];

#define UDP_REQUEST_LEN 100
uint8_t UdpRequestLen;
uint8_t UdpRequest[UDP_REQUEST_LEN];

#define IP_REQUEST_LEN 100
uint8_t IpRequestLen;
uint8_t IpRequest[IP_REQUEST_LEN];


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
        #define pevPort 50032 /* "random" port. */
        UdpRequest[0] = pevPort >> 8;
        UdpRequest[1] = pevPort  & 0xFF;
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
        //#showAsHex(self.UdpRequest, "UDP request ")
        //broadcastIPv6 = [ 0xff, 0x02, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1]
        //# The content of buffer is ready. We can calculate the checksum. see https://en.wikipedia.org/wiki/User_Datagram_Protocol
        checksum = udpChecksum_calculateUdpChecksumForIPv6(UdpRequest, UdpRequestLen, EvccIp, broadcastIPv6); 
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
        //EvccIp = addressManager_getLinkLocalIpv6Address("bytearray");
        for (i=0; i<16; i++) {
            IpRequest[8+i] = EvccIp[i]; // source IP address
        }            
        for (i=0; i<16; i++) {
            IpRequest[24+i] = broadcastIPv6[i]; // destination IP address
        }
        for (i=0; i<UdpRequestLen; i++) {
            IpRequest[40+i] = UdpRequest[i];
        }            
        //#showAsHex(self.IpRequest, "IpRequest ")
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

