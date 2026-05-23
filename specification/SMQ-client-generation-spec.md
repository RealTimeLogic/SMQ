# SMQ Client Generation Specification

**Purpose:** Guidance for AI-generated SMQ client libraries in any language, including
Python, C++, JavaScript, Lua, Go, Rust, and C#.

Generated clients must expose a high-level API that follows the behavior and naming
model of the BAS SMQ Lua client while remaining idiomatic for the target language.
The API must hide packet-level details from application code, but the implementation
must preserve SMQ semantics: broker-assigned topic IDs, ephemeral topic IDs,
publisher TIDs, sub-topics, one-to-one messaging, observe/change events, ping/pong,
and reconnect handling.

## 1. Reference Documents

This document defines the required API shape and behavior for AI-generated SMQ
clients.

For protocol-level details, generated clients must use `SMQ-specification.md` in
this directory as the authoritative local protocol reference.

For upstream BAS API documentation, use:

https://realtimelogic.com/downloads/basapi.md

When the local specification and upstream documentation differ, prefer
`SMQ-specification.md` for wire-protocol behavior. Use the upstream BAS documentation
to clarify BAS API behavior and to check for newer BAS-specific details.

## 2. Required Design Goals

An AI-generated SMQ client must:

- Connect to an SMQ broker over the requested transport.
- Perform the SMQ `Init`, `Connect`, and `Connack` handshake.
- Expose the connected client's Ephemeral Topic ID (ETID).
- Resolve topic names and sub-topic names to broker-assigned IDs.
- Publish to named topics, numeric topic IDs, and peer ETIDs.
- Subscribe to named topics and to the client's own ETID using `self`.
- Support sub-topic-specific callbacks, especially for one-to-one messages.
- Include automatic reconnect as part of the connection API.
- Provide callback hooks for authentication, connect, reconnect, close, and messages.
- Avoid broker-managed session assumptions. On reconnect, the application should do a
  full setup and resubscribe to all required topics.

## 3. Terminology

| Term | Meaning |
| --- | --- |
| Client | The generated SMQ client object. |
| Broker | The SMQ broker/hub. |
| Topic | A named publish/subscribe channel, such as `chat` or `/device/temp`. |
| TID | Broker-assigned 32-bit Topic ID. |
| ETID | Ephemeral Topic ID assigned to a connected client. |
| PTID | Publisher TID received with a message. PTID is the publisher's ETID. |
| Sub-topic | Secondary routing value used as a message type, method, or operation name. |
| STID or subtid | Broker-assigned 32-bit sub-topic ID. |
| UID | Client unique identifier sent during connection. |
| Credentials | Authentication bytes/string returned by the client's authentication callback. |
| Info | Optional client information string sent during connection. |

## 4. Public API Shape

Generated clients should expose this language-neutral API. Names may be adapted to
target-language conventions, but the semantics must remain the same.

```text
client = SMQClient.create(url_or_connector, options)

client.create(topic, onack = null)
client.createsub(subtopic, onack = null)
client.publish(data, topic_or_tid_or_etid, subtopic_or_subtid = null)
client.subscribe(topic_or_self, subtopic_or_subtid = null, settings = {})
client.unsubscribe(topic_or_tid)
client.observe(topic_or_tid_or_etid, onchange)
client.unobserve(topic_or_tid_or_etid)
client.disconnect()
client.close()

client.gettid()
client.topic2tid(topic)
client.tid2topic(tid)
client.subtopic2tid(subtopic)
client.tid2subtopic(subtid)
```

The client object must also support these callback properties or constructor
callbacks:

```text
client.onauth(rnd, ipaddr) -> credentials
client.onconnect(etid, rnd, ipaddr)
client.onreconnect(etid, rnd, ipaddr)
client.onclose(reason, can_reconnect) -> reconnect_decision
client.onmsg(data, ptid, tid, subtid)
```

Target languages may prefer event emitters, delegates, virtual methods, futures,
promises, or function pointers. The generated API must still provide equivalent
hooks.

## 5. Constructor and Options

### 5.1 Create

```text
SMQClient.create(url_or_connector, options) -> client
```

`create` must allocate the client, initialize queues and caches, start the connection
attempt, and enable automatic reconnect.

`url_or_connector` may be:

- A URL string for HTTP(S), WebSocket, or secure WebSocket connection.
- A connector function/object supplied by advanced users.

Required options:

| Option | Type | Default | Meaning |
| --- | --- | --- | --- |
| `uid` | string/bytes | implementation-defined | Client unique ID. Must be at least 6 bytes after fallback generation. |
| `info` | string/bytes | empty or local address | Optional client info string sent in `Connect`. |
| `timeout` | duration | 5000 ms | Connection and handshake timeout. |
| `ping` | duration | 120 s | Idle time before sending SMQ `Ping`. |
| `pong` | duration | 10 s | Time to wait for `Pong` before closing. |
| `cleanstart` | bool | true recommended | If true, do not attempt local replay of old subscriptions or queued publishes after reconnect. |
| `reconnect` | bool | true | Enables automatic reconnect. |
| `reconnect_delay` | duration | 5000 ms | Default delay before reconnect attempt. |
| `max_reconnect_delay` | duration | implementation-defined | Optional cap for backoff. |
| `headers` | map | empty | Optional HTTP headers for raw socket bootstrap. |

Generated clients must include reconnect support in `create`; applications should not
have to build a reconnect loop around the library.

### 5.2 Reconnect Model

The generated client must automatically reconnect when the connection fails and
`can_reconnect` is true, unless the application explicitly disabled reconnect or
`onclose` rejects reconnect.

Recommended `onclose` return behavior:

| Return value | Meaning |
| --- | --- |
| `false`, `null`, or no reconnect decision | Do not reconnect, or use the option default if the language cannot distinguish no return. |
| `true` | Reconnect using the configured delay. |
| number/duration | Reconnect after the returned delay. |

The client must call:

- `onconnect(etid, rnd, ipaddr)` after the first successful connection.
- `onreconnect(etid, rnd, ipaddr)` after a later successful reconnection, if this
  callback exists.
- `onconnect(etid, rnd, ipaddr)` after reconnection if `onreconnect` is not provided.

Do not rely on automatic replay as the primary reconnect strategy. Local replay of
subscriptions and pending messages is difficult to implement correctly. The
recommended application pattern is to put all setup code in a shared function and call
it from both `onconnect` and `onreconnect`.

Example pattern:

```text
function setup(client):
    client.subscribe("telemetry", { onmsg: onTelemetry })
    client.subscribe("self", "rpc.getStatus", { onmsg: onGetStatus })
    client.observe("alarms", onAlarmSubscriberChange)

client.onconnect = function(etid, rnd, ipaddr):
    setup(client)

client.onreconnect = function(etid, rnd, ipaddr):
    setup(client)
```

## 6. Connection Handshake

The client must implement this sequence:

```text
Client                                      Broker
  | ---- transport bootstrap --------------> |
  | <---------------- Init ------------------ |
  | ---------------- Connect ---------------> |
  | <--------------- Connack ---------------- |
  |                  ready                    |
```

### 6.1 Init

The broker sends:

| Field | Size | Meaning |
| --- | ---: | --- |
| Version | 1 byte | Protocol version `1`. |
| Seed | 4 bytes | Random value for authentication. |
| IP address | N bytes | Client IP address as seen by the broker. |

The generated client must pass `Seed` and `IP address` to `onauth`.

### 6.2 Connect

The client sends:

| Field | Size | Meaning |
| --- | ---: | --- |
| Version | 1 byte | Protocol version `1`. |
| UID length | 1 byte | Length of UID. |
| UID | N bytes | Client unique ID. |
| Credentials length | 1 byte | Length of credentials. |
| Credentials | N bytes | Authentication data from `onauth`, or empty. |
| Info | N bytes | Optional info string through end of packet. |

If the UID is not supplied, the client must generate a stable or sufficiently unique
fallback UID. If a generated UID would be shorter than 6 bytes, the client must extend
it to at least 6 bytes.

### 6.3 Connack

The broker sends:

| Field | Size | Meaning |
| --- | ---: | --- |
| Return code | 1 byte | `0x00` accepted, non-zero refused. |
| ETID | 4 bytes | Client ETID if accepted. |
| Message | N bytes | Optional human-readable refusal reason. |

On return code `0x00`, the generated client must:

- Store the ETID.
- Add a topic cache mapping from `"self"` to the ETID.
- Mark the client connected.
- Start receive processing.
- Start ping/pong keepalive handling.
- Call `onconnect` or `onreconnect`.

On non-zero return code, the client must close the transport and invoke `onclose`.

## 7. Topic and Sub-topic Resolution

### 7.1 Caches

The client must maintain these internal maps:

```text
topic_name_to_tid
tid_to_topic_name
subtopic_name_to_subtid
subtid_to_subtopic_name
pending_topic_acks
pending_subtopic_acks
```

When a topic or sub-topic is accepted by the broker, the client must cache both
directions.

### 7.2 create(topic, onack)

`create` resolves a topic name to a TID without subscribing.

If already cached, call `onack(true, topic, tid)` immediately or asynchronously
according to target-language convention.

If not cached, send `Create(topic)`. When `Createack` arrives:

```text
onack(accepted, topic, tid)
```

### 7.3 createsub(subtopic, onack)

`createsub` resolves a sub-topic name to a sub-topic ID.

If already cached, call `onack(true, subtopic, subtid)`.

If not cached, send `Createsub(subtopic)`. When `Createsuback` arrives:

```text
onack(accepted, subtopic, subtid)
```

## 8. Publish API

### 8.1 publish(data, topic, subtopic)

```text
client.publish(data, topic_or_tid_or_etid, subtopic_or_subtid = null) -> bool
```

`topic_or_tid_or_etid` may be:

- A topic name string.
- A resolved topic TID.
- A peer ETID, usually received as `ptid` in an incoming message.

`subtopic_or_subtid` may be:

- `null` or omitted, meaning sub-topic ID `0`.
- A sub-topic name string.
- A resolved sub-topic ID.

If a topic or sub-topic name is not yet cached, the client should resolve it by
sending `Create` and/or `Createsub`, then publish after the ID is accepted.

The publish packet body is:

| Field | Size | Meaning |
| --- | ---: | --- |
| Destination TID | 4 bytes | Topic TID or peer ETID. |
| Publisher ETID | 4 bytes | The client's own ETID. |
| Sub-topic ID | 4 bytes | Sub-topic ID or `0`. |
| Payload | N bytes | Application data. |

The client must not publish before it has an ETID.

### 8.2 Payload Conversion

The generated client should support raw bytes and text. It may additionally support
JSON convenience methods.

Recommended methods:

```text
client.publish(data, topic, subtopic = null)
client.pubjson(value, topic, subtopic = null)
```

Language-specific guidance:

- Python: accept `bytes`, `bytearray`, `str`, and JSON-serializable values for
  `pubjson`.
- C++: accept byte spans/vectors and UTF-8 strings; keep JSON optional.
- JavaScript/TypeScript: accept `Uint8Array`, `ArrayBuffer`, and `string`.

## 9. Subscribe API

### 9.1 subscribe(topic, subtopic, settings)

```text
client.subscribe(topic_or_self, subtopic_or_subtid = null, settings = {})
```

`topic_or_self` may be:

- A topic name string.
- A resolved topic TID.
- The literal string `"self"`, meaning the client's own ETID.

`settings` should support:

| Setting | Meaning |
| --- | --- |
| `onack(accepted, topic, tid, subtopic, subtid)` | Called when the broker accepts or denies the subscription. |
| `onmsg(data, ptid, tid, subtid)` | Callback for messages matching this subscription. |
| `datatype` or `json` | Optional payload decoding helper. |

If `subtopic` is provided as a string, the client must first resolve it with
`createsub`, then subscribe or register the callback.

### 9.2 Sub-topic Callback Dispatch

The client must support multiple callbacks for the same topic when different
sub-topics are used.

Recommended internal callback table:

```text
message_callbacks[tid] = {
    catch_all: callback_or_null,
    subtopics: {
        subtid: callback
    }
}
```

Dispatch rules for incoming `Publish`:

1. Look up `message_callbacks[tid]`.
2. If a callback exists for `subtid`, call that callback.
3. Otherwise, if a catch-all callback exists for `tid`, call it.
4. Otherwise, call global `client.onmsg`, if set.
5. Otherwise, drop the message locally.

### 9.3 Self Subscription for One-to-One Messages

A generated client must support subscribing to its own ETID using the topic name
`"self"`.

This is essential for one-to-one message handling. A client can register dedicated
callbacks for direct messages by subscribing to `"self"` with sub-topics.

Example:

```text
client.subscribe("self", "rpc.getStatus", {
    onmsg: function(data, ptid, tid, subtid):
        response = buildStatus()
        client.publish(response, ptid, "rpc.getStatus.reply")
})

client.subscribe("self", "alarm.ack", {
    onmsg: function(data, ptid, tid, subtid):
        acknowledgeAlarm(data)
})
```

Semantics:

- `"self"` resolves to the client's current ETID.
- The sub-topic selects the operation or message type.
- The callback receives `ptid`, which is the sender's ETID.
- The callback can reply directly by publishing to `ptid`.
- After reconnect, `"self"` changes because the ETID may change; the application must
  resubscribe to `"self"` and all required sub-topics.

The implementation should not send a normal `Subscribe` packet for `"self"` if the
broker already routes direct ETID messages implicitly. It should register the local
callback against the current ETID. If the target broker requires an explicit
subscription for self-routing, the implementation may send the appropriate subscribe
request, but the public API must remain the same.

## 10. One-to-One Messaging

SMQ one-to-one messaging uses ETIDs instead of dynamically generated reply topics.

### 10.1 Receiving a Peer Address

When the client receives a message:

```text
onmsg(data, ptid, tid, subtid)
```

`ptid` is the publisher's ETID. This is the address the receiver can use for a direct
reply.

### 10.2 Sending a Direct Reply

To reply to the sender:

```text
client.publish(reply_data, ptid, reply_subtopic)
```

The destination is the peer ETID, not a topic name.

### 10.3 Calling a Peer Method

If a client knows a peer ETID, it can call a method-like handler by publishing to the
peer ETID with a sub-topic:

```text
client.publish(request_data, peer_etid, "rpc.setLed")
```

The peer should have registered:

```text
client.subscribe("self", "rpc.setLed", { onmsg: onSetLed })
```

### 10.4 Correlation

SMQ provides the return address (`ptid`) and the sub-topic (`subtid`), but it does not
define a message correlation field. Applications that need correlation IDs should
encode them in the payload or use a sub-topic convention.

## 11. Unsubscribe API

```text
client.unsubscribe(topic_or_tid)
```

The client must remove local callbacks for the topic and send `Unsubscribe(tid)` if
connected and the TID is known. `Unsubscribe` has no acknowledgement packet.

## 12. Observe API

### 12.1 observe(topic, onchange)

```text
client.observe(topic_or_tid_or_etid, onchange)
```

The client sends `Observe(tid_or_etid)` and registers:

```text
onchange(subscribers, topic_or_tid_or_etid)
```

Named topic observation:

- The callback is invoked when a client subscribes, unsubscribes, or disconnects from
  the observed topic.
- Notifications are broker-local for named topics in an SMQ cluster.

ETID observation:

- The callback is invoked when the observed ETID disconnects.
- ETID observation is suitable for supervising a peer before sending one-to-one
  messages.
- ETID disconnect notifications work in an SMQ cluster environment.
- The client should automatically remove an ETID observation after receiving the
  disconnect notification.

### 12.2 unobserve(topic)

```text
client.unobserve(topic_or_tid_or_etid)
```

The client sends `Unobserve(tid_or_etid)` and removes the local observe callback.

## 13. Receive Loop

The generated client must run a receive loop while connected. For each packet:

| Packet | Required client action |
| --- | --- |
| `Suback` | Resolve pending subscribe callbacks and cache topic mapping. |
| `Createack` | Resolve pending create callbacks and cache topic mapping. |
| `Createsuback` | Resolve pending createsub callbacks and cache sub-topic mapping. |
| `Publish` | Dispatch to sub-topic callback, catch-all callback, or global `onmsg`. |
| `Ping` | Send `Pong`. |
| `Pong` | Mark keepalive response received. |
| `Change` | Call registered observe callback. |
| `Disconnect` | Close the transport and enter reconnect/closed handling. |
| Unknown | Treat as protocol error and close the connection. |

The receive loop must tolerate packet coalescing and fragmentation according to the
transport. Raw sockets use the two-byte SMQ packet length. WebSockets provide frame
boundaries.

## 14. Send Queue

The client should serialize writes and maintain an internal send queue. This is
especially important for async languages and event-loop implementations.

Required behavior:

- If connected and the queue is empty, attempt immediate send.
- If send fails, mark the connection closed and enter reconnect handling.
- If not connected, reject the operation or queue it according to `cleanstart` and
  implementation policy.
- Do not silently retain unbounded queued messages.

Recommended policy:

- `cleanstart=true`: do not preserve queued publishes across reconnect.
- `cleanstart=false`: local replay may be offered, but must be bounded and clearly
  documented. It is not recommended as the default.

## 15. Keepalive

The client must implement SMQ `Ping`/`Pong`.

Recommended behavior:

1. Track whether any packet has been received since the last keepalive interval.
2. If no packet has been received for `ping` seconds, send `Ping`.
3. Wait up to `pong` seconds.
4. If `Pong` or any valid packet is received, continue.
5. If no response is received, close the connection and enter reconnect handling.

When receiving `Ping`, immediately send `Pong`.

## 16. Error Handling

The client must close the connection and call `onclose` for:

- Transport failure.
- Handshake timeout.
- Unsupported protocol version.
- Malformed packet.
- Unexpected packet type for the current state.
- `Connack` refusal.
- Missing `Pong` after `Ping`.
- Broker `Disconnect`.

`onclose(reason, can_reconnect)` must receive enough information for application
code to decide whether reconnect should continue.

## 17. Threading and Async Requirements

Generated clients may be blocking, threaded, event-loop based, or async/await based.
Regardless of model:

- Callback invocation order must be deterministic per connection.
- Send operations must not interleave packet bytes.
- Reconnect must not run concurrently with an active connection attempt.
- `disconnect()` must prevent further automatic reconnect.
- `close()` should be an alias for `disconnect()` unless the target language uses a
  different convention.

## 18. Minimal Examples

### 18.1 Connect and Subscribe

```text
client = SMQClient.create(url, {
    uid: "device-123",
    info: "python-client",
    reconnect: true,
    cleanstart: true
})

function setup():
    client.subscribe("sensors/temp", {
        onmsg: function(data, ptid):
            print("temperature:", data)
    })

client.onconnect = function(etid, rnd, ipaddr):
    setup()

client.onreconnect = function(etid, rnd, ipaddr):
    setup()
```

### 18.2 Request/Response Using PTID

```text
client.subscribe("commands", "getStatus", {
    onmsg: function(data, ptid, tid, subtid):
        status = makeStatusPayload()
        client.publish(status, ptid, "getStatus.reply")
})
```

### 18.3 Dedicated One-to-One Handlers

```text
client.subscribe("self", "led.set", {
    onmsg: function(data, ptid):
        setLed(data)
        client.publish("ok", ptid, "led.set.reply")
})

client.subscribe("self", "led.get", {
    onmsg: function(data, ptid):
        client.publish(getLedState(), ptid, "led.get.reply")
})
```

### 18.4 Calling a Peer ETID

```text
peer = last_message_ptid
client.publish("{\"state\":true}", peer, "led.set")
```

## 19. Language-Specific Notes

### Python

- Prefer `asyncio` for network clients, but blocking clients are acceptable if clearly
  documented.
- Use `bytes` internally for packets.
- Decode text and JSON only when the subscription requested it.
- Use `Callable` types for callbacks.

### C++

- Use RAII for connection cleanup.
- Use `std::span`, `std::vector<std::uint8_t>`, or `std::string_view` for payloads.
- Make callback ownership explicit.
- Protect shared state if callbacks can run on background threads.

### JavaScript/TypeScript

- Use `Uint8Array` internally for payloads.
- Use `Promise` or event emitters for connection lifecycle if idiomatic.
- Keep `onconnect`, `onreconnect`, `onclose`, and per-subscription `onmsg` semantics.

## 20. Compliance Checklist

An AI-generated client is compliant with this specification if it:

- Implements `create` with automatic reconnect support.
- Exposes `onconnect`, `onreconnect`, `onclose`, `onauth`, and `onmsg` equivalents.
- Exposes `create`, `createsub`, `publish`, `subscribe`, `unsubscribe`, `observe`,
  `unobserve`, `disconnect`, and ID translation helpers.
- Stores ETID after `Connack`.
- Treats `"self"` as the current ETID.
- Supports subscribing to `"self"` with a sub-topic and dedicated callback.
- Dispatches incoming messages by `tid` and `subtid`.
- Provides `ptid` to callbacks and supports publishing replies to `ptid`.
- Recommends application-level setup and resubscribe on reconnect.
- Implements ping/pong keepalive.
- Handles broker accept/deny acknowledgements.
- Does not assume MQTT-style QoS, retained messages, or broker-managed persistent
  sessions.
