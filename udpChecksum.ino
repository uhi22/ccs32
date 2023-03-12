

uint16_t udpChecksum_calculateUdpChecksumForIPv6(uint8_t *udpframe, uint16_t udpframeLen, const uint8_t *ipv6source, const uint8_t *ipv6dest) {
    //# Parameters:
    //# udpframe: the udp frame including udp header and udppayload
    //# ipv6source: the 16 byte IPv6 source address. Must be the same, which is used later for the transmission.
    //# ipv6source: the 16 byte IPv6 destination address. Must be the same, which is used later for the transmission.
    udpframe[6] = 0; // # at the beginning, set the checksum in the udp header to 00 00.
    udpframe[7] = 0; //
    //# construct an array, consisting of a 40-byte-pseudo-ipv6-header, and the udp frame (consisting of udp header and udppayload)
    /* todo ....
    bufferlen = 40+len(udpframe)
    if ((bufferlen & 1)!=0):
        # if we have an odd buffer length, we need to add a padding byte in the end, because the sum calculation
        # will need 16-bit-aligned data.
        bufferlen+=1
    buffer = bytearray(bufferlen)
    for i in range(0, len(buffer)):
        buffer[i] = 0 # everything 0 for clean initialization
    # fill the pseudo-ipv6-header
    for i in range(0, 16): # copy 16 bytes IPv6 addresses
        buffer[i] = ipv6source[i] # IPv6 source address
        buffer[16+i] = ipv6dest[i] # IPv6 destination address
    udplen = len(udpframe)
    nxt = 0x11 # should be 0x11 in case of udp
    buffer[32] = 0 # high byte of the FOUR byte length is always 0
    buffer[33] = 0 # 2nd byte of the FOUR byte length is always 0
    buffer[34] = udplen >> 8 # 3rd
    buffer[35] = udplen & 0xFF # low byte of the FOUR byte length
    buffer[36] = 0 # 3 padding bytes with 0x00
    buffer[37] = 0
    buffer[38] = 0
    buffer[39] = nxt # the nxt is at the end of the pseudo header
    # pseudo-ipv6-header finished. Now lets put the udpframe into the buffer. (Containing udp header and udppayload)
    for i in range(0, len(udpframe)):
        buffer[40+i] = udpframe[i]
    # showAsHex(buffer, "buffer ")
    # buffer is prepared. Run the checksum over the complete buffer.
    totalSum = 0
    for i in range(0, len(buffer)>>1): # running through the complete buffer, in 2-byte-steps
        value16 = buffer[2*i] * 256 + buffer[2*i+1] # take the current 16-bit-word
        totalSum += value16 # we start with a normal addition of the value to the totalSum
        # But we do not want normal addition, we want a 16 bit one's complement sum,
        # see https://en.wikipedia.org/wiki/User_Datagram_Protocol
        if (totalSum>=65536): # On each addition, if a carry-out (17th bit) is produced, 
            totalSum-=65536 # swing that 17th carry bit around 
            totalSum+=1 # and add it to the least significant bit of the running total.
    # Finally, the sum is then one's complemented to yield the value of the UDP checksum field.
    checksum = totalSum ^ 0xffff
    return checksum
  */  
  return 0;
}
