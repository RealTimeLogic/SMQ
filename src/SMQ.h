/*
 *     ____             _________                __                _     
 *    / __ \___  ____ _/ /_  __(_)___ ___  ___  / /   ____  ____ _(_)____
 *   / /_/ / _ \/ __ `/ / / / / / __ `__ \/ _ \/ /   / __ \/ __ `/ / ___/
 *  / _, _/  __/ /_/ / / / / / / / / / / /  __/ /___/ /_/ / /_/ / / /__  
 * /_/ |_|\___/\__,_/_/ /_/ /_/_/ /_/ /_/\___/_____/\____/\__, /_/\___/  
 *                                                       /____/          
 *
 ****************************************************************************
 *            HEADER
 *
 *   $Id: SMQ.h 5021 2022-01-13 18:59:01Z wini $
 *
 *   COPYRIGHT:  Real Time Logic LLC, 2014 - 2022
 *
 *   This software is copyrighted by and is the sole property of Real
 *   Time Logic LLC.  All rights, title, ownership, or other interests in
 *   the software remain the property of Real Time Logic LLC.  This
 *   software may only be used in accordance with the terms and
 *   conditions stipulated in the corresponding license agreement under
 *   which the software has been supplied.  Any unauthorized use,
 *   duplication, transmission, distribution, or disclosure of this
 *   software is expressly forbidden.
 *                                                                        
 *   This Copyright notice may not be removed or modified without prior
 *   written consent of Real Time Logic LLC.
 *                                                                         
 *   Real Time Logic LLC. reserves the right to modify this software
 *   without notice.
 *
 *               https://realtimelogic.com
 ****************************************************************************
 * SMQ C library:
 *  https://realtimelogic.com/ba/doc/en/C/reference/html/group__SMQClient.html
 */

#ifndef __SMQClient_h
#define __SMQClient_h

#define SMQ_NONSEC

#include "selib.h"

#ifndef ISDIGIT
#define ISDIGIT isdigit
#endif

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

/** @addtogroup SMQClient
@{
*/

/** \defgroup SMQClientErrorCodes Error codes returned by function SMQ_getMessage
\ingroup SMQClient
\anchor SMQClientErrorCodes hello
@{
*/

/** The buffer provided in SMQ_constructor is not sufficiently large.
 */
#define SMQE_BUF_OVERFLOW    -10000

/** The URL provided is invalid.
 */
#define SMQE_INVALID_URL     -10002 

/** TCP connection error
 */
#define SMQE_PROTOCOL_ERROR  -10003 

/** Server sent a disconnect message
 */
#define SMQE_DISCONNECT      -10004


/** No PONG response to PING: timeout
 */
#define SMQE_PONGTIMEOUT     -10005


/** The SMQ_getMessage call timed out.
 */
#define SMQE_TIMEOUT         -10100


/** @} */ /* end SMQClientErrorCodes */


/** \defgroup SMQClientRespCodes Response codes returned by function SMQ_getMessage
\ingroup SMQClient
@{
*/

/** #SMQ_subscribe response message received via #SMQ_getMessage.
    \li SMQ::ptid is set to the subscribed Topic ID.
    \li SMQ::status is set to zero (0) if the request was accepted and
    a negative value if the request was denied.
    \li the 'msg' out parameter in #SMQ_getMessage is set to the optional
    server's authentication response message if the request was
    denied.
 */
#define SMQ_SUBACK           -20000

/** #SMQ_create response message received via #SMQ_getMessage.
    \li SMQ::ptid is set to the created Topic ID.
    \li SMQ::status is set to zero (0) if the request was accepted and
    a negative value if the request was denied.
    \li the 'msg' out parameter in #SMQ_getMessage is set to the optional
    server's authentication response message if the request was
    denied.
 */
#define SMQ_CREATEACK        -20001

/** #SMQ_createsub response message received via #SMQ_getMessage.
    \li SMQ::ptid is set to the created Subtopic ID.
    \li SMQ::status is set to zero (0) if the request was accepted and
    a negative value if the request was denied.
 */
#define SMQ_CREATESUBACK     -20002

/** Change notification event (from observed tid). Observe events are
 * initiated via #SMQ_observe.

    \li SMQ::ptid is set to the observed Topic ID.
    \li SMQ::status is set to the number of clients subscribed to the topic.
 */
#define SMQ_SUBCHANGE        -20003

/** @} */ /* end SMQClientRespCodes */


#define SMQSTR(str) str, (sizeof(str)-1)

/** SimpleMQ structure.
 */
typedef struct SMQ
{
   SOCKET sock;

   U8* buf; /**< The buffer set via the constructor. */
#ifdef SMQ_ENABLE_SENDBUF
   U8* sBuf;
#endif

   /** Timeout in milliseconds to wait in functions waiting for server
       data */
   U32 timeout;
   S32 pingTmoCounter,pingTmo;
   U32 clientTid; /**< Client's unique topic ID */
   U32 tid;  /**< Topic: set when receiving MSG_PUBLISH from broker */
   U32 ptid; /**< Publisher's tid: Set when receiving MSG_PUBLISH from broker */
   U32 subtid; /**< Sub-tid: set when receiving MSG_PUBLISH from broker */
   int status; /**< Last known error code */
   U16 bufLen;
   U16 rBufIx;
#ifdef SMQ_ENABLE_SENDBUF
   U16 sBufIx;
#endif
   U16 frameLen; /**< The SimpleMQ frame size for the incomming data */
   /** Read frame data using SMQ_getMessage until: frameLen - bytesRead = 0 */
   U16 bytesRead;
   U8 inRecv; /* boolean set to true when thread blocked in SMQ_recv */
#ifdef __cplusplus

/** Create a SimpleMQ client instance.
    \see SMQ_constructor
*/
   SMQ(U8* buf, U16 bufLen);


/** Initiate the SMQ server connection. 
    \see SMQ_init
*/
   int init(const char* url, U32* rnd);

/** Connect/establish a persistent SimpleMQ connection.
    \see SMQ_connect
*/
   int connect(const char* uid, int uidLen, const char* credentials,
                    U8 credLen, const char* info, int infoLen);


/** Gracefully close the connection. 
    \see SMQ_disconnect
*/
   void disconnect();


/** Terminate a SimpleMQ instance.
    \see SMQ_destructor
*/
   ~SMQ();


/** Create a topic an fetch the topic ID (tid).
    \see SMQ_create
*/
   int create(const char* topic);


/** Create a sub-topic and fetch the subtopic ID.
    \see SMQ:createsub
*/
   int createsub(const char* subtopic);


/** The response to SMQ_subscribe is asynchronous and returned as status
    #SMQ_SUBACK via #SMQ_getMessage.
    \see SMQ_subscribe
*/
   int subscribe(const char* topic);


/** Requests the broker to unsubscribe the server from a topic.
    \see SMQ_unsubscribe
*/
   int unsubscribe(U32 tid);


/** Publish messages to a topic and optionally to a sub-topic. Topic
    name must have previosly been been resolved by #SMQ_create and
    sub-topic should preferably have been created by #SMQ_createsub.
    \see SMQ_publish
*/
   int publish(const void* data, int len, U32 tid, U32 subtid);


/** Publish a message in chunks and request the broker to assemble the
    message before publishing to the subscriber(s).
    \see SMQ_wrtstr
*/
   int wrtstr(const char* str);

/** Publish a message in chunks and request the broker to assemble the
    message before publishing to the subscriber(s).
    \see SMQ_write
*/
   int write( const void* data, int len);

/** Flush the internal buffer and request the broker to assemble all
    stored fragments as one message.
    \see SMQ_pubflush
*/
   int pubflush(U32 tid, U32 subtid);


/** Request the broker to provide change notification events when the
    number of subscribers to a specific topic changes. Ephemeral topic
    IDs can also be observed.
    \see SMQ_observe
*/
   int observe(U32 tid);


/** Stop receiving change notifications for a topic ID or ephemeral topic ID.
    \see SMQ_unobserve
*/
   int unobserve(U32 tid);


/** Wait for messages sent from the broker.
    \see SMQ_getMessage
*/
   int getMessage(U8** msg);


/** Returns the message size, which is SMQ::frameLen - 15.
    \see SMQ_getMsgSize
*/
   int getMsgSize();

#endif
} SMQ;

#ifdef __cplusplus
extern "C" {
#endif

/** \defgroup SMQClient_C C API
\ingroup SMQClient
@{
*/


/** Create a SimpleMQ client instance.
    \param o Uninitialized data of size sizeof(SMQ).
    \param buf is used for internal management and must not be less
    than 127 bytes and not smaller than the largest control
    frame. Function SMQ_getMessage will return #SMQE_BUF_OVERFLOW if
    the buffer is not sufficiently large.
    \param bufLen buffer length.
 */
void SMQ_constructor(SMQ* o, U8* buf, U16 bufLen);


/** Bare metal configuration. This macro must be called immediately
    after calling the constructor on bare metal systems.
    \param o the #SMQ instance.
    \param ctx an #SeCtx instance.
 */
#define SMQ_setCtx(o, ctx) SOCKET_constructor(&(o)->sock, ctx)


/** Initiate the SMQ server connection. The connection phase is
    divided into two steps: (1) initiating and (2) connecting via
    SMQ_connect.
    \param o the SMQ instance.
    \param url is a URL that starts with http:// and this URL
    must be to a server resource that initiates a SimpleMQ connection.
    \param rnd (out param) a random number created by the server. This
    number can be used for securing hash based password encryption.
    \returns 0 on success, error code from TCP/IP stack, or
    [SimpleMQ error code](\ref SMQClientErrorCodes). SMQ::buf is set
    to the IP address of the client as seen by the broker.
 */
int SMQ_init(SMQ* o, const char* url, U32* rnd);

/** Connect/establish a persistent SimpleMQ connection. The connection
    phase is divided into two steps: (1) initiating via SMQ_init and (2)
    connecting.
    \param o the SMQ instance.
    \param uid a universally unique client identifier (uid) must be
    unique across all clients connecting to the same broker
    instance. The uid is preferably a stringified version of the
    client's Ethernet MAC address.
    \param uidLen the uid length.
    \param credentials provide credentials if required by the broker instance.
    \param credLen credentials length.
    \param info a string that provides information to optional server
    code interacting with the broker. This string is also passed into
    the optional broker's authentication callback function.
    \param infoLen length of info.
    \returns 0 on success, error code from TCP/IP stack,
    [SimpleMQ error code](\ref SMQClientErrorCodes), or one of the
    following error codes from the broker:

    \li 0x01	Connection Refused: unacceptable protocol version
    \li 0x02	Connection Refused: server unavailable
    \li 0x03	Connection Refused: Incorrect credentials
    \li 0x04	Connection Refused: Client certificate required
    \li 0x05	Connection Refused: Client certificate not accepted
    \li 0x06	Connection Refused: Access denied

    The broker may optionally send a human readable string in addition
    to the above broker produced error codes. This string is avaiable
    via SMQ::buf.
 */
int SMQ_connect(SMQ* o, const char* uid, int uidLen, const char* credentials,
                U8 credLen, const char* info, int infoLen);


/** Gracefully close the connection. You cannot publish any messages
    after calling this method.
    \param o the SMQ instance.
 */
void SMQ_disconnect(SMQ* o);


/** Terminate a SimpleMQ instance.
    \param o the SMQ instance.
 */
void SMQ_destructor(SMQ* o);


/** Create a topic an fetch the topic ID (tid). The SMQ protocol is
    optimized and does not directly use a string when publishing, but a
    number. The server randomly a creates 32 bit number and
    persistently stores the topic name and number.

    The response to SMQ_create is asynchronous and returned as status
    #SMQ_CREATEACK via #SMQ_getMessage.

    \param o the SMQ instance.
    \param topic the topic name where you plan on publishing messages.
 */
int SMQ_create(SMQ* o, const char* topic);


/** Create a sub-topic and fetch the subtopic ID.

    The response to SMQ_subscribe is asynchronous and returned as status
    #SMQ_CREATESUBACK via #SMQ_getMessage.

    \param o the SMQ instance.
    \param subtopic the sub-topic name you want registered.
 */
int SMQ_createsub(SMQ* o, const char* subtopic);


/**

    The response to SMQ_subscribe is asynchronous and returned as status
    #SMQ_SUBACK via #SMQ_getMessage.

    \param o the SMQ instance.
    \param topic the topic name to subscribe to.
 */
int SMQ_subscribe(SMQ* o, const char* topic);


/** Requests the broker to unsubscribe the server from a topic.
    \param o the SMQ instance.
    \param tid the topic name's Topic ID.
 */
int SMQ_unsubscribe(SMQ* o, U32 tid);


/** Publish messages to a topic and optionally to a sub-topic. Topic
    name must have previosly been been resolved by #SMQ_create and
    sub-topic should preferably have been created by #SMQ_createsub.
    \param o the SMQ instance.
    \param data message payload.
    \param len payload length.
    \param tid the topic ID (created with SMQ_create).
    \param subtid optional sub-topic ID preferably created with SMQ_createsub.
 */
int SMQ_publish(SMQ* o, const void* data, int len, U32 tid, U32 subtid);


/** Publish a message in chunks and request the broker to assemble the
    message before publishing to the subscriber(s). This method uses
    the internal buffer (SMQ::buf) and sends the message as a chunk
    when the internal buffer is full, thus sending the message as an
    incomplete message to the broker. The message is assembled by the
    broker when you flush the remaining bytes in the buffer by calling
    #SMQ_pubflush.

    \param o the SMQ instance.
    \param str a string.
 */
int SMQ_wrtstr(SMQ* o, const char* str);

/** Publish a message in chunks and request the broker to assemble the
    message before publishing to the subscriber(s). This method uses
    the internal buffer (SMQ::buf) and sends the message as a chunk
    when the internal buffer is full, thus sending the message as an
    incomplete message to the broker. The message is assembled by the
    broker when you flush the remaining bytes in the buffer by calling
    #SMQ_pubflush.


    \param o the SMQ instance.
    \param data message payload.
    \param len payload length.
 */
int SMQ_write(SMQ* o,  const void* data, int len);

/** Flush the internal buffer and request the broker to assemble all
    stored fragments as one message. This message is then published to
    topic 'tid', and sub-topic 'subtid'.

    \param o the SMQ instance.
    \param tid the topic ID (created with SMQ_create).
    \param subtid optional sub-topic ID preferably created with SMQ_createsub.

    Example:
    \code
    SMQ_wrtstr(smq, "Hello");
    SMQ_wrtstr(smq, " ");
    SMQ_wrtstr(smq, "World");
    SMQ_pubflush(smq,tid,subtid);
    \endcode

 */
int SMQ_pubflush(SMQ* o, U32 tid, U32 subtid);


/** Request the broker to provide change notification events when the
    number of subscribers to a specific topic changes. Ephemeral topic
    IDs can also be observed. The number of connected subscribers for
    an ephemeral ID can only be one, which means the client is
    connected. Receiving a change notification for an ephemeral ID
    means the client has disconnected and that you no longer will get
    any change notifications for the observed topic ID.

    Change notification events are received as #SMQ_SUBCHANGE via
    #SMQ_getMessage.

    \param o the SMQ instance.
    \param tid the Topic ID you which to observe.
 */
int SMQ_observe(SMQ* o, U32 tid);


/** Stop receiving change notifications for a topic ID or ephemeral topic ID.
    \param o the SMQ instance.
    \param tid the Topic ID you no longer want to observe.
 */
int SMQ_unobserve(SMQ* o, U32 tid);


/** Wait for messages sent from the broker.
    \param o the SMQ instance.
    \param msg a pointer to the response data (out param)
    \returns
    \li a negative value signals an
    [error code](\ref SMQClientErrorCodes) or an
    [asynchronous response code](\ref SMQClientRespCodes).
    \li zero signals timeout.
    \li a value greater than zero signals the reception of a full
    message or a message fragment. See receiving large frames for details.

    <b>Receiving large frames:</b><br>
    The SimpleMQ protocol is frame based, but the function can return
    a fragment before the complete SimpleMQ frame is received if the
    frame sent by the peer is larger than the provided buffer. The
    frame length is returned in SMQ::frameLen and the data consumed
    thus far is returned in SMQ::bytesRead. The complete frame is
    consumed when frameLen == bytesRead.

    <b>Note:</b> the default timeout value is set to one minute. You
    can set the timeout value by setting SharkMQ::timeout to the
    number of milliseconds you want to wait for incoming messages
    before the timeout triggers. Note: Setting a long timeout may
    interfere with the built in PING timer.
 */
int SMQ_getMessage(SMQ* o, U8** msg);


/** Returns the message size, which is SMQ::frameLen - 15.
    \param o the SMQ instance.
 */
#define SMQ_getMsgSize(o) ((o)->frameLen-15)

/** @} */ /* end group SMQClient_C */ 

#ifdef __cplusplus
}
inline SMQ::SMQ(U8* buf, U16 bufLen) {
   SMQ_constructor(this,buf, bufLen);
}

inline int SMQ::init(const char* url, U32* rnd) {
   return SMQ_init(this, url, rnd);
}

inline int SMQ::connect(const char* uid, int uidLen, const char* credentials,
                        U8 credLen, const char* info, int infoLen) {
   return SMQ_connect(this,  uid, uidLen, credentials, credLen, info, infoLen);
}

inline void SMQ::disconnect() {
   return SMQ_disconnect(this);
}

inline SMQ::~SMQ() {
   SMQ_destructor(this);
}

inline int SMQ::create(const char* topic) {
   return SMQ_create(this, topic);
}

inline int SMQ::createsub(const char* subtopic) {
   return SMQ_createsub(this, subtopic);
}

inline int SMQ::subscribe(const char* topic) {
   return SMQ_subscribe(this, topic);
}

inline int SMQ::unsubscribe(U32 tid) {
   return SMQ_unsubscribe(this, tid);
}

inline int SMQ::publish(const void* data, int len, U32 tid, U32 subtid) {
   return SMQ_publish(this, data, len, tid, subtid);
}

inline int SMQ::wrtstr(const char* str) {
   return SMQ_wrtstr(this, str);
}

inline int SMQ::write( const void* data, int len) {
   return SMQ_write(this, data, len);
}

inline int SMQ::pubflush(U32 tid, U32 subtid) {
   return SMQ_pubflush(this, tid, subtid);
}

inline int SMQ::observe(U32 tid) {
   return SMQ_observe(this, tid);
}

inline int SMQ::unobserve(U32 tid) {
   return SMQ_unobserve(this, tid);
}

inline int SMQ::getMessage(U8** msg) {
   return SMQ_getMessage(this, msg);
}

inline int SMQ::getMsgSize() {
   return SMQ_getMsgSize(this);
}

#endif
 

/*
  The SharkMQ (secure SMQ) compatibility API makes it easy to write
  code that can later be upgraded to a secure version, if needed.
*/
#ifndef SHARKMQ_COMPAT
#define SHARKMQ_COMPAT 1
#endif

#if SHARKMQ_COMPAT
#define SharkMQ SMQ
#define SharkSslCon void
#define SharkMQ_constructor(o, buf, bufLen) SMQ_constructor(o, buf, bufLen)
#define SharkMQ_setCtx(o, ctx) SMQ_setCtx(o, ctx)
#define SharkMQ_init(o, scon, url, rnd) SMQ_init(o, url, rnd)
#define SharkMQ_connect(o, uid, uidLen, credentials, credLen, info, infoLen, NotApplicable) \
   SMQ_connect(o, uid, uidLen, credentials, credLen, info, infoLen)
#define SharkMQ_disconnect(o) SMQ_disconnect(o)
#define SharkMQ_destructor(o) SMQ_destructor(o)
#define SharkMQ_create(o, topic) SMQ_create(o, topic)
#define SharkMQ_createsub(o, subtopic) SMQ_createsub(o, subtopic)
#define SharkMQ_subscribe(o, topic) SMQ_subscribe(o, topic)
#define SharkMQ_unsubscribe(o, tid) SMQ_unsubscribe(o, tid)
#define SharkMQ_publish(o, data, len, tid, subtid) \
   SMQ_publish(o, data, len, tid, subtid)
#define SharkMQ_wrtstr(o, str) SMQ_wrtstr(o, str)
#define SharkMQ_write(o,  data, len) SMQ_write(o,  data, len)
#define SharkMQ_pubflush(o, tid, subtid) SMQ_pubflush(o, tid, subtid)
#define SharkMQ_observe(o, tid) SMQ_observe(o, tid)
#define SharkMQ_unobserve(o, tid) SMQ_unobserve(o, tid)
#define SharkMQ_getMessage(o, msg) SMQ_getMessage(o, msg)
#define SharkMQ_getMsgSize(o) SMQ_getMsgSize(o)
#endif


/** @} */ /* end group selib */

#endif
