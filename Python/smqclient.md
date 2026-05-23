# Simple Message Queue Python Client

`smqclient.py` provides a small threaded Python client for the [Simple Message
Queues (SMQ) protocol](https://realtimelogic.com/products/simplemq/).

This document follows the same broad organization as the [Lua SMQ client](https://realtimelogic.com/ba/doc/en/lua/SMQC.html): constructor, object methods, event handlers, and examples.

The client connects to an SMQ broker over the raw SMQ transport. It first sends an
HTTP or HTTPS bootstrap request with the `SimpleMQ: 1` and
`SendSmqHttpResponse: true` headers. After the broker returns HTTP status `200`,
the client and broker exchange binary SMQ packets on the same socket.

```python
from smqclient import SMQClient

smq = SMQClient.create(url, options)
```

`SMQClient.create()` creates an SMQ client instance, starts a background connection
thread, and returns immediately. Lifecycle callbacks and message callbacks run on
the client's background receive thread.

Note: this Python client does not automatically preserve old subscriptions across
reconnect. Put setup code in a shared function and call it from both `onconnect`
and `onreconnect`.

## Constructor

```python
smq = SMQClient.create(url_or_connector, options=None)
```

Create an SMQ client instance and initiate the connection with the broker.

- `url_or_connector`: A fully qualified `http://` or `https://` SMQ broker URL, or
  an advanced connector callable that returns a connected `socket.socket`.
- `options`: Optional dictionary with connection settings and callbacks.

The broker URL is normally the URL of the SMQ LSP endpoint, for example:

```text
http://localhost/smq.lsp
```

If Mako is not listening on port 80, use the printed port:

```text
http://localhost:<port>/smq.lsp
```

### Options

- `uid`: Client unique identifier as `str` or bytes. If omitted, the client creates
  a generated UID. The UID sent to the broker is always at least 6 bytes.
- `info`: Optional information string or bytes passed to server-side broker logic.
- `timeout`: Connection and handshake timeout in seconds. Default: `5.0`.
- `ping`: Idle time in seconds before the client sends an SMQ `Ping`. Default:
  `120.0`.
- `pong`: Maximum time in seconds to wait for a `Pong` or other valid packet after
  sending `Ping`. Default: `10.0`.
- `cleanstart`: Reserved client policy flag. Default: `True`. This implementation
  uses a clean reconnect model.
- `reconnect`: Enables automatic reconnect after unexpected close. Default: `True`.
- `reconnect_delay`: Delay in seconds before a reconnect attempt. Default: `5.0`.
- `max_reconnect_delay`: Optional reconnect delay cap.
- `headers`: Extra HTTP headers to include in the raw SMQ bootstrap request.
- `onauth`: Optional callback `onauth(rnd, ipaddr) -> credentials`.
- `onconnect`: Optional callback `onconnect(etid, rnd, ipaddr)`.
- `onreconnect`: Optional callback `onreconnect(etid, rnd, ipaddr)`.
- `onclose`: Optional callback `onclose(reason, can_reconnect)`.
- `onmsg`: Optional global message callback `onmsg(data, ptid, tid, subtid)`.

Callbacks may also be assigned after creating the object:

```python
smq = SMQClient(url, {"uid": "device-123"})
smq.onconnect = lambda etid, rnd, ipaddr: print("connected", etid)
smq.start()
```

## SMQ Object Methods

Most methods that require the broker return `False` if the client is not currently
connected. Methods that resolve topics or sub-topics report broker results through
callbacks.

### `smq.start()`

```python
smq.start()
```

Start the background connection thread. This is called automatically by
`SMQClient.create()`. Use it only when constructing with `SMQClient(...)` directly.

### `smq.wait_connected(timeout=None)`

```python
connected = smq.wait_connected(5)
```

Wait until the current connection is established.

- `timeout`: Optional timeout in seconds.
- Returns: `True` if connected before the timeout, otherwise `False`.

### `smq.create(topic, onack=None)`

```python
smq.create("chat", onack)
```

Create or resolve a topic name and fetch its topic ID. SMQ publishes use numeric
topic IDs on the wire. The broker assigns the topic ID and the client caches the
mapping.

- `topic`: Topic name as `str`.
- `onack`: Optional callback `onack(accepted, topic, tid)`.
- Returns: `True` if the request was accepted locally for sending, otherwise
  `False`.

Callback arguments:

- `accepted`: `True` if the broker accepted the topic request.
- `topic`: Topic name requested.
- `tid`: Broker-assigned 32-bit topic ID.

`create_topic(topic, onack=None)` is an alias for the instance form of
`create()`.

### `smq.createsub(subtopic, onack=None)`

```python
smq.createsub("rpc.ping", onack)
```

Create or resolve a sub-topic name and fetch its sub-topic ID.

- `subtopic`: Sub-topic name as `str`.
- `onack`: Optional callback `onack(accepted, subtopic, subtid)`.
- Returns: `True` if the request was accepted locally for sending, otherwise
  `False`.

Callback arguments:

- `accepted`: `True` if the broker accepted the sub-topic request.
- `subtopic`: Sub-topic name requested.
- `subtid`: Broker-assigned 32-bit sub-topic ID.

### `smq.publish(data, topic, subtopic=None)`

```python
smq.publish(b"hello", "chat", "message")
smq.publish("hello", topic_tid, subtid)
smq.publish(b"reply", ptid, "rpc.reply")
```

Publish data to a topic, topic ID, or peer ETID. The destination can be:

- Topic name as `str`.
- Topic ID as `int`.
- Peer ETID as `int`, usually received as `ptid` in an `onmsg` callback.
- The string `"self"`, which resolves to this client's current ETID.

Arguments:

- `data`: `str`, `bytes`, `bytearray`, or `memoryview`. Strings are UTF-8 encoded.
- `topic`: Topic name, topic ID, or peer ETID.
- `subtopic`: Optional sub-topic name or numeric sub-topic ID.
- Returns: `True` if the publish was accepted locally for sending, otherwise
  `False`.

If a topic or sub-topic name is not cached yet, `publish()` first asks the broker to
resolve it and then publishes after the acknowledgement arrives.

### `smq.pubjson(value, topic, subtopic=None)`

```python
smq.pubjson({"state": True}, "devices", "state.changed")
```

JSON encode `value` and publish it with compact UTF-8 JSON formatting.

- `value`: Any JSON-serializable Python value.
- `topic`: Topic name, topic ID, or peer ETID.
- `subtopic`: Optional sub-topic name or numeric sub-topic ID.
- Returns: Same as `publish()`.

### `smq.subscribe(topic, subtopic=None, settings=None)`

```python
smq.subscribe("chat", {"onmsg": on_chat})
smq.subscribe("chat", "message", {"onmsg": on_message})
smq.subscribe("self", "rpc.ping", on_rpc_ping)
```

Subscribe to a topic and optionally install a sub-topic-specific callback. You can
subscribe multiple times to the same topic when using different sub-topics.
Subscribing without a sub-topic installs a catch-all callback for the topic.

The topic name `"self"` means this client's current ephemeral topic ID. This is how
one-to-one message handlers are installed.

Arguments:

- `topic`: Topic name as `str`, topic ID as `int`, or `"self"`.
- `subtopic`: Optional sub-topic name as `str`, sub-topic ID as `int`, settings
  dictionary, or callback shorthand.
- `settings`: Optional dictionary or callback shorthand.
- Returns: `True` if the subscription was accepted locally for sending, otherwise
  `False`.

`settings` keys:

- `onack`: Optional callback `onack(accepted, topic, tid, subtopic, subtid)`.
- `onmsg`: Optional callback `onmsg(data, ptid, tid, subtid)`.
- `datatype`: Optional payload conversion. Use `"text"` for UTF-8 text or `"json"`
  for JSON decoding.
- `json`: If true, equivalent to `datatype="json"` when `datatype` is not set.

Callback arguments for `onack`:

- `accepted`: `True` if the broker accepted the subscription.
- `topic`: Topic requested.
- `tid`: Broker-assigned topic ID.
- `subtopic`: Sub-topic requested, or `None`.
- `subtid`: Broker-assigned sub-topic ID, or `None`.

Callback arguments for `onmsg`:

- `data`: Message payload. This is `bytes` by default, `str` for
  `datatype="text"`, or a decoded Python value for `datatype="json"`.
- `ptid`: Publisher ETID. Use this value to reply directly to the sender.
- `tid`: Destination topic ID or ETID.
- `subtid`: Sub-topic ID, or `0` when no sub-topic was used.

The Python client also accepts callback shorthand:

```python
smq.subscribe("chat", on_chat)
smq.subscribe("chat", "message", on_message)
smq.subscribe("self", "rpc.ping", on_rpc_ping)
```

### `smq.unsubscribe(topic)`

```python
smq.unsubscribe("chat")
smq.unsubscribe(topic_tid)
```

Unsubscribe from a topic and remove local message callbacks for that topic.

- `topic`: Topic name as `str` or topic ID as `int`.
- Returns: `True` if an unsubscribe packet was sent, otherwise `False`.

### `smq.observe(topic, onchange)`

```python
smq.observe("chat", on_change)
smq.observe(peer_etid, on_peer_change)
```

Ask the broker to send change notifications for a topic or ETID.

- `topic`: Topic name, topic ID, or peer ETID.
- `onchange`: Callback `onchange(subscribers, topic)`.
- Returns: `True` if the observe request was accepted locally for sending,
  otherwise `False`.

Callback arguments:

- `subscribers`: Current subscriber count. For an observed ETID disconnect, this
  becomes `0`.
- `topic`: Topic name or ETID being observed.

When an ETID observation reports `0`, the client removes the local observation
callback.

### `smq.unobserve(topic)`

```python
smq.unobserve("chat")
smq.unobserve(peer_etid)
```

Stop receiving change notifications.

- `topic`: Topic name, topic ID, or peer ETID.
- Returns: `True` if an unobserve packet was sent, otherwise `False`.

### `smq.disconnect()`

```python
smq.disconnect()
```

Gracefully disconnect. This sends SMQ `Disconnect` when possible, closes the socket,
and prevents automatic reconnect.

`smq.close()` is an alias for `disconnect()`.

### `smq.gettid()`

```python
etid = smq.gettid()
```

Get the client's current ephemeral topic ID. The ETID changes after reconnect.
Callbacks receive publisher ETIDs as `ptid`; publish to a `ptid` to send a direct
reply.

- Returns: Current ETID as `int`, or `None` if not connected.

### `smq.topic2tid(topic)`

```python
tid = smq.topic2tid("chat")
```

Translate a cached topic name to a topic ID.

- `topic`: Topic name.
- Returns: Topic ID as `int`, or `None` if not cached.

### `smq.tid2topic(tid)`

```python
topic = smq.tid2topic(tid)
```

Translate a cached topic ID to a topic name.

- `tid`: Topic ID.
- Returns: Topic name as `str`, or `None` if not cached.

### `smq.subtopic2tid(subtopic)`

```python
subtid = smq.subtopic2tid("rpc.ping")
```

Translate a cached sub-topic name to a sub-topic ID.

- `subtopic`: Sub-topic name.
- Returns: Sub-topic ID as `int`, or `None` if not cached.

### `smq.tid2subtopic(subtid)`

```python
subtopic = smq.tid2subtopic(subtid)
```

Translate a cached sub-topic ID to a sub-topic name.

- `subtid`: Sub-topic ID.
- Returns: Sub-topic name as `str`, or `None` if not cached.

## SMQ Event Handlers

Event handlers may be passed in the constructor options dictionary or assigned as
attributes on the client object.

### `smq.onauth(rnd, ipaddr)`

```python
def onauth(rnd, ipaddr):
    return "username:password"
```

Called during the SMQ handshake before the client sends `Connect`.

- `rnd`: Random seed from the broker.
- `ipaddr`: Client IP address as seen by the broker.
- Returns: Credentials as `str` or bytes. Return an empty string or `None` when no
  authentication is required.

### `smq.onconnect(etid, rnd, ipaddr)`

```python
def onconnect(etid, rnd, ipaddr):
    print("connected", etid)
```

Called after the first successful connection.

- `etid`: Client ephemeral topic ID.
- `rnd`: Random seed from the broker.
- `ipaddr`: Client IP address as seen by the broker.

### `smq.onreconnect(etid, rnd, ipaddr)`

```python
def onreconnect(etid, rnd, ipaddr):
    setup_subscriptions()
```

Called after a later successful reconnect. If `onreconnect` is not set, the client
calls `onconnect` after reconnect.

- `etid`: New client ephemeral topic ID.
- `rnd`: New random seed from the broker.
- `ipaddr`: Client IP address as seen by the broker.

### `smq.onclose(reason, can_reconnect)`

```python
def onclose(reason, can_reconnect):
    print("closed", reason)
    return can_reconnect
```

Called when the connection closes, fails, or is refused.

- `reason`: Human-readable close reason.
- `can_reconnect`: `True` when reconnect is allowed by the client state.
- Returns:
  - `False` to stop reconnecting.
  - `True` to reconnect using `reconnect_delay`.
  - A number to reconnect after that many seconds.
  - `None` to use the option default. With `reconnect=True`, the client reconnects.

### `smq.onmsg(data, ptid, tid, subtid)`

```python
def onmsg(data, ptid, tid, subtid):
    print("message", data, "from", ptid)
```

Global message callback. It is used when no topic-specific or sub-topic-specific
callback matches a received publish.

- `data`: Message payload as `bytes`, unless a subscription requested conversion.
- `ptid`: Publisher ETID.
- `tid`: Destination topic ID or ETID.
- `subtid`: Sub-topic ID, or `0`.

## Examples

The examples below assume the broker is running from this directory:

```powershell
mako -l::test-broker
```

Use the printed port if Mako is not listening on port 80:

```powershell
python example.py http://localhost:<port>/smq.lsp
```

If Mako reports `Server listening on IPv4 port 80`, use:

```powershell
python example.py http://localhost/smq.lsp
```

### Example 1: Connect and Print the ETID

Save as `examples/connect.py`:

```python
import sys
import time
import uuid

from smqclient import SMQClient


url = sys.argv[1] if len(sys.argv) > 1 else "http://localhost/smq.lsp"


def onconnect(etid, rnd, ipaddr):
    print("connected")
    print("etid:", etid)
    print("broker sees ip:", ipaddr)


smq = SMQClient.create(
    url,
    {
        "uid": "py-" + uuid.uuid4().hex[:12],
        "info": "connect example",
        "onconnect": onconnect,
        "reconnect": False,
    },
)

try:
    if not smq.wait_connected(5):
        raise SystemExit("connection timed out")
    time.sleep(1)
finally:
    smq.disconnect()
```

Run:

```powershell
mkdir examples
python examples/connect.py http://localhost/smq.lsp
```

### Example 2: Publish and Subscribe

Save as `examples/pubsub.py`:

```python
import sys
import threading
import uuid

from smqclient import SMQClient


url = sys.argv[1] if len(sys.argv) > 1 else "http://localhost/smq.lsp"
topic = "python.example.chat"
received = threading.Event()


def setup():
    def on_message(data, ptid, tid, subtid):
        print("received:", data, "from", ptid)
        received.set()

    def on_suback(accepted, topic, tid, subtopic, subtid):
        if not accepted:
            raise RuntimeError("subscribe denied")
        smq.publish("hello from Python", topic, subtopic)

    smq.subscribe(topic, "message", {"datatype": "text", "onmsg": on_message, "onack": on_suback})


def onconnect(etid, rnd, ipaddr):
    print("connected as", etid)
    setup()


smq = SMQClient.create(
    url,
    {
        "uid": "py-" + uuid.uuid4().hex[:12],
        "info": "pubsub example",
        "onconnect": onconnect,
        "reconnect": False,
    },
)

try:
    if not received.wait(10):
        raise SystemExit("message timed out")
finally:
    smq.disconnect()
```

Run:

```powershell
python examples/pubsub.py http://localhost/smq.lsp
```

### Example 3: One-to-One Request and Reply

This example creates two clients. Client A subscribes to its own ETID using
`"self"` and the sub-topic `"rpc.ping"`. Client B sends a direct message to client
A's ETID. Client A replies directly to client B by publishing to the received
`ptid`.

Save as `examples/rpc_ping.py`:

```python
import sys
import threading
import uuid

from smqclient import SMQClient


url = sys.argv[1] if len(sys.argv) > 1 else "http://localhost/smq.lsp"
ready_a = threading.Event()
ready_b = threading.Event()
reply_seen = threading.Event()
state = {"a_etid": None, "rpc_pong_subtid": None}


def on_a_connect(etid, rnd, ipaddr):
    state["a_etid"] = etid

    def on_ping(data, ptid, tid, subtid):
        print("A received:", data.decode("utf-8"))
        a.publish("pong", ptid, state["rpc_pong_subtid"] or "rpc.pong")

    a.subscribe("self", "rpc.ping", {"onmsg": on_ping, "onack": lambda accepted, *_: ready_a.set() if accepted else None})


def on_b_connect(etid, rnd, ipaddr):
    def on_pong(data, ptid, tid, subtid):
        print("B received:", data.decode("utf-8"))
        reply_seen.set()

    b.subscribe("self", "rpc.pong", {"onmsg": on_pong, "onack": lambda accepted, *_: ready_b.set() if accepted else None})


a = SMQClient.create(url, {"uid": "py-a-" + uuid.uuid4().hex[:10], "onconnect": on_a_connect, "reconnect": False})
b = SMQClient.create(url, {"uid": "py-b-" + uuid.uuid4().hex[:10], "onconnect": on_b_connect, "reconnect": False})

try:
    if not ready_a.wait(5):
        raise SystemExit("client A not ready")
    if not ready_b.wait(5):
        raise SystemExit("client B not ready")

    state["rpc_pong_subtid"] = b.subtopic2tid("rpc.pong")
    b.publish("ping", state["a_etid"], "rpc.ping")

    if not reply_seen.wait(10):
        raise SystemExit("reply timed out")
finally:
    a.disconnect()
    b.disconnect()
```

Run:

```powershell
python examples/rpc_ping.py http://localhost/smq.lsp
```

### Example 4: Reconnect and Resubscribe

Reconnect creates a new ETID and clears local topic/sub-topic callback state. Put
all setup in one function and call it from both `onconnect` and `onreconnect`.

```python
from smqclient import SMQClient


def setup():
    smq.subscribe("python.example.status", "update", {"onmsg": on_status})
    smq.subscribe("self", "rpc.status", {"onmsg": on_status_rpc})


def on_status(data, ptid, tid, subtid):
    print("status update:", data)


def on_status_rpc(data, ptid, tid, subtid):
    smq.publish(b"ok", ptid, "rpc.status.reply")


def onconnect(etid, rnd, ipaddr):
    print("connected", etid)
    setup()


def onreconnect(etid, rnd, ipaddr):
    print("reconnected", etid)
    setup()


def onclose(reason, can_reconnect):
    print("closed:", reason)
    return can_reconnect


smq = SMQClient.create(
    "http://localhost/smq.lsp",
    {
        "uid": "python-reconnect-example",
        "onconnect": onconnect,
        "onreconnect": onreconnect,
        "onclose": onclose,
        "reconnect": True,
        "reconnect_delay": 2,
    },
)
```

Run this example as a script, stop and restart the broker, and watch the client call
`onreconnect` after the broker becomes available again.

### Example 5: Tkinter M2M LED Control UI

The larger example [examples/m2m_led_tk.py](examples/m2m_led_tk.py)
replicates the browser UI client from the [SimpleMQ M2M LED demo](https://simplemq.com/m2m-led/) using Tkinter. See the tutorial [Browser to Device LED Control using SMQ](https://makoserver.net/articles/Browser-to-Device-LED-Control-using-SimpleMQ) for details.

It acts as a display/control client:

- Subscribes to `/m2m/led/device`, sub-topic `devinfo`, as JSON.
- Subscribes to `"self"`, sub-topic `devinfo`, as JSON.
- Subscribes to `/m2m/led/device`, sub-topic `led`, for two-byte LED updates.
- Subscribes to `/m2m/temp` for two-byte signed temperature updates.
- Publishes `"Hello"` to `/m2m/led/display` to discover devices.
- Sends LED commands directly to the device ETID with a two-byte payload:
  LED ID and on/off state.
- Observes each device ETID and removes the device from the UI when it disconnects.

Run against the public SimpleMQ broker:

```powershell
python examples/m2m_led_tk.py
```

Run against a local broker:

```powershell
mako -l::test-broker
python examples/m2m_led_tk.py http://localhost/smq.lsp
```

The public demo requires a compatible LED device or simulator connected to the same
broker. The UI will show `No devices connected` until a device publishes its
capability JSON.

## Smoke Test

The repository includes a focused broker smoke test:

```powershell
mako -l::test-broker
python examples/smq_smoke.py http://localhost/smq.lsp
```

The smoke test demonstrates:

- Connect and receive an ETID.
- Create or resolve a topic.
- Publish and subscribe on a named topic.
- Subscribe to `"self"` with sub-topic `"rpc.ping"`.
- Send a one-to-one message and reply directly to the sender's PTID.
- Reconnect and perform setup/resubscribe after reconnect.

Successful output looks like:

```text
ok: connect etid ...
ok: create/resolve topic ...
ok: publish/subscribe
ok: self rpc.ping callback
ok: one-to-one PTID reply
ok: reconnect and resubscribe
```

## Notes

- SMQ payloads are bytes on the wire. Use `datatype="text"` or
  `datatype="json"` only when the receiver expects that encoding.
- The maximum SMQ payload size documented by BAS is `0xFFF0` bytes.
- Topic names and sub-topic names are resolved to broker-assigned 32-bit IDs.
- `"self"` is local shorthand for the current client's ETID. It changes after
  reconnect.
- `ptid` is the publisher ETID. Publish to `ptid` to send a direct reply.
- Do not assume MQTT-style retained messages, QoS, or broker-managed persistent
  sessions.
