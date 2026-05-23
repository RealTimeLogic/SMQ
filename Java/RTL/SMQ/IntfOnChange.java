package RTL.SMQ;

/** Callback interface for 'change' events sent by the broker.
 */
public interface IntfOnChange
{
  /**
     The method is called when the broker sends a change notification event.

     @param subscribers the number of clients subscribed to the topic.
     @param tid the topic name requested or the ephemeral topic ID
     requested to be observed.
   */
  public void smqOnChange(final long subscribers, final long tid);
};
