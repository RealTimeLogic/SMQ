package RTL.SMQ;

public class SmqException extends Exception
{
  SmqException(short reason)
  {
    super();
    _reason = reason;
    _cause = null; 
  }
  SmqException(short reason,Throwable cause)
  {
    super();
    _reason = reason;
    _cause = cause;
  }

  public String toString()
  {
    String result = "Code: " + _reason;
    if(_cause != null)
      result = result + " - " + _cause.toString();
    return result;
  } 

  public short getReason() {
    return _reason;
  }

  public Throwable getCause() {
    return _cause;
  }

  public static final short UNACCEPTABLE_PROTOCOL_VERSION   = 0x01;
  public static final short SERVER_UNAVAILABLE              = 0x02;
  public static final short INCORRECT_CREDENTIALS           = 0x03; 
  public static final short CLIENT_CERTIFICATE_REQUIRED     = 0x04;
  public static final short CLIENT_CERTIFICATE_NOT_ACCEPTED = 0x05;
  public static final short ACCESS_DENIED                   = 0x06;
  
  public static final short CANNOT_CONNECT             = 101;
  public static final short DISCONNECT                 = 102;
  public static final short INVALID_STATE              = 103;
  public static final short NON_200_RESPONSE_CODE      = 104;
  public static final short PONG_TMO                   = 105;
  public static final short PROTOCOL_ERROR             = 106;
  public static final short SERVER_DISCONNECT          = 107;
  public static final short SSL_NOT_SUPPORTED          = 108;
  public static final short URL_NOT_A_BROKER           = 109;
  public static final short INVALID_ARG                = 110;


  private short _reason=0;
  private Throwable _cause=null;
};
