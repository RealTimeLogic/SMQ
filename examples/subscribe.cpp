/*
  Introductory SMQ example: How to subscribe.
  The code does not include proper error handling. The code uses
  assert() and abort() to detect what would most likely be programming
  errors.

  Assemble the SMQ broker URL:
 */
#define BROKER_NAME "localhost"
#define PORTNO "80"
#define SMQ_URL "http://" BROKER_NAME ":" PORTNO "/smq.lsp"

/* The pre registered topic names and the Topic IDs (tids).

   NOTE: We still need the string values when subscribing and macro
   mkstr() below helps us convert the defines to strings.
*/
#define EXAMPLE_STRUCT_A     2
#define EXAMPLE_STRUCT_B     3
#define EXAMPLE_JSTRUCT_A    4
#define EXAMPLE_JSON_ARRAY   5
#define mkstr(s) #s


#include <JDecoder.h>
#include <SMQ.h>

#include "ExampleStruct.h"

/* Assemble received SMQ topic EXAMPLE_JSTRUCT_A.
   An aggregate class (collection of logic) required for assembling
   one complete JSON message and converting the data to ExampleStructA.
*/
class AssembleJsonStructA
{
   char maxMembN[40]; /* Buffer for holding temporary JSON member name */
   unsigned char jDecoderBuf[100];
   JDecoder decoder;
   JParser parser; /* JSON parser */
public:
   AssembleJsonStructA();
   void assemble(const U8* msg, int size);
};

/* Assemble the components */
AssembleJsonStructA::AssembleJsonStructA() :
   decoder(jDecoderBuf, sizeof(jDecoderBuf)),
   parser(&decoder, maxMembN, sizeof(maxMembN), AllocatorIntf_getDefault(),0)
{
}


/* Parse a complete JSON message using JParser and inject parsed JSON
 * syntax tree directly into ExampleStructA by using JDecoder.
 * JParser is an efficient JSON parser (JParser) callback object that
 * can be used when we know the exact structure of the received
 * JSON message.
 * Tutorial:
 * https://realtimelogic.com/ba/doc/en/C/reference/html/md_en_C_md_JSON.html#DecodingData
 * Ref:
 * https://realtimelogic.com/ba/doc/en/C/reference/html/structJDecoder.html
 */
void AssembleJsonStructA::assemble(const U8* msg, int size)
{
   /* We do not need to re-run the following each time the JDecoder is
    * used if the parser parses the same structure. In this example,
    * we could move the following two code lines to the constructor.
    */
   ExampleStructA a;
   if(decoder.get("{sdf}",
                  JD_MSTR(&a, str),
                  JD_MNUM(&a, i),
                  JD_MNUM(&a, d)))
   {
      printf("jDecoderBuf too small\n");
      abort();
   }
   // < 0 signals an error and zero signals that we did not parse a
   // complete object. This code expects a complete object.
   if(parser.parse(msg, size) <= 0)
   {
      printf("JSON parse error %d\n",__LINE__);
      abort();
   }
   printf("JSON Assembled ExampleStructA: %s, %d, %lf\n",a.str,a.i,a.d);
}


/* Assemble received SMQ topic EXAMPLE_JSON_ARRAY.
   An aggregate class (collection of logic) required for assembling an
   array of JSON objects representing ExampleStructA.
   The class can manage receiving the JSON array as chunks.
*/
class AssembleJSonArray
{
   char maxMembN[40]; /* Buffer for holding temporary JSON member name */
   JParserValFact pv; /* JSON Parser Value (JVal) Factory */
   JParser parser; /* JSON parser */
public:
   AssembleJSonArray();
   void assemble(SMQ* smq, U8* msg, int size);
};

/* Assemble the components */
AssembleJSonArray::AssembleJSonArray() :
   pv(AllocatorIntf::getDefault(), AllocatorIntf::getDefault()),
   parser(&pv, maxMembN, sizeof(maxMembN), AllocatorIntf::getDefault(),0)
{
}

/* Loop and keep parsing until the complete JSON array has been
 * received. The argument msg is from the main function and could be a
 * complete JSON message, however, this method can manage receiving the
 * SMQ message in chunks. An SMQ message will be received in chunks if
 * the SMQ buffer is not large enough to contain a complete SMQ
 * message.
 */
void AssembleJSonArray::assemble(SMQ* smq, U8* msg, int size)
{
   for(;;)
   {
      int status = parser.parse(msg, size);
      if(status) // Error or a complete message
      {
         if(status < 0) // Error
         {
            printf("JSON parse error %d\n",__LINE__);
            abort();
         }
         /* We received a complete message. SMQ is packet based and
          * the following check makes sure we receive exactly one
          * packet when the JSON parser completed.
         */
         assert(smq->frameLen == smq->bytesRead);

         /* We used the memory efficient JParser callback object in
          * the AssembleJsonStructA class above. The JParser syntax
          * tree callback object cannot be used when assembling
          * anything that is dynamic in nature such as an array. Here
          * we use the standard JParserValFact callback object (pv),
          * which builds a dynamic JSON syntax tree. The following
          * code gets the root node, loops over the received array,
          * and extracts the JSON encoded ExampleStructA array object
          * for each element in the array.
          * https://realtimelogic.com/ba/doc/en/C/reference/html/structJParserValFact.html
          * https://realtimelogic.com/ba/doc/en/C/reference/html/structJVal.html
          */
         JErr err;
         JVal* v=pv.getFirstVal()->getArray(&err);
         assert(v); //We should have the start of the array
         do {
            ExampleStructA a;
            v->get(&err,"{sdf}", "str",&a.str, "i",&a.i, "d",&a.d);
            // We are not using the data, we just make sure it's as expected.
            assert(err.noError());
         } while(NULL != (v = v->getNextElem()));
         assert(err.noError());
         return; // We are done
      }
      // Get next chunk from the SMQ stack.
      assert(smq->frameLen != smq->bytesRead);
      size = smq->getMessage(&msg);
      if(size < 0)
      {
         //A TCP disconnect
         printf("getMessage returned %d\n",size);
      }
   } //End loop
}


int main()
{
#ifdef _WIN32
   WSADATA wsaData;
   /* Windows specific: Start winsock library */
    WSAStartup(MAKEWORD(1,1), &wsaData);
#endif

   U8 smqBuf[256];
   SMQ smq(smqBuf,sizeof(smqBuf));

   AssembleJsonStructA jsonStructA;
   AssembleJSonArray jsonArray;

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

   /* Subscribe to all 4 topics:
    */
   smq.subscribe(mkstr(EXAMPLE_STRUCT_A));
   smq.subscribe(mkstr(EXAMPLE_STRUCT_B));
   smq.subscribe(mkstr(EXAMPLE_JSTRUCT_A));
   smq.subscribe(mkstr(EXAMPLE_JSON_ARRAY));

   int counter=0;
   // Loop and receive SMQ control messages and messages we
   // subscribed to.
   for(;;)
   {
      U8* msg;
      /* Wait for the next message */
      int len =  smq.getMessage(&msg);
      if(len < 0) /* We received a control message or an error code */
      {
         if(SMQ_TIMEOUT == len)
            continue; /* Timeout not used */

         /* We receive an SMQ_SUBACK for each topic we subscribe
          * to. All tids have been statically registered by Lua code
          * in the broker/.preload script. The following asserts
          * that it is as expected.
          */
         if(SMQ_SUBACK == len)
         {
            assert(EXAMPLE_STRUCT_A == smq.ptid ||
                   EXAMPLE_STRUCT_B == smq.ptid ||
                   EXAMPLE_JSTRUCT_A == smq.ptid ||
                   EXAMPLE_JSON_ARRAY == smq.ptid);
            continue;
         }

         /* We are not expecting any other SMQ control messages */
         printf("Rec Error: %d, %d.\n",len,smq.status);
         break;
      }

      ExampleStructA* a;
      ExampleStructB* b;

      /* We can use a standard C switch statement since all tids have
       * been pre-registered by the Lua .preload script. The
       * following code expects that the SMQ buffer (smqBuf[256]) is
       * sufficiently large for containing all received messages,
       * except EXAMPLE_JSON_ARRAY.
       */
      switch(smq.tid)
      {
         case EXAMPLE_STRUCT_A:
            assert(sizeof(ExampleStructA) == len); // See intro comment above
            a = (ExampleStructA*)msg; // Convert raw data to ExampleStructA
            printf("ExampleStructA: %s, %d, %lf\n",a->str,a->i,a->d);
            counter = counter ? counter+1 : a->i;
            // Fails if you restart publisher or use multiple publishers
            assert(counter == a->i);
            break;

         case EXAMPLE_STRUCT_B:
            assert(sizeof(ExampleStructB) == len);  // See intro comment above
            b = (ExampleStructB*)msg; // Convert raw data to ExampleStructB
            printf("ExampleStructB: {%s, %d, %lf} {%s, %d, %lf}\n",
                    b->a.str,b->a.i,b->a.d, b->b.str,b->b.i,b->b.d);
            break;

         case EXAMPLE_JSTRUCT_A:
            // Method assemble checks that we received one complete message
            jsonStructA.assemble(msg,len);
            break;

         case EXAMPLE_JSON_ARRAY:
            //Chunks OK
            jsonArray.assemble(&smq,msg,len);
            break;

         default:
            printf("Received unexpected tid %d\n",smq.tid);
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
