package RTL.SMQ;

import java.util.*;
import java.net.*;
import java.io.*;
import javax.net.ssl.*;
import java.security.*;

/**
   The Simple Message Queue (SMQ) Java client provides a similar high
   level API as the JavaScript SMQ client. However, the use of the API
   differs in that the Java methods for publish, subscribe,
   etc. cannot be used when the client is not connected. An exception
   is thrown if you attempt to publish or subscribe if the client is
   not connected to a broker.
   <p>
   Each SMQ instance creates two threads, where one thread is used for
   upstream data and the other thread is used for downstream data. All
   user callbacks run in the context of the downstream thread. The
   callback action should be delegated in non thread safe UI code such
   as Swing and Android. See the utility classes {@link SwingSMQ} and
   {@link AndroidSMQ} for more information.
   <p>

   The SMQ instance will not garbage collect unless method
   {@link SMQ#close} is called, which terminates the two threads.
   
<p>
   All methods sending messages to the broker (such as publish) return
   immediately and before the message is sent to the broker. The
   message being sent is queued internally and sent one at a time by
   the upstream thread running in the background.

   @see <a href="https://realtimelogic.com/ba/doc/?url=SMQ.html">
   SMQ Overview</a>
   @see <a href="https://realtimelogic.com/ba/doc/?url=JavaScript/SMQ.html">
   JavaScript SMQ client</a>
*/
public class SMQ
{

  /**
     Create an SMQ client instance

     @param smqUrl The broker URL must be an HTTPS type URL. An
     exception is thrown for non secure HTTP URLs.

     @param trustMgr Optional TrustManager. The default TrustManager
     is used if this parameter is null. The utility class {@link TrustAny}
     provides a TrustManager that accepts any certificate, including
     self signed certificates.


     @param hostVerifier Optional HostnameVerifier. The default
     HostnameVerifier is used if this parameter is null. The utility
     class {@link TrustAny} provides a HostnameVerifier that accepts any
     domain/hostname.

     @param proxy Optional Proxy. The system default proxy (or none)
     is used if this parameter is null.

     @param onClose The client calls the user proved callback if the
     broker gracefully closed the connection, or if the connection
     unexpectedly closed.

     @see TrustAny
   */
  public SMQ(URL smqUrl, TrustManager[] trustMgr,
             HostnameVerifier hostVerifier, Proxy proxy, IntfOnClose onClose)
  {
    _smqUrl=smqUrl;
    _proxy = proxy;
    _trustMgr=trustMgr;
    _hostVerifier=hostVerifier;
    _onClose = onClose;
    _lock=this;
    _upstreamThread = new Thread() { public void run() {upstreamThreadFunc();} };
    _downstreamThread = new Thread() { public void run() {downstreamThreadFunc();} };
    _upstreamThread.start();
    _downstreamThread.start();
  }


  /**
     Returns the random number provided by the broker. The method can be
     called as soon as {@link SMQ#init} returns.
  */
  public final long getRand() { return _rand; }

  /**
     Returns the client's IP address as seen by the server. This
     address may not be the same as the client's IP address if the
     client is behind a NAT router. The client's IP address can be
     used by hash based authentication to further strengthen the hash
     value. The method can be called as soon as {@link SMQ#init}
     returns.
   */
  public final String getIpAddr() { return _ipAddr; }

  /** Returns the client's ephemeral topic ID.
   */
  public final long getETid() { return _etid; }

  /**
     The init method initiates a connection with the broker. The init
     method can optionally be called prior to calling {@link
     SMQ#connect} if data available from the following methods are
     required by the code calling connect: {@link
     SMQ#getRand} and {@link SMQ#getIpAddr}.

   */
  public void init() throws SmqException
  {
    _recTimeStamp = System.currentTimeMillis();
    _isRunning = false;
    if(_conState != 0)
      doEx(SmqException.INVALID_STATE);
    SSLContext sc=null;
    try { 
      sc = SSLContext.getInstance("SSL");
      sc.init(null,_trustMgr,null);
    }
    catch(NoSuchAlgorithmException e) { doEx(SmqException.SSL_NOT_SUPPORTED,e); }
    catch (KeyManagementException e) { doEx(SmqException.SSL_NOT_SUPPORTED,e); }
    SSLSocketFactoryWrapper factory =
      new SSLSocketFactoryWrapper(sc.getSocketFactory());
    HttpsURLConnection con=null;
    int status=0;
    try {
      con = (HttpsURLConnection)
        (_proxy == null ?
         _smqUrl.openConnection() :
         _smqUrl.openConnection(_proxy));
      con.setUseCaches(false);
      con.setDoInput(true);
      con.setDoOutput(true);
      con.setSSLSocketFactory(factory);
      if(_hostVerifier != null)
        con.setHostnameVerifier(_hostVerifier);
      con.setRequestProperty("SimpleMQ", "true");
      con.setRequestProperty("SendSmqHttpResponse", "true");
      status=con.getResponseCode();
    }
    catch(IOException e) { doEx(SmqException.CANNOT_CONNECT,e); }
    if(con.getHeaderField("SmqBroker") == null)
      doEx(SmqException.URL_NOT_A_BROKER);
    if(status != 200)
      doEx(SmqException.NON_200_RESPONSE_CODE);
    _sock = factory.getSocket();
    try { _sock.setTcpNoDelay(true); }
    catch(SocketException e) {}
    try { _is = new DataInputStream(_sock.getInputStream()); }
    catch(IOException e) { doEx(SmqException.CANNOT_CONNECT,e); }
    if(MSG_INIT != dispatchDownstreamMsg())
      doEx(SmqException.PROTOCOL_ERROR);
    _conState = 1;
  }

  /**
     Connect to the broker and morph (upgrade) the HTTPS connection to
     a persistent and secure (encrypted) TCP connection. Method {@link
     SMQ#init} is called by connect unless init was explicitly called
     prior to calling connect.

     @param credentials must be provided if the broker requires
     password based authentication and should be set to null if no
     authentication is required. See the <a
     href="https://realtimelogic.com/ba/doc/?url=JavaScript/SMQ.html#onauth">
     SMQ JavaScript client</a> for suggestions on how to manage
     authentication.

     @param info an optional string that may be used by server side code
     interacting with the broker.
   */
  public void connect(byte[] uid, String credentials, String info)
    throws SmqException
  {
    if(_conState == 0)
      init();
    else if(_conState != 1)
      doEx(SmqException.INVALID_STATE);
    try {
      OutMsg msg = new OutMsg(MSG_CONNECT);
      msg.dos.writeByte(_version);
      msg.dos.writeByte(uid.length);
      msg.dos.write(uid);
      msg.writeString(credentials, true);
      msg.writeString(info, false);
      msg.send(_sock);
    }
    catch(IOException e) { doEx(SmqException.DISCONNECT,e); }
    if(MSG_CONNACK != dispatchDownstreamMsg())
      doEx(SmqException.PROTOCOL_ERROR);
    _conState=2;
    _isRunning = true;
    _topic2tidM.put("self",_etid);
    _tid2topicM.put(_etid,"self");
    synchronized(_downstreamThread) { _downstreamThread.notify(); }
  }

  /** Returns true if the client is connected.
   */
  public boolean connected() {
    return _conState == 2;
  }

  /** Translates a topic name known to the client to topic ID (tid).

      @param topic topic name

      @return a non zero value if the topic is known to the client and
      zero if the topic is not known.

      @see SMQ#create
   */
  public long topic2tid(String topic)
  {
    synchronized(_lock) {
      Long x = _topic2tidM.get(topic);
      return x == null ? 0 : x.longValue();
    }
  }

  /** Translates a subtopic name known to the client to subtopic ID.

      @param subtopic name

      @return a non zero value if the subtopic is known to the client and
      zero if the topic is not known.

     @see SMQ#createsub
   */
  public long subtopic2tid(String subtopic)
  {
    synchronized(_lock) {
      Long x = _subtopic2tidM.get(subtopic);
      return x == null ? 0 : x.longValue();
    }
  }

  /** Translates topic ID known to the client to topic name.

      @param tid topic ID

      @return a string if the topic name is known to the client and
      null if the topic is not known.

      @see SMQ#create
   */
  public String tid2topic(long tid)
  {
    synchronized(_lock) {
      return _tid2topicM.get(tid);
    }
  }

  /** Translates subtopic ID known to the client to subtopic name.

      @param subtid subtopic ID

      @return a string if the subtopic name is known to the client and
      null if the subtopic is not known.

     @see SMQ#createsub
   */
  public String tid2subtopic(long subtid)
  {
    synchronized(_lock) {
      return _tid2subtopicM.get(subtid);
    }
  }

  /** 
      Create a topic and fetch the topic ID (tid). The SMQ protocol is
      optimized and does not directly use a string when publishing,
      but a number. The server randomly creates a 32 bit number and
      persistently stores the topic name and number. The 'create'
      method is typically used prior to publishing a message on a
      specific topic if the server logic implements
      authorization. Otherwise, the publish method can be used
      directly with a topic string since the publish method takes care
      of first creating a topic if you publish to a topic unknown to
      the client stack.
      <p>

      The create method is used internally by the publish methods
      (using topic name) if the topic is not known to the client when
      publish is called.

      @param topic the topic name where you plan on publishing messages.
      @param ack the on response callback.
   */
  public void create(String topic, IntfOnCreateAck ack)
    throws SmqException
  {
    create(topic, null, ack);
  }

  /** Create a topic name and sub topic name.
     @param topic the topic name where you plan on publishing messages.
     @param subtopic the secondary topic name.
     @param ack the on response callback.
     @see SMQ#createsub
   */
  public void create(String topic, String subtopic, IntfOnCreateAck ack)
    throws SmqException
  {
    createOrSub(topic, subtopic, null, ack);
  }

  /**
     @param subtopic
     @param ack the on response callback

   */
  public void createsub(final String subtopic, final IntfOnCreatsubeAck ack)
    throws SmqException
  {
    if(_conState != 2)
      doEx(SmqException.INVALID_STATE);
    if(subtopic == null)
      smqOnCreatesubAck(ack,true, subtopic, 0);
    else {
      long subtid = subtopic2tid(subtopic);
      if(subtid != 0)
        smqOnCreatesubAck(ack,true, subtopic, subtid);
      else {
        OnMsgAck action = new OnMsgAck() {
            public void action(boolean accepted, String topic, long tid) {
              assert subtopic == topic || accepted == false;
              if(accepted) {
                Long xtid=tid;
                synchronized(_lock) {
                  Long x = _subtopic2tidM.get(subtopic);
                  if(x == null) {
                    _subtopic2tidM.put(subtopic,xtid);
                    _tid2subtopicM.put(xtid,subtopic);
                    assert _tid2subtopicM.get(xtid) == null;
                  }
                  else {
                    assert x.equals(xtid);
                    assert subtopic.equals(_tid2subtopicM.get(xtid));
                  }
                }
              }
              smqOnCreatesubAck(ack,accepted, subtopic, tid);
            }
          };
        if(add2AckM(_createSubAckM, subtopic, action) == false) {
          try {add2UpstreamQ(new OutMsg(MSG_CREATESUB,subtopic));}
          catch(IOException e){}
        }
      }
    }
  }

  /**
     Subscribe to a named topic and to a named subtopic. You can
     subscribe multiple times to the same topic for different subtopic
     names.
     <p>
     The topic name "self" is interpreted as subscribing to the
     client's own Ephemeral Topic ID (etid) -- in other words, it
     means subscribing to the (tid) returned by method {@link
     SMQ#getETid}. Subscribing to your own Ephemeral Topic ID makes it
     possible for other connected clients to send a message directly
     to this client.

     @param topic the topic name to subscribe to. The topic name 'self'
     means subscribing to the client's own Ephemeral Topic ID.

     @param subtopic the subtopic name to subscribe to.

     @param msg the message callback is called for each message received
     from the broker.

     @param ack optional on subscribe ack callback. Set to null if not needed.
   */
  public void subscribe(String topic, String subtopic,
                        IntfOnMsg msg, IntfOnCreateAck ack)
    throws SmqException
  {
    if(ack == null) {
      ack = new IntfOnCreateAck() {
          public void smqOnCreateAck(final boolean accepted,
                                     final String topic, final long tid,
                                     final String subtopic,final long subtid){
          }
        };
    }
    createOrSub(topic, subtopic, msg, ack);
  }

  /**
     Subscribe to a named topic, but no subtopic and create a "catch
     all" for subtopics not subscribed to (including zero for no
     subtopic).
     <p>
     The topic name "self" is interpreted as subscribing to the
     client's own Ephemeral Topic ID (etid) -- in other words, it
     means subscribing to the (tid) returned by method {@link
     SMQ#getETid}. Subscribing to your own Ephemeral Topic ID makes it
     possible for other connected clients to send a message directly
     to this client.

     @param topic the topic name to subscribe to. The topic name 'self'
     means subscribing to the client's own Ephemeral Topic ID.

     @param msg the message callback is called for each message received
     from the broker.

     @param ack optional on subscribe ack callback. Set to null if not needed.

   */
  public void subscribe(String topic, IntfOnMsg msg, IntfOnCreateAck ack)
    throws SmqException
  {
    subscribe(topic, null, msg, ack);
  }

  /** Publish a message to a named topic and set subtopic ID to zero.
      @param topic the topic name is automatically translated to topic
      ID if the topic name is not known by the client.

     @param data the data to published.
   */
  public void publish(String topic, byte[] data)
    throws SmqException
  {
    publish(topic, null, data);
  }

  /** Publish a message to a named topic and set subtopic ID to zero.

      @param topic the topic name is automatically translated to topic
      ID if the topic name is not known by the client.

     @param msg the string to be published is converted to a UTF-8 data array.

     @see Msg#toString
   */
  public void publish(String topic, String msg)
    throws SmqException
  {
    publish(topic, null, msg);
  }

  /** Publish a message to a named topic and subtopic.

      @param topic the topic name is automatically translated to topic
      ID if the topic name is not known by the client.

      @param subtopic the subtopic name is automatically translated to
      subtopic ID if the subtopic name is not known by the client.

     @param msg the string to be publish is converted to a UTF-8 data array

     @see Msg#toString
   */
  public void publish(final String topic, final String subtopic, String msg)
    throws SmqException
  {
    try {
      publish(topic, subtopic, msg.getBytes("UTF-8"));
    }
    catch(UnsupportedEncodingException drop) {}
  }

  /** Publish a message to a named topic and subtopic.

      @param topic the topic name is automatically translated to topic
      ID if the topic name is not known by the client.

      @param subtopic the subtopic name is automatically translated to
      subtopic ID if the subtopic name is not known by the client.

     @param data the data to publish.
   */
  public void publish(final String topic, final String subtopic,
                      final byte[] data)
    throws SmqException
  {
    if(_conState != 2)
      doEx(SmqException.INVALID_STATE);
    long subtid=0;
    if( subtopic != null && (subtid=subtopic2tid(subtopic)) == 0 ) {
      IntfOnCreatsubeAck action = new IntfOnCreatsubeAck() {
          public void smqOnCreatesubAck(
           final boolean accepted, final String subtopic, final long subtid) {
            if(accepted) {
              try { publish(topic, subtid, data); }
              catch(SmqException drop) {}
            }
            else
              System.out.println("Dropping on pub "+topic+" : "+subtopic);
          }
        };
      createsub(subtopic, action);
    }
    else
      publish(topic, subtid, data);
  }

  /** Publish a message to a named topic and subtopic.

      @param topic the topic name is automatically translated to topic
      ID if the topic name is not known by the client.

     @param subtid the subtopic ID (from named subtopic). Set to zero
     if not used.

     @param data the data to publish.
   */
  public void publish(final String topic, final long subtid, final byte[] data)
    throws SmqException
  {
    if(_conState != 2)
      doEx(SmqException.INVALID_STATE);


    long tid=topic2tid(topic);
    if( tid == 0 ) {
      IntfOnCreateAck action = new IntfOnCreateAck() {
          public void smqOnCreateAck(final boolean accepted,
                                     final String topic, final long tid,
                                     final String na1, final long na2) {
            if(accepted) {
              try { publish(tid, subtid, data, 0, data.length); }
              catch(SmqException drop) {}
            }
            else
              System.out.println("Dropping on pub "+topic);
          }
        };
      create(topic, action);
    }
    else
      publish(tid, subtid, data, 0, data.length);


  }

  /** Publish a message to a (named topic or ephemeral topic ID) and
      to a named subtopic.
     @param tid the topic ID (from named topic) or ephemeral topic ID.
     @param subtid the subtopic ID (from named subtopic). Set to zero
     if not used.
     @param data the data to publish.
   */
  public void publish(long tid, long subtid, byte[] data)
    throws SmqException
  {
    publish(tid, subtid, data, 0, data.length);
  }

  /** Publish a message to a (named topic or ephemeral topic ID) and
      to a named subtopic.
     @param tid the topic ID (from named topic) or ephemeral topic ID.
     @param subtid the subtopic ID (from named subtopic). Set to zero
     if not used.
     @param b the data to published.
     @param off the start offset in the data.
     @param len the number of bytes to write.
   */
  public void publish(long tid, long subtid, byte[] b, int off, int len)
    throws SmqException
  {
    if(_conState != 2)
      doEx(SmqException.INVALID_STATE);
    try {
      OutMsg msg = new OutMsg(MSG_PUBLISH);
      msg.writeUnsignedInt(tid);
      msg.writeUnsignedInt(_etid);
      msg.writeUnsignedInt(subtid);
      msg.dos.write(b,off,len);
      add2UpstreamQ(msg);
    }
    catch(IOException e) { doEx(SmqException.DISCONNECT,e); }
  }

  /**
     Requests the server to unsubscribe the client from a topic. All
     registered callback interfaces, including all callbacks for
     subtopics, will be removed from the client stack.

     @param topic name

   */
  public void unsubscribe(String topic)
    throws SmqException
  {
    unsubscribe(topic2tid(topic));
  }

  /**
     Requests the server to unsubscribe the client from a topic. All
     registered callback interfaces, including all callbacks for
     subtopics, will be removed from the client stack.

     @param tid the topic ID (from named topic).
   */
  public void unsubscribe(long tid)
    throws SmqException
  {
    if(_conState != 2)
      doEx(SmqException.INVALID_STATE);
    if(tid != 0) {
      if(sendTidMsg(MSG_UNSUBSCRIBE, tid)) {
        _callbackSubM.remove(tid);
        _callbackM.remove(tid);
      }
    }
  }

  /** Requests the broker to provide change notification events when
      the number of subscribers to a specific topic changes.
     @param topic name
     @param ch the callback called when the number of subscribers changes.

   */
  public void observe(final String topic, final IntfOnChange ch)
    throws SmqException
  {
    long tid=topic2tid(topic);
    if(tid != 0)
      observe(tid, ch);
    else {
      IntfOnCreateAck ack = new IntfOnCreateAck() {
          public void smqOnCreateAck(final boolean accepted,
                                     final String topic, final long tid,
                                     final String subtopic, final long subtid) {
            if(accepted) {
              try { observe(tid, ch); }
              catch(SmqException e) {}
            }
          }
        };
      create(topic, ack);
    }
  }

  /**
     Requests the broker to provide change notification events when
     the number of subscribers to a specific topic changes. Ephemeral
     topic IDs can also be observed. The number of connected
     subscribers for an ephemeral ID can only be one, which means the
     client is connected. Receiving a change notification for an
     ephemeral ID means the client has disconnected and that you no
     longer will get any change notifications for the observed topic
     ID. The client stack automatically "unobserves" observed
     ephemeral topic IDs when it receives a change notification.

     @param tid the topic ID (from named topic) or ephemeral topic ID.
     @param ch the callback called when the number of subscribers changes.
   */
  public void observe(long tid, IntfOnChange ch)
    throws SmqException
  {
    if(_conState != 2)
      doEx(SmqException.INVALID_STATE);
    if(tid != 0 && sendTidMsg(MSG_OBSERVE, tid)) {
      LinkedList<IntfOnChange> l = _changeM.get(tid);
      if(l == null) {
        l = new  LinkedList<IntfOnChange>();
        _changeM.put(tid, l);
      }
      l.add(ch);
    }
  }

  /** Stop receiving change notifications for a named topic.
      @param topic name
   */
  public void unobserve(String topic)
    throws SmqException
  {
    unobserve(topic2tid(topic));
  }


  /**
     Stop receiving change notifications for a named topic or
     ephemeral topic ID.

     @param tid the topic ID (from named topic) or ephemeral topic ID.
   */
  public void unobserve(long tid)
    throws SmqException
  {
    if(_conState != 2)
      doEx(SmqException.INVALID_STATE);
    if(tid != 0 && sendTidMsg(MSG_UNOBSERVE, tid))
      _changeM.remove(tid);
  }


  /**
     Terminate the upstream and downstream threads and close the
     broker connection if the connection is established. The SMQ
     object can no longer be used after calling the close method.

     @param flush send all pending outstream messages prior to closing
     the connection.
     @param onClose optional callback
   */
  public void close(final boolean flush, final IntfOnClose onClose) {
      if (_downstreamThread != null) {
          _isRunning = false;
          synchronized (_downstreamThread) {
              Thread t = _downstreamThread;
              _downstreamThread = null;
              t.notify();
          }
          Runnable r = new Runnable() {
              public void run() {
                  synchronized (_upstreamThread) {
                      Thread t = _upstreamThread;
                      _upstreamThread = null;
                      t.notify();
                  }
                  if (flush && _conState == 2) {
                      try {
                          OutMsg msg = new OutMsg(MSG_DISCONNECT);
                          msg.send(_sock);
                      } catch (IOException ignore) {
                      }
                  }
                  sockClose();
                  if (onClose != null)
                      onClose.smqOnClose(null);
                  //System.out.println("sockClose");
              }
          };
          if (flush && _conState == 2)
              add2UpstreamQ(r);
          else {
              r.run();
              /*if (onClose != null)
                  onClose.smqOnClose(null);*/
          }
      } else if (onClose != null)
          onClose.smqOnClose(null);
  }

  private void createAndPut(Map<Long,LinkedList<IntfOnMsg>> map,
                            Long key,IntfOnMsg val)
  {
    LinkedList<IntfOnMsg> l = map.get(key);
    if(l == null) {
      l = new  LinkedList<IntfOnMsg>();
      map.put(key, l);
    }
    l.add(val);
  }

  private void createOrSub(final String topic, final String subtopic,
                           final IntfOnMsg onMsg, final IntfOnCreateAck ack)
    throws SmqException
  {
    if(_conState != 2)
      doEx(SmqException.INVALID_STATE);
    IntfOnCreatsubeAck sa = new IntfOnCreatsubeAck() {
        public void smqOnCreatesubAck(final boolean accepted,
                                      final String subtopic2, final long subtid)
        {
          assert subtopic == subtopic2 || accepted == false;
          if(onMsg == null) {  // If create
            long tid = topic2tid(topic);
            if(tid != 0) {
              smqOnCreateAck(ack,true, topic, tid, subtopic, subtid);
              return;
            }
          }
          OnMsgAck action = new OnMsgAck() {
              public void action(boolean accepted, String topic2, long tid) {
                assert topic == topic2 || accepted == false;
                if(accepted) {
                  Long xtid=tid;
                  synchronized(_lock) {
                    Long x = _topic2tidM.get(topic);
                    if(x == null) {
                      _topic2tidM.put(topic,xtid);
                      _tid2topicM.put(xtid,topic);
                      assert _tid2topicM.get(xtid) == null;
                    }
                    else {
                      assert x.equals(xtid);
                      assert topic.equals(_tid2topicM.get(xtid));
                    }
                    if(onMsg != null) { // If subscribe
                      if(subtopic == null) {
                        createAndPut(_callbackM, tid, onMsg);
                      }
                      else {
                        Map<Long,LinkedList<IntfOnMsg>> m = 
                          _callbackSubM.get(tid);
                        if(m == null) {
                          m = new HashMap<Long,LinkedList<IntfOnMsg>>();
                          _callbackSubM.put(tid, m);
                        }
                        createAndPut(m, subtid, onMsg);  
                      }
                    }
                  }
                }
                smqOnCreateAck(ack,accepted, topic, tid, subtopic, subtid);
              }
            };

          if(topic == "self") {
            if(onMsg != null) {
              action.action(true, topic, _etid);
            }
          }
          else if(add2AckM(onMsg == null ? _createAckM : _SubAckM,topic,action)
                  == false || onMsg != null) {
            try {
              add2UpstreamQ(new OutMsg(onMsg == null?
                                       MSG_CREATE:MSG_SUBSCRIBE,topic));
            }
            catch(IOException e) {
              smqOnCreateAck(ack,false,topic,0,subtopic,subtid);
            }
          }
        } // End func smqOnCreatesubAck
      };
    createsub(subtopic, sa);
  }

  void smqOnCreateAck(final IntfOnCreateAck ack,final boolean accepted,
                      final String topic, final long tid,
                      final String subtopic, final long subtid)
  {
    ack.smqOnCreateAck(accepted,topic, tid, subtopic, subtid);
  }

  void smqOnCreatesubAck(final IntfOnCreatsubeAck ca, final boolean accepted,
                         final String subtopic, final long subtid)
  {
    ca.smqOnCreatesubAck(accepted,subtopic,subtid);
  }

  void smqOnMsg(final IntfOnMsg om, final Msg msg)
  {
    om.smqOnMsg(msg);
  }

  void smqOnClose(IntfOnClose oc, SmqException e)
  {
    oc.smqOnClose(e);
  }

  void smqOnChange(IntfOnChange oc, final long subscribers, final long tid)
  {
    oc.smqOnChange(subscribers, tid);
  }


  private final void add2UpstreamQ(final OutMsg msg)
  {
    Runnable r = new Runnable() {
        public void run() {
          try { msg.send(_sock); }
          catch(IOException e){ manageUnexpectedClose(e); }
        }
      };
    add2UpstreamQ(r);
  }

  private final void add2UpstreamQ(Runnable r)
  {
    synchronized(_lock) { _upstreamQ.add(r); }
    synchronized(_upstreamThread) { _upstreamThread.notify(); }
  }

  private final void upstreamThreadFunc()
  {
    for(;;) {
      if(_upstreamThread == null)
        break;
      synchronized(_upstreamThread) {
        try { _upstreamThread.wait(60*1000); }
        catch(InterruptedException e) {continue;}
        if(_conState == 2 && System.currentTimeMillis() >
           (_pingTmo+_recTimeStamp)) {
          if(_pingActive) {
            long max=_pingTmo+_recTimeStamp+_pongRespTmo;
            if(System.currentTimeMillis() > max) {
              manageUnexpectedClose(new SmqException(SmqException.PONG_TMO));
            }
          }
          else {
            _pingActive=true;
            try {add2UpstreamQ(new OutMsg(MSG_PING));}
            catch(IOException ignore) {}
          }
        }
      }
      for(;;) {
        Runnable r=null;
        synchronized(_lock) {
          try { r=_upstreamQ.removeFirst(); }
          catch(NoSuchElementException e) { break; }
        }
        if(r==null)
          break;
        r.run();
      }
    }
    //System.out.println("Closing upstreamThreadFunc");
  }

  private final void downstreamThreadFunc()
  {
    for(;;) {
      if(_downstreamThread == null)
        break;
      synchronized(_downstreamThread) {
        try { _downstreamThread.wait(); }
        catch(InterruptedException e) {continue;}
      }
      //System.out.println("Activating downstreamThreadFunc");
      if(_downstreamThread != null) {
        while(_conState == 2) {
          try { dispatchDownstreamMsg(); }
          catch(SmqException e) {
            manageUnexpectedClose(e);
            break;
          }
        }
      }
    }
    //System.out.println("Closing downstreamThreadFunc");
  }

  private final void sockClose()
  {
    synchronized(_lock) {
      try { if(_sock != null) _sock.close(); }
      catch(IOException e) {}
      _sock=null;
      _is=null;
      _conState=0;
      _pingActive=false;
      _upstreamQ.clear();
      _createAckM.clear();
      _SubAckM.clear();
      _createSubAckM.clear();
      _topic2tidM.clear();
      _subtopic2tidM.clear();
      _tid2topicM.clear();
      _tid2subtopicM.clear();
      _callbackSubM.clear();
      _callbackM.clear();
      _changeM.clear();
    }
  }


  private final short dispatchDownstreamMsg() throws SmqException
  {
    long tid;
    int len = readUnsignedShort();
    short msgType = (short)readUnsignedByte();
    len -= 3;
    //System.out.println("MSG : "+msgType);
    _recTimeStamp = System.currentTimeMillis();
    _pingActive=false;
    switch(msgType) {
    case MSG_INIT:
      if(readUnsignedByte() != _version)
        doEx(SmqException.PROTOCOL_ERROR);
      _rand = readUnsignedInt();
      _ipAddr = readString(len-5);
      break;

    case MSG_CONNACK:
      Exception ex=null;
      short rspCode=(short)readUnsignedByte();
      _etid=readUnsignedInt();
      len -= 5;
      if(len > 0)
        ex = new Exception(readString(len));
      if(rspCode != 0)
        doEx(rspCode, ex);
      break;

    case MSG_SUBACK:
    case MSG_CREATEACK:
    case MSG_CREATESUBACK:
      boolean accepted = readUnsignedByte() == 0 ? true : false;
      tid=readUnsignedInt();
      String topic = readString(len-5);
      runOnAck(msgType == MSG_SUBACK ? _SubAckM :
               (msgType == MSG_CREATEACK ? _createAckM : _createSubAckM),
               accepted, topic, tid);
      break;

    case MSG_PUBLISH:
      tid=readUnsignedInt();
      long ptid=readUnsignedInt();
      long subtid=readUnsignedInt();
      len -= 12;
      runOnMsg(ptid,tid,subtid, len > 0 ? readBytes(len) : null);
      break;

    case MSG_DISCONNECT:
      Exception e=null;
      if(len > 0) {
        try {e = new Exception(readString(len)); }
        catch(SmqException ignore) {}
      }
      doEx(SmqException.SERVER_DISCONNECT, e);

    case MSG_PING:
      try { add2UpstreamQ(new OutMsg(MSG_PONG)); }
      catch(IOException ignore) {}
      break;

    case MSG_PONG:
      // Do nothing: _recTimeStamp and _pingActive managed above
      break;

    case MSG_CHANGE:
      tid=readUnsignedInt();
      manageOnChange(readUnsignedInt(), tid);
      break;

      default:
        doEx(SmqException.PROTOCOL_ERROR);
    }
    return msgType;
  }

  private final boolean sendTidMsg(short msgType, long tid)
  {
    try {
      OutMsg msg = new OutMsg(msgType);
      msg.writeUnsignedInt(tid);
      add2UpstreamQ(msg);
      return true;
    }
    catch(IOException e) { manageUnexpectedClose(e); }
    return false;
  }
  

  private final void manageOnChange(long subscribers, long tid)
  {
    LinkedList<IntfOnChange> l;
    synchronized(_lock) {
      l = _changeM.get(tid);
    }
    if(l != null) {
      for(IntfOnChange on : l)
        smqOnChange(on, subscribers, tid);
      if(tid2topic(tid) == null) { // if ephemeral ID
        assert subscribers == 0;
        synchronized(_lock) {
          _changeM.remove(tid);
        }
      }
    }
  }

  private final int readUnsignedShort() throws SmqException
  {
    if(_is == null) return 0; //Closed
    try { return _is.readUnsignedShort(); }
    catch(IOException e) { doEx(SmqException.DISCONNECT,e); }
    return 0; //Make compiler happy
  }

  private final int readUnsignedByte() throws SmqException
  {
    if(_is == null) return 0; //Closed
    try { return _is.readUnsignedByte(); }
    catch(IOException e) { doEx(SmqException.DISCONNECT,e); }
    return 0; //Make compiler happy
  }

  private final long readUnsignedInt() throws SmqException
  {
    if(_is == null) return 0; //Closed
    try { return _is.readInt() & 0xFFFFFFFFL; }
    catch(IOException e) { doEx(SmqException.DISCONNECT,e); }
    return 0; //Make compiler happy
  }

  private final String readString(int len) throws SmqException
  {
    if(_is == null) return ""; //Closed
    try { return new String(readBytes(len), "UTF-8"); }
    catch(UnsupportedEncodingException e) { doEx(SmqException.DISCONNECT,e); }
    return null; //Make compiler happy
  }

  private final void doEx(short reason) throws SmqException
  {
    sockClose();
    throw new SmqException(reason);
  }
  private final void doEx(short reason,Throwable cause)  throws SmqException
  {
    sockClose();
    throw new SmqException(reason,cause);
  }

  private final void manageUnexpectedClose(Throwable cause)
  {
    manageUnexpectedClose(new SmqException(SmqException.DISCONNECT, cause));
  }

  private final void manageUnexpectedClose(SmqException e)
  {
    boolean isRunning;
    synchronized(_lock) {
      isRunning = _isRunning;
      _isRunning=false;
    }
    if(isRunning) {
      sockClose();
      smqOnClose(_onClose, e);
    }
  }

  private final byte[] readBytes(int len) throws SmqException
  {
    if(_is == null) return null; //Closed
    try {
      byte[] b = new byte[len];
      int offset=0;
      while(len != 0) {
        int n = _is.read(b, offset, len);
        if(n < 1) doEx(SmqException.DISCONNECT);
        offset += n;
        len -= n;
        assert len >= 0;
      }
      return b; 
    }
    catch(IOException e) { doEx(SmqException.DISCONNECT,e); }
    return null; //Make compiler happy
  }

  private final boolean add2AckM(Map<String, LinkedList<OnMsgAck>> onAck,
                               String topic, OnMsgAck value)
  {
    synchronized(_lock) {
      boolean found;
      LinkedList<OnMsgAck> lr = onAck.get(topic);
      if(lr == null) {
        lr = new LinkedList<OnMsgAck>();
        onAck.put(topic, lr);
        found=false;
      }
      else
        found=true;
      lr.addFirst(value);
      //System.out.println("add2AckM "+onAck+" : topic="+topic);
      return found;
    }
  }

  // Lookup key in Map and call the user callback smqOnMsg if found.
  // Used by runOnMsg below
  private boolean runOnMsg(long key,Map<Long,LinkedList<IntfOnMsg>> map,Msg msg)
  {
    IntfOnMsg[] a = null;
    synchronized(_lock) {
      LinkedList<IntfOnMsg> l = map.get(key);
      if(l != null)
        a = l.toArray(new IntfOnMsg[0]);
    }
    //System.out.println("runOnMsg2 key "+key+", "+a);
    if(a != null) {
      for(IntfOnMsg on : a)
        smqOnMsg(on, msg);
      return true;
    }
    return false;
  }

  // MSG_PUBLISH -> execute callback 'smqOnMsg' if found for tid/subtid
  private final void runOnMsg(long ptid, long tid, long subtid, byte[] data)
  {
    boolean found=false;
    Msg msg = new Msg(ptid, tid, subtid, data);
    if(subtid != 0) {
      Map<Long,LinkedList<IntfOnMsg>> m = null;
      synchronized(_lock) {
        m = _callbackSubM.get(tid);
      }
      if(m != null)
        found=runOnMsg(subtid, m, msg);
      //System.out.println("runOnMsg subtid "+tid+", "+subtid+", "+m+", "+found);
    }
    if(found == false) {
      if( ! runOnMsg(tid, _callbackM, msg) ) {
        System.out.println("Dropping msg "+tid);
      }
    }
  }


  private final void runOnAck(Map<String, LinkedList<OnMsgAck>> onAck,
                              boolean accepted, String topic, long tid)
  {
    LinkedList<OnMsgAck> lr;
    synchronized(_lock) {
      lr = onAck.get(topic);
      if(lr != null)
        onAck.remove(topic);
    }
    if(lr != null) {
      for(OnMsgAck ack : lr)
        ack.action(accepted,topic,tid);
    }
  }


  private class OutMsg {
    ByteArrayOutputStream baos;
    DataOutputStream dos;
    public OutMsg(int msgType) throws IOException
    {
      baos = new ByteArrayOutputStream();
      dos = new DataOutputStream(baos); 
      dos.writeShort(0); // Space for size
      dos.writeByte(msgType);
    }
    public OutMsg(int msgType, String s) throws IOException
    {
      this(msgType);
      writeString(s, false);
    }

    final void writeUnsignedInt(long i) throws IOException
    {
      dos.writeInt((int)i);
    }

    final void writeString(String s, boolean setLen) throws IOException
    {
      if(s != null) {
        byte[] bc = s.getBytes("UTF-8");
        if(setLen)
          dos.writeByte(bc.length);
        dos.write(bc);
      }
      else if(setLen)
        dos.writeByte(0);
    }

    final void send(Socket sock) throws IOException
    {
      if(sock == null)
        return;
      dos.flush();
      byte[] b = baos.toByteArray();
      int len = b.length;
      if(len > 0xFFFF)
        throw new IOException("Message overflow");
      b[0] = (byte)(len >> 8);
      b[1] = (byte)len;
      sock.getOutputStream().write(b);
    }
  };

  interface OnMsgAck
  {
    public void action(boolean accepted, String topic, long tid);
  };

  private URL _smqUrl;
  private Proxy _proxy;
  private TrustManager[] _trustMgr;
  private HostnameVerifier _hostVerifier;
  private Socket _sock=null;
  private DataInputStream _is;
  private long _rand;
  private String _ipAddr;
  private short _conState=0; // 0: not connected, 1: init, 2: connected.
  private long _etid=0;
  private Thread _upstreamThread=null;
  private Thread _downstreamThread=null;
  // Set to false in init,manageUnexpectedClose and true at end of connect.
  private boolean _isRunning=false;
  private Object _lock;
  private IntfOnClose _onClose;
  private long _recTimeStamp=0;
  private boolean _pingActive=false;

  private LinkedList<Runnable> _upstreamQ = new LinkedList<Runnable>();
  private Map<String,LinkedList<OnMsgAck>> _createAckM = // MSG_CREATEACK
    new HashMap<String,LinkedList<OnMsgAck>>();
  private Map<String,LinkedList<OnMsgAck>> _SubAckM = // MSG_SUBACK
    new HashMap<String,LinkedList<OnMsgAck>>();
  private Map<String,LinkedList<OnMsgAck>> _createSubAckM = // MSG_CREATESUBACK
    new HashMap<String,LinkedList<OnMsgAck>>();
  private Map<String,Long> _topic2tidM = new HashMap<String,Long>();
  private Map<String,Long> _subtopic2tidM = new HashMap<String,Long>();
  private Map<Long,String> _tid2topicM = new HashMap<Long,String>();
  private Map<Long,String> _tid2subtopicM = new HashMap<Long,String>();

  private Map<Long,Map<Long,LinkedList<IntfOnMsg>>> _callbackSubM =
    new HashMap<Long,Map<Long,LinkedList<IntfOnMsg>>>();
  private Map<Long,LinkedList<IntfOnMsg>> _callbackM =
    new HashMap<Long,LinkedList<IntfOnMsg>>();
  private Map<Long,LinkedList<IntfOnChange>> _changeM =
    new HashMap<Long,LinkedList<IntfOnChange>>();

  private static final short _version       = 1;
  private static final long _pingTmo        = 20 * 60 * 1000;
  private static final long _pongRespTmo    = 20 * 1000;

  private static final short MSG_INIT         = 1;
  private static final short MSG_CONNECT      = 2;
  private static final short MSG_CONNACK      = 3;
  private static final short MSG_SUBSCRIBE    = 4;
  private static final short MSG_SUBACK       = 5;
  private static final short MSG_CREATE       = 6;
  private static final short MSG_CREATEACK    = 7;
  private static final short MSG_PUBLISH      = 8;
  private static final short MSG_UNSUBSCRIBE  = 9;
  private static final short MSG_DISCONNECT   = 11;
  private static final short MSG_PING         = 12;
  private static final short MSG_PONG         = 13;
  private static final short MSG_OBSERVE      = 14;
  private static final short MSG_UNOBSERVE    = 15;
  private static final short MSG_CHANGE       = 16;
  private static final short MSG_CREATESUB    = 17;
  private static final short MSG_CREATESUBACK = 18;
  private static final short MSG_PUBFRAG      = 19;

}

