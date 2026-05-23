
import java.awt.*;
import java.awt.event.*;
import java.net.*;
import java.io.*;
import java.util.*;
import javax.swing.*;
import RTL.SMQ.*;
import org.json.*;
import eu.hansolo.steelseries.extras.Led;
import eu.hansolo.steelseries.tools.LedColor;

interface SetLedIntf {
  public void state(boolean on);
}

interface DevIntf {
  public void setLed(int ledId, boolean on);
  public void removeDev();
}


public class LedSMQ extends JFrame  implements IntfOnClose {

  public LedSMQ()
  {
    initUI();
    try {
      _macAddr = NetworkInterface.getByInetAddress(
                    InetAddress.getLocalHost()).getHardwareAddress();
      connect();
    }
    catch (UnknownHostException e) {}
    catch (SocketException e) {}
  }

  //For connecting and reconnecting to the broker.
  private boolean connect()
  {
    try {
      URL url=null;
      try {url = new URL("https://simplemq.com/smq.lsp"); }
      catch(MalformedURLException e) {System.exit(2);}
      _smq = new SwingSMQ(url,
                          null, //TrustAny.cert(),
                          null, //TrustAny.hostName(),
                          null,
                          this);
      _smq.connect(_macAddr, null, null);
      IntfOnMsg onDevInfoCB = new IntfOnMsg() {
          public void smqOnMsg(Msg msg) { newDevice(msg); }
        };
      //When a new device broadcasts to all connected "display" units.
      _smq.subscribe("/m2m/led/device", "devinfo", onDevInfoCB, null);
      //When a device responds to our "/m2m/led/display" published message.
      _smq.subscribe("self", "devinfo", onDevInfoCB, null);

      //When a device publishes LED state change.
      _smq.subscribe("/m2m/led/device", "led", new IntfOnMsg() {
          public void smqOnMsg(Msg msg) { ledStateChange(msg); }
        }, null);

      /* Broadcast to all connected devices.
         Device will then send 'info' to our ptid ("self"), sub-tid: "devinfo".
      */
      _smq.publish("/m2m/led/display", "Hello");
    }
    catch(SmqException e) {
      System.out.println("Connect err: "+e.toString());
      if(e.getReason() == SmqException.SERVER_DISCONNECT)
        System.exit(1); //Not allowed to reconnect
      return false;
    }
    return true;
  }


  private void initUI()
  {
    JPanel pane = (JPanel)getContentPane();
    _tabbedPane.setTabPlacement(SwingConstants.LEFT);
    pane.add(_tabbedPane);
    setTitle("SMQ LED Demo");
    setSize(350, 600);
    setLocationRelativeTo(null);
    setDefaultCloseOperation(EXIT_ON_CLOSE);
    setVisible(true);
  }

  //Decode JSON data describing the device and create UI LEDs based on this data
  private void newDevice(Msg msg)
  {
    final Map<Integer, SetLedIntf> devLedM = new HashMap<Integer, SetLedIntf>();
    final long devId = msg.getPTid();
    final JPanel pane = new JPanel();

    BoxLayout layout = new BoxLayout(pane, BoxLayout.PAGE_AXIS);
    pane.setLayout(layout);

    JSONObject j = new JSONObject(msg.toString());
    JSONArray jleds = j.getJSONArray("leds");
    for(int i = 0 ; i < jleds.length() ; i++) {
      JSONObject jled = jleds.getJSONObject(i);
      final int ledId = jled.getInt("id");
      final Led led = new Led();
      led.setPreferredSize(new Dimension(70, 70));
      final JToggleButton button = new JToggleButton(jled.getString("name"));
      button.setPreferredSize(new Dimension(100, 25));
      if(jled.getBoolean("on")) {
        button.setSelected(true);
        led.setLedOn(true);
      }
      button.addItemListener(new ItemListener() {
          public void itemStateChanged(ItemEvent ev) {
            setLedState(devId,ledId, ev.getStateChange()==ItemEvent.SELECTED);
          }
        });
      JPanel rpane = new JPanel();
      rpane.add(button);
      switch(jled.getString("color")) {
      case "yellow": led.setLedColor(LedColor.YELLOW_LED); break;
      case "green": led.setLedColor(LedColor.GREEN_LED); break;
      case "blue": led.setLedColor(LedColor.BLUE_LED); break;
      default: led.setLedColor(LedColor.RED_LED);
      }
      rpane.add(led);
      pane.add(rpane);
      SetLedIntf sli = new SetLedIntf() {
          public void state(boolean on) {
            led.setLedOn(on);
            if(button.isSelected() != on)
              button.setSelected(on);
          }
        };
      devLedM.put(ledId,sli);

    }
    JScrollPane spane = new JScrollPane(pane);
    _tabbedPane.addTab(j.getString("ipaddr"), spane);
    _tabbedPane.setToolTipTextAt(_tabbedPane.getTabCount()-1,
                                 j.getString("devname"));
    DevIntf di = new DevIntf() {
        public void setLed(int ledId, boolean on) {
          devLedM.get(ledId).state(on);
        }
        public void removeDev() {
          _tabbedPane.remove(pane);
          _devicesM.remove(devId);
        }
      };
    _devicesM.put(devId,di);
    try {
      _smq.observe(msg.getPTid(), new IntfOnChange() {
          public void smqOnChange(final long subscribers, final long tid) {
            deviceDisconnected(tid);
          }
        });
    }
    catch(SmqException e) {}
  }


  private void deviceDisconnected(long devId)
  {
    DevIntf di = _devicesM.get(devId);
    if(di != null)
      di.removeDev();
  }

  // Send LED state change to ephemeral tid (device tid) when LED button clicked
  private void setLedState(long devId,int ledId, boolean on)
  {
    byte[] data = new byte[2];
    data[0] = (byte)ledId;
    data[1] = (byte)(on ? 1 : 0);
    try {
      _smq.publish(devId, 0, data);
    }
    catch(SmqException e) {}
  }

  // Device changed LED state: update UI.
  private void ledStateChange(Msg msg)
  {
    byte[] data = msg.getData();
    long devId = msg.getPTid();
    int ledId = (int)(data[0] & 0xFF);
    boolean checked = data[1] == 0 ? false : true;
    _devicesM.get(devId).setLed(ledId, checked);
    //muhahaha
    setAlwaysOnTop(true); toFront(); setAlwaysOnTop(false);
  }

  
  @Override //SMQ callback tries to reconnect
  public void smqOnClose(SmqException e)
  {
    System.out.println("Disconnect : " +e);
    if(e.getReason() == SmqException.SERVER_DISCONNECT)
      System.exit(1); //Not allowed to reconnect
    Iterator<Long> it = _devicesM.keySet().iterator();
    // Remove all UI components.
    while (it.hasNext()) {
      DevIntf di = _devicesM.get(it.next());
      it.remove();
      if(di != null)
        di.removeDev();
    }

    // loop: reconnect. Note this code should be in a separate thread.
    for(;;) {
      if(connect())
        return; // Connected.
      try {Thread.sleep(2000); }
      catch(InterruptedException ignore) {}
    }
  }

  private static void openURL(String url)
  {
    try {
      java.awt.Desktop.getDesktop().browse(
             new URI(url));
    }
    catch(URISyntaxException e1) {}
    catch(IOException e2) {}
  }

  public static void main(String[] args) {
    javax.swing.SwingUtilities.invokeLater(new Runnable() {
        public void run() {
          openURL("https://simplemq.com/m2m-led/");
          Runtime.getRuntime().addShutdownHook(new Thread(new Runnable() {
              public void run() {
                openURL("https://makoserver.net/articles/Browser-to-Device-LED-Control-using-SimpleMQ");
                openURL("https://realtimelogic.com/products/simplemq/");
              }
            }));
          new LedSMQ();
        }
      });
  }

  private JTabbedPane _tabbedPane = new JTabbedPane();
  private Map<Long, DevIntf> _devicesM = new HashMap<Long, DevIntf>();
  private SwingSMQ _smq;
  private byte[] _macAddr;
}
