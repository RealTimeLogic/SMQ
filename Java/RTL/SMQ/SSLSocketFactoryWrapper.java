
package RTL.SMQ;

import java.net.*;
import java.io.*;
import javax.net.ssl.*;


class SSLSocketFactoryWrapper extends SSLSocketFactory
{
   
  public SSLSocketFactoryWrapper(SSLSocketFactory factory)
  {
    _df = factory;
  }


  public Socket createSocket(Socket socket, String host, int port, boolean autoClose)
    throws IOException
  {
    return setup(_df.createSocket(socket, host, port, autoClose));
  }

  public Socket createSocket() throws IOException
  {
    return setup(_df.createSocket());
  }

  public Socket createSocket(String host, int port)
    throws IOException, UnknownHostException
  {
    return setup(_df.createSocket(host, port));
  }

  public Socket createSocket(String host, int port, InetAddress localAddr, int localPort)
    throws IOException, UnknownHostException
  {
    return setup(_df.createSocket(host, port, localAddr, localPort));
  }

  public Socket createSocket(InetAddress address, int port) throws IOException
  {
    return setup(_df.createSocket(address, port));
  }

  public Socket createSocket(InetAddress address, int port,
                             InetAddress localAddr, int localPort)
    throws IOException
  {
    return setup(_df.createSocket(address, port, localAddr, localPort));
  }
   
  public String[] getDefaultCipherSuites(){
    return _df.getDefaultCipherSuites();
  }
   
  public String[] getSupportedCipherSuites(){
    return _df.getSupportedCipherSuites();
  }

  Socket getSocket()
  {
    return _socket;
  }

  private Socket setup(Socket s)
  {
    _socket = s;
    return s;
  }

  private Socket _socket=null;
  SSLSocketFactory _df;
}
