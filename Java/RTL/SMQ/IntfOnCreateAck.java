package RTL.SMQ;

/** Callback interface for responses to 'create' messages sent to the broker.
 */
public interface IntfOnCreateAck{
  /** Called when the broker sends the CreateAck message to the client.
      @param accepted set to 'true' if the broker accepted the request.
      @param topic the topic requested.
      @param tid the translated topic ID created by the broker.
      @param subtopic the subtopic requested or null if no subtopic
      was requested.
      @param subtid the translated subtopic ID created by the broker
      or zero if no subtopic was requested.
   */
  public void smqOnCreateAck(final boolean accepted,
                             final String topic, final long tid,
                             final String subtopic, final long subtid);
};
