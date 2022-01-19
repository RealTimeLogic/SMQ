/*
  Introductory SMQ example: How to publish.
  The code does not include proper error handling. The code uses
  assert() and abort() to detect what would most likely be programming
  errors.

  Assemble the SMQ broker URL:
 */
#define BROKER_NAME "localhost"
#define PORTNO "80"
#define SMQ_URL "http://" BROKER_NAME ":" PORTNO "/smq.lsp"
// See the file broker/smq.lsp for details.

/* The pre registered topic names and the Topic IDs (tids).
   NOTE: We do not need the string values when publishing.
*/
#define EXAMPLE_STRUCT_A     2
#define EXAMPLE_STRUCT_B     3
#define EXAMPLE_JSTRUCT_A    4
#define EXAMPLE_JSON_ARRAY   5

#include <JEncoder.h>
#include <SMQ.h>
#include "ExampleStruct.h"

/* Publish SMQ topic EXAMPLE_JSTRUCT_A.
   An aggregate class (collection of logic) required for publishing
 * ExampleStructA as one complete JSON message.
*/
class PubJsonStructA
{
    // 'buf' must be sufficiently large to contain the complete JSON message
   char buf[80];
   JErr err; //https://realtimelogic.com/ba/doc/en/C/reference/html/structJErr.html
   BufPrint bp; //https://realtimelogic.com/ba/doc/en/C/reference/html/structBufPrint.html
   JEncoder encoder; //https://realtimelogic.com/ba/doc/en/C/reference/html/structJEncoder.html
   SMQ* smq; //https://realtimelogic.com/ba/doc/en/C/reference/html/structSMQ.html
public:
   PubJsonStructA(SMQ* smq);
   void publish(ExampleStructA& a);
};

/* Assemble the components */
PubJsonStructA::PubJsonStructA(SMQ* smq) :
   bp(buf, sizeof(buf)),
   encoder(&err, &bp),
   smq(smq)
{
}


/* Publish ExampleStructA (argument a) as a complete JSON message
 */
void PubJsonStructA::publish(ExampleStructA& a)
{
   /* See method 'set':
      https://realtimelogic.com/ba/doc/en/C/reference/html/structJEncoder.html
   */
   encoder.set("{sdf}", "str",a.str, "i",a.i, "d",a.d);
    //All errors are programming errors.
   assert(err.noError());
   //Publish the complete JSON message
   smq->publish(bp.buf,bp.cursor,EXAMPLE_JSTRUCT_A,0);
   encoder.commit(); //Reset buffer and encoder so the encoder can be used again.
}


/* Publish SMQ topic EXAMPLE_JSON_ARRAY
 * An aggregate class (collection of logic) required for publishing an
 * array of JSON objects representing ExampleStructA. The JSON message
 * is sent as several chunks and not as a complete JSON message.
 */
class PubJSonArray : public BufPrint
{
    // 'buf' does not need to contain the complete JSON message
   char buf[40];
   JErr err;
   JEncoder encoder;
   SMQ* smq;
   static int flush(BufPrint* bp, int sizeRequired);
public:
   PubJSonArray(SMQ* smq);
   void publish(ExampleStructA& a);
};


/* Assemble the components */
PubJSonArray::PubJSonArray(SMQ* smq) :
   BufPrint(buf, sizeof(buf),this,flush), //May produce a warning, but is safe.
   encoder(&err, this), //May produce a warning, but is safe.
   smq(smq)
{
}


/* Send JSON as chunks. This is a BufPrint callback function, which is
 * triggered when the buffer (buf[40]) is full or when
 * encoder.commit() is called.
 * typedef int(* BufPrint_Flush) (struct BufPrint *o, int sizeRequired)
 * Click the "BufPrint_Flush" link on the following page:
 * https://realtimelogic.com/ba/doc/en/C/reference/html/structBufPrint.html
*/

int PubJSonArray::flush(BufPrint* bp, int sizeRequired)
{
   int status=0;
   PubJSonArray* self = (PubJSonArray*)bp;
   if(bp->cursor) // Data in buffer
   {
      // SMQ::write enables sending an SMQ messages as smaller chunks.
      status=self->smq->write(bp->buf, bp->cursor);
      bp->cursor = 0;
   }
   //sizeRequired=0 when encoder.commit() is called in method publish below
   if(sizeRequired == 0 && status == 0)
   {
      // smq::pubflus: Tell broker to assemble all chunks and to
      // publish the complete message.
      status=self->smq->pubflush(EXAMPLE_JSON_ARRAY,0);
   }
   // A non zero return value will set the JErr instance to state JErrT_IOErr
   return status;
}

/* Publish an array of JSON objects representing ExampleStructA as
 * multiple chunks. The first array element is from the argument
 * 'a'. The other 10 array elements are created inline below.
 */
void PubJSonArray::publish(ExampleStructA& a)
{
   encoder.beginArray(); // JSON: [
   encoder.set("{sdf}", "str",a.str, "i",a.i, "d",a.d);
   for(int i=0 ; i < 10 ; i++)
   {
      char buf[20];
      //Function basnprintf is in the BufPrint lib/
      basnprintf(buf,sizeof(buf),"Counter=%d",i);
      // This may trigger PubJSonArray::flush
      encoder.set("{sdf}", "str",buf, "i",i, "d",rand()/3);
      if( ! err.noError() ) break;
   }
   encoder.endArray(); // JSON: ]
   //All errors, except JErrT_IOErr, are programming errors.
   assert(err.noError() || err.getErrT() == JErrT_IOErr);
   encoder.commit(); //Triggers PubJSonArray::flush with sizeRequired==0
}

/* Connect to broker
   loop: publish all four topics (message types).
*/
int main()
{
#ifdef _WIN32
   WSADATA wsaData;
   /* Windows specific: Start winsock library */
    WSAStartup(MAKEWORD(1,1), &wsaData);
#endif

   U8 smqBuf[256];
   SMQ smq(smqBuf,sizeof(smqBuf));

   PubJsonStructA pubJsonStructA(&smq);
   PubJSonArray  pubJSonArray(&smq);

   /** Connecting to the broker is a two step proceedure:
    */
   printf("Connecting to SMQ broker: %s\n",SMQ_URL);
   if(smq.init(SMQ_URL, 0) < 0)
   {
      printf("Cannot connect to broker, status: %d\n", smq.status);
      return 0;
   }
   static const char uniquID[]={"publish: " __DATE__ __TIME__};
   if(smq.connect(uniquID, sizeof(uniquID),
                  0, 0, /* credentials */
                  0, 0 /* info */))
   {
      printf("SMQ Connect failed, status: %d\n", smq.status);
      return -1;
   }
   printf("Connected\n\n");

   /* This code would have to call SMQ::create() here to register the
    * topics if we did not force the broker to use static tids. See
    * LED-SMQ.c for how this is done. Note that LED-SMQ.c calls
    * SharkMQ_create(), which is the secure C version of SMQ::create()
    * running in SMQ compatibility mode.
    */

   smq.timeout=50; // Make SMQ::getMessage() below sleep for max 50ms
   for(int counter=1;;counter++) //Forever
   {
      struct ExampleStructA a = {
         "Hello",
         counter,
         rand()/1.1,
      };
      //Publish the raw ExampleStructA data as one chunk
      printf("Pub struct A\n");
      smq.publish(&a,sizeof(a), EXAMPLE_STRUCT_A, 0);

      struct ExampleStructB b = {
         {
            "Hello",
            4,
            6.5
         },
         {
            "World",
            5,
            3.2
         },
      };
      //Publish the raw ExampleStructB data as one chunk
      printf("Pub struct B\n");
      smq.publish(&b,sizeof(b), EXAMPLE_STRUCT_B, 0);

      //Publish ExampleStructA as one JSON encoded chunk
      printf("Pub JSON struct A\n");
      pubJsonStructA.publish(a);

      //Publish array of ExampleStructA as multiple JSON encoded chunks
      printf("Pub JSON Array\n");
      pubJSonArray.publish(a);

      /* The following code slows down the loop of sending the
       * messages to 50 millisecond bursts. The code also checks that
       * we are still connected. We could also check this when we
       * publish, but it's easier to just check it here instead of
       * each time publish is called. Note, we are not expecting any
       * messages.
       */
      U8* msg;
      int status = smq.getMessage(&msg);
      if(status != SMQ_TIMEOUT)
      {
         printf("getMessage returned %d, line %d\n",status,__LINE__);
         break;
      }
   }

   return 0;
}


//Used by selib.c when XPRINTF is defined.
#ifdef XPRINTF
void _xprintf(const char* fmt, ...)
{
   va_list varg;
   va_start(varg, fmt);
   vprintf(fmt, varg);
   va_end(varg);
}
#endif
