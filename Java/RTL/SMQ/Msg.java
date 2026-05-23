package RTL.SMQ;

import java.io.*;

/** Messages received from the broker are encapsulated in Msg instances.
 */
public class Msg
{
  Msg(long ptid, long tid, long subtid, byte[] data)
  {
    _ptid=ptid;
    _tid=tid;
    _subtid=subtid;
    _data=data;
  }

  /** Returns the raw data sent by the broker.
   */
  public final byte[] getData()
  {
    return _data;
  }

  /** Attempt to convert the data from raw UTF-8 to a Java string.
      @return the converted string or null if the data cannot be converted.
     @see SMQ#publish(String topic, String msg)
   */
  public final String toString()
  {
    try { return new String(_data, "UTF-8"); }
    catch(UnsupportedEncodingException ignore) { }
    return null;
  }

  /** Returns the publisher's ephemeral topic ID, i.e. the sender's address.
   */
  public final long getPTid()
  {
    return _ptid;
  }

  /** Returns the topic ID for which this message was published to.
   */
  public final long getTid()
  {
    return _tid;
  }

  /** Returns the subtopic ID for which this message was published to
      or zero if subtopic was not set by the publisher.
   */
  public final long getSubTid()
  {
    return _subtid;
  }

  private long _ptid; // Publisher's ephemeral topic ID.
  private long _tid;
  private long _subtid;
  private byte[] _data;
};
