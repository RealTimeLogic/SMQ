package RTL.SMQ;

/** Callback interface for responses to 'createsub' messages sent to the broker.
 */
public interface IntfOnCreatsubeAck {
  /** Called when the broker sends the CreatesubAck message to the client.
      @param accepted set to 'true' if the broker accepted the request.
      @param subtopic the subtopic requested.
      @param subtid the translated subtopic ID created by the broker.
   */
  public void smqOnCreatesubAck(final boolean accepted,
                                final String subtopic, final long subtid);
};
