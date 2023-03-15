



#include "EXITypes.h"
#include "dinEXIDatatypes.h"
#include "dinEXIDatatypesEncoder.h"
#include "dinEXIDatatypesDecoder.h"

#define BUFFER_SIZE 256
const uint8_t mytestbuffer[BUFFER_SIZE] = {0x80, 0x9A, 0x01, 0x01, 0xBB, 0xC0, 0x1C, 0x51, 0xE0, 0x20, 0x1B, 0x71, 0x10, 0x9C, 0x7F, 0x64, 0x6C, 0x00, 0x00 };
const uint8_t mytestbufferLen = 19;
uint8_t mybuffer[BUFFER_SIZE];
struct dinEXIDocument dinDoc;
bitstream_t global_stream1;
size_t global_pos1;
int g_errn;
char gDebugString[100];

#if defined(__cplusplus)
extern "C"
{
#endif
void addToTrace_chararray(char *s);
#if defined(__cplusplus)
}
#endif

void debugAddStringAndInt(char *s, int i) {
	char sTmp[1000];
	sprintf(sTmp, "%s%d", s, i);
	strcat(gDebugString, sTmp);
}

int projectExiConnector_test(int a) {
  unsigned int i;
  char s[100];
  char s2[100];
  strcpy(gDebugString, "");
  global_stream1.size = mytestbufferLen;
  global_stream1.data = mytestbuffer;
  global_stream1.pos = &global_pos1;
  *(global_stream1.pos) = 0; /* the decoder shall start at the byte 0 */	
  
  for (i=0; i<1000; i++) { /* for runtime measuremement, run the decoder n times. */
    g_errn = decode_dinExiDocument(&global_stream1, &dinDoc);
  }
  //dinDoc.V2G_Message.Header.SessionID.bytesLen
  addToTrace_chararray("Test from projectExiConnector_test");
  sprintf(s, "SessionID ");
  for (i=0; i<4; i++) {
	sprintf(s2, "%hx ", dinDoc.V2G_Message.Header.SessionID.bytes[i]);
	strcat(s, s2);
  }
  addToTrace_chararray(s);  
  //g_errn = encode_dinExiDocument(&global_stream1, &dinDoc);
 return 2*a;
}