

/* Logging verbosity settings */
//#define VERBOSE_INIT_ETH

/* Ethernet */
#define MY_ETH_TRANSMIT_BUFFER_LEN 250
#define MY_ETH_RECEIVE_BUFFER_LEN 250

/* TCP */
#define TCP_RX_DATA_LEN 200
extern uint8_t tcp_rxdataLen;
extern uint8_t tcp_rxdata[TCP_RX_DATA_LEN];

#define TCP_PAYLOAD_LEN 200
extern uint8_t tcpPayloadLen;
extern uint8_t tcpPayload[TCP_PAYLOAD_LEN];

/* V2GTP */
#define V2GTP_HEADER_SIZE 8 /* header has 8 bytes */
