package RTL.SMQ;

import java.security.*;
import java.security.cert.*;
import javax.net.ssl.*;  

/** Utility class that provides a TrustManager and HostnameVerifier
    that accepts any certificate and hostname. This class is typically
    used during development and when connecting to a broker using a
    self signed certificate.
 */
public class TrustAny
{

  /**
     Returns a TrustManager set that enables the SMQ client to accept
     any server certificate.
   */
  static public TrustManager[] cert()
  {
    TrustManager[] trustAllCerts = new TrustManager[] {
      new X509TrustManager() {
        public X509Certificate[] getAcceptedIssuers() 
        {
          return null;
        }
        public void checkServerTrusted(X509Certificate[] certs,
                                       String authType)
          throws CertificateException
        {
          return;
        }
        public void checkClientTrusted(X509Certificate[] certs,
                                       String authType)
          throws CertificateException
        {
          return;
        }
      }//X509TrustManager
    };//TrustManager[]
    
    return trustAllCerts;
  }

  /**
     Returns a HostnameVerifier that enables the SMQ client to accept
     any domain/hostname.
   */
  static public HostnameVerifier hostName()
  {
    HostnameVerifier verifier = new HostnameVerifier() {
        public boolean verify(String hostname, SSLSession session)
        {
          return true;
        }
      };
    return verifier;
  }
};

