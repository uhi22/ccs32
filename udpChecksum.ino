
#define PSEUDO_HEADER_LEN 40
uint8_t pseudoHeader[PSEUDO_HEADER_LEN];

uint16_t udpChecksum_calculateUdpChecksumForIPv6(uint8_t *udpframe, uint16_t udpframeLen, const uint8_t *ipv6source, const uint8_t *ipv6dest) {
	uint16_t evenUdpFrameLen, i, value16, checksum;
	uint8_t nxt;
	uint32_t totalSum;
    //# Parameters:
    //# udpframe: the udp frame including udp header and udppayload
    //# ipv6source: the 16 byte IPv6 source address. Must be the same, which is used later for the transmission.
    //# ipv6source: the 16 byte IPv6 destination address. Must be the same, which is used later for the transmission.
    udpframe[6] = 0; // # at the beginning, set the checksum in the udp header to 00 00.
    udpframe[7] = 0; //
    // Goal: construct an array, consisting of a 40-byte-pseudo-ipv6-header, and the udp frame (consisting of udp header and udppayload).
	// For memory efficienty reason, we do NOT copy the pseudoheader and the udp frame together into one new array. Instead, we are using
	// a dedicated pseudo-header-array, and the original udp buffer.
	evenUdpFrameLen = udpframeLen;
	if ((evenUdpFrameLen & 1)!=0) {
        /* if we have an odd buffer length, we need to add a padding byte in the end, because the sum calculation
           will need 16-bit-aligned data. */
		evenUdpFrameLen++;
		udpframe[evenUdpFrameLen-1] = 0; /* Fill the padding byte with zero. */
	}
    for (i=0; i<PSEUDO_HEADER_LEN; i++) {
		pseudoHeader[i]=0; 
	}
    /* fill the pseudo-ipv6-header */
    for (i=0; i<16; i++) { /* copy 16 bytes IPv6 addresses */
        pseudoHeader[i] = ipv6source[i]; /* IPv6 source address */
        pseudoHeader[16+i] = ipv6dest[i]; /* IPv6 destination address */
	}
    nxt = 0x11; // # should be 0x11 in case of udp
    pseudoHeader[32] = 0; // # high byte of the FOUR byte length is always 0
    pseudoHeader[33] = 0; // # 2nd byte of the FOUR byte length is always 0
    pseudoHeader[34] = udplen >> 8; // # 3rd
    pseudoHeader[35] = udplen & 0xFF; // # low byte of the FOUR byte length
    pseudoHeader[36] = 0; // # 3 padding bytes with 0x00
    pseudoHeader[37] = 0;
    pseudoHeader[38] = 0;
    pseudoHeader[39] = nxt; // # the nxt is at the end of the pseudo header
    // pseudo-ipv6-header finished.
    // Run the checksum over the concatenation of the pseudoheader and the buffer.
    totalSum = 0;
	for (i=0; i<PSEUDO_HEADER_LEN/2; i++) { // running through the pseudo header, in 2-byte-steps
        value16 = pseudoHeader[2*i] * 256 + pseudoHeader[2*i+1]; // take the current 16-bit-word
        totalSum += value16; // we start with a normal addition of the value to the totalSum
        // But we do not want normal addition, we want a 16 bit one's complement sum,
        // see https://en.wikipedia.org/wiki/User_Datagram_Protocol
        if (totalSum>=65536) { // On each addition, if a carry-out (17th bit) is produced, 
            totalSum-=65536; // swing that 17th carry bit around 
            totalSum+=1; // and add it to the least significant bit of the running total.
		}
	}
	for (i=0; i<evenUdpFrameLen/2; i++) { // running through the udp buffer, in 2-byte-steps
        value16 = udpframe[2*i] * 256 + udpframe[2*i+1]; // take the current 16-bit-word
        totalSum += value16; // we start with a normal addition of the value to the totalSum
        // But we do not want normal addition, we want a 16 bit one's complement sum,
        // see https://en.wikipedia.org/wiki/User_Datagram_Protocol
        if (totalSum>=65536) { // On each addition, if a carry-out (17th bit) is produced, 
            totalSum-=65536; // swing that 17th carry bit around 
            totalSum+=1; // and add it to the least significant bit of the running total.
		}
	}
    // Finally, the sum is then one's complemented to yield the value of the UDP checksum field.
    checksum = (uint16_t) (totalSum ^ 0xffff);
    return checksum;
}
