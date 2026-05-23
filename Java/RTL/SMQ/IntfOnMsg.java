package RTL.SMQ;

/** Callback interface for receiving messages from the broker.
 */
public interface IntfOnMsg
{
  /** Called each time a new message arrives.
      @param msg the SMQ message received.
  */
  public void smqOnMsg(Msg msg);
};
