package RTL.SMQ;

import android.content.Context;
import android.os.Handler;
import android.os.Looper;

import java.net.Proxy;
import java.net.URL;

import javax.net.ssl.HostnameVerifier;
import javax.net.ssl.TrustManager;


/**
 * AndroidSMQ is specifically designed for being used in an Android
 * GUI application. Android applications are not thread safe and SMQ
 * can, therefore, be difficult to use together with Android
 * applications. The AndroidSMQ class is designed such that the events
 * triggered by SMQ are run in the context of the Android GUI
 * thread. This means that you do not have to do any thread
 * synchronization with AndroidSMQ.
 */

public class AndroidSMQ extends SMQ {

    public interface OnSmqConnectionListener {
        void onSmqConnected();

        void onSmqConnectionErr(SmqException e);
    }
    public interface OnSmqCloseListener{
        void onSmqClosed();
    }

    /**
     * See {@link SMQ#SMQ} for details.
     */

    public AndroidSMQ(URL smqUrl, TrustManager[] trustMgr,
                      HostnameVerifier hostVerifier, Proxy proxy, IntfOnClose onClose) {
        super(smqUrl, trustMgr, hostVerifier, proxy, onClose);
    }

    public void connectAsync(final byte[] uid, final String credentials, final String info, final OnSmqConnectionListener onSmqConnectionListener) {
        Thread thread = new Thread() {
            @Override
            public void run() {
                try {
                    connect(uid, credentials, info);
                    if (onSmqConnectionListener != null) {
                        mainHandler.post(new Runnable() {
                            @Override
                            public void run() {
                                onSmqConnectionListener.onSmqConnected();
                            }
                        });

                    }
                } catch (final SmqException e) {
                    if (onSmqConnectionListener != null) {
                        mainHandler.post(new Runnable() {
                            @Override
                            public void run() {
                                onSmqConnectionListener.onSmqConnectionErr(e);
                            }
                        });

                    } else {
                        e.printStackTrace();
                    }
                }
            }
        };

        thread.start();
    }

    /** See SMQ.close for details.
     */
    @Override
    public void close(final boolean flush, final IntfOnClose onClose) {

        Thread thread = new Thread() {
            @Override
            public void run() {
                IntfOnClose onClose2;
                if (onClose != null) {
                    onClose2 = new IntfOnClose() {
                        public void smqOnClose(final SmqException e) {
                            Runnable r = new Runnable() {
                                public void run() {
                                    onClose.smqOnClose(e);
                                }
                            };
                            mainHandler.post(r);
                        }
                    };
                } else
                    onClose2 = onClose;
                AndroidSMQ.super.close(flush, onClose2);
            }

        };

        thread.start();
    }


    @Override
    void smqOnCreateAck(final IntfOnCreateAck ack, final boolean accepted,
                        final String topic, final long tid,
                        final String subtopic, final long subtid) {
        Runnable r = new Runnable() {
            public void run() {
                ack.smqOnCreateAck(accepted, topic, tid, subtopic, subtid);
            }
        };
        mainHandler.post(r);
    }

    @Override
    void smqOnCreatesubAck(final IntfOnCreatsubeAck ca, final boolean accepted,
                           final String subtopic, final long subtid) {
        Runnable r = new Runnable() {
            public void run() {
                ca.smqOnCreatesubAck(accepted, subtopic, subtid);
            }
        };
        mainHandler.post(r);
    }

    @Override
    void smqOnMsg(final IntfOnMsg om, final Msg msg) {
        Runnable r = new Runnable() {
            public void run() {
                om.smqOnMsg(msg);
            }
        };
        mainHandler.post(r);
    }

    @Override
    void smqOnChange(final IntfOnChange oc, final long subscribers, final long tid) {
        Runnable r = new Runnable() {
            public void run() {
                oc.smqOnChange(subscribers, tid);
            }
        };
        mainHandler.post(r);
    }

    @Override
    void smqOnClose(final IntfOnClose oc, final SmqException e) {
        Runnable r = new Runnable() {
            public void run() {
                oc.smqOnClose(e);
            }
        };
        mainHandler.post(r);
    }

    Handler mainHandler = new Handler(Looper.getMainLooper());

};
