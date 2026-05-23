package RTL.SMQ;

/** Callback interface for 'close' events.
 */
public interface IntfOnClose
{
  /** Called if the broker requested the connection to be closed
      (graceful disconnect) or if the persistent TCP connection should
      disconnect.
      @param e reason for the disconnect.
   */
  public void smqOnClose(SmqException e);
};
