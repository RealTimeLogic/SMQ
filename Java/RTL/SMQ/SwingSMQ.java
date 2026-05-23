package RTL.SMQ;
import javax.swing.SwingUtilities;
import java.net.*;
import javax.net.ssl.*;
import java.security.*;
 
/**
   SwingSMQ is specifically designed for being used in a Swing GUI
   application. Swing applications are not thread safe and SMQ can,
   therefore, be difficult to use together with Swing
   applications. The SwingSMQ class is designed such that the events
   triggered by SMQ are run in the context of the Swing GUI
   thread. This means that you do not have to do any thread
   synchronization with SwingSMQ.
 */
public class SwingSMQ extends SMQ
{
  /** See {@link SMQ#SMQ} for details.
   */
  public SwingSMQ(URL smqUrl, TrustManager[] trustMgr,
             HostnameVerifier hostVerifier, Proxy proxy, IntfOnClose onClose)
  {
    super(smqUrl, trustMgr, hostVerifier, proxy, onClose);
  }

  /** See SMQ.close for details
   */
  @Override
  public void close(final boolean flush, final IntfOnClose onClose)
  {
    IntfOnClose onClose2;
    if(onClose != null) {
      onClose2 = new IntfOnClose() {
          public void smqOnClose(final SmqException e) {
            Runnable r = new Runnable() {
                public void run() {
                  onClose.smqOnClose(e);
                }
              };
            SwingUtilities.invokeLater(r);
          }
        };
    }
    else
      onClose2=onClose;
    super.close(flush,onClose2);
  }

    
  @Override
  void smqOnCreateAck(final IntfOnCreateAck ack,final boolean accepted,
                      final String topic, final long tid,
                      final String subtopic, final long subtid)
  {
    Runnable r = new Runnable() {
        public void run() {
          ack.smqOnCreateAck(accepted,topic, tid, subtopic, subtid);
        }
      };
    SwingUtilities.invokeLater(r);
  }

  @Override
  void smqOnCreatesubAck(final IntfOnCreatsubeAck ca, final boolean accepted,
                         final String subtopic, final long subtid)
  {
    Runnable r = new Runnable() {
        public void run() {
          ca.smqOnCreatesubAck(accepted,subtopic,subtid);
        }
      };
    SwingUtilities.invokeLater(r);
  }

  @Override
  void smqOnMsg(final IntfOnMsg om, final Msg msg)
  {
    Runnable r = new Runnable() {
        public void run() {
          om.smqOnMsg(msg);
        }
      };
    SwingUtilities.invokeLater(r);
  }

  @Override
  void smqOnChange(final IntfOnChange oc,final long subscribers,final long tid)
  {
    Runnable r = new Runnable() {
        public void run() {
          oc.smqOnChange(subscribers, tid);
        }
      };
    SwingUtilities.invokeLater(r);
  }

  @Override
  void smqOnClose(final IntfOnClose oc, final SmqException e)
  {
    Runnable r = new Runnable() {
        public void run() {
          oc.smqOnClose(e);
        }
      };
    SwingUtilities.invokeLater(r);
  }



};
