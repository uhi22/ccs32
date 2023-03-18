

#include "EXITypes.h"

#include "appHandEXIDatatypes.h"
#include "appHandEXIDatatypesEncoder.h"
#include "appHandEXIDatatypesDecoder.h"

#include "dinEXIDatatypes.h"
#include "dinEXIDatatypesEncoder.h"
#include "dinEXIDatatypesDecoder.h"

extern struct appHandEXIDocument aphsDoc;
extern struct dinEXIDocument dinDoc;
extern bitstream_t global_stream1;
extern char gResultString[500];

#if defined(__cplusplus)
extern "C"
{
#endif
void projectExiConnector_decode_appHandExiDocument(void);
  /* precondition: The global_stream1.size and global_stream1.data have been set to the byte array with EXI data. */

#if defined(__cplusplus)
}
#endif


#if defined(__cplusplus)
extern "C"
{
#endif
int projectExiConnector_test(int a);
#if defined(__cplusplus)
}
#endif