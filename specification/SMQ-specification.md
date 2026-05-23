# Simple Message Queues (SMQ) Protocol Specification

## 1. Introduction

Simple Message Queues (SMQ) is a lightweight publish/subscribe protocol used by the
Barracuda App Server (BAS) for real-time web, device, and IoT communication. SMQ is
similar in purpose to MQTT: clients connect to a broker, subscribe to topics, and
publish payloads that the broker routes to matching subscribers.

SMQ differs from MQTT in several important ways:

- SMQ topic names are resolved by the broker into 32-bit numeric Topic IDs (TIDs).
  Data packets use TIDs, not topic strings.
- Every connected client receives an Ephemeral Topic ID (ETID). The ETID acts as the
  client's temporary return address.
- Published messages include the publisher's ETID, called the publisher topic ID
  (PTID) in callbacks, which lets a receiver reply directly to the sender.
- A client can publish to a normal topic TID for one-to-many delivery or directly to
  another client's ETID for one-to-one delivery.
- SMQ supports sub-topics as a second 32-bit routing value. Sub-topics are commonly
  used as method names, message types, or RPC operation selectors.
- SMQ supports `Observe`/`Change` notifications so a client can monitor subscriber
  counts or supervise a peer ETID.
- Browser clients use WebSockets, while embedded clients can use a simpler raw
  socket transport after an HTTP(S) bootstrap.

SMQ is designed for message exchange, not bulk transfer. BAS documents the maximum
SMQ payload size as `0xFFF0` bytes.

## 2. Terminology

| Term | Meaning |
| --- | --- |
| Broker | The BAS SMQ hub instance that accepts clients and routes messages. |
| Client | Any browser, device, or server-side participant connected to the broker. |
| Topic name | A string such as `/device1/temperature` or `chat`. |
| TID | A 32-bit Topic ID assigned by the broker to a topic name. |
| ETID | A 32-bit Ephemeral Topic ID assigned to a connected client. Valid only while that client is connected. |
| PTID | Publisher Topic ID. In received messages, this is the ETID of the publishing client. |
| Sub-topic | A secondary topic name that the broker resolves to a 32-bit sub-topic ID. |
| STID or subtid | The 32-bit sub-topic ID carried in `Publish` messages. |
| UID | Client supplied unique identifier. BAS exposes this in the broker peer table as `uid`. |
| Credentials | Client supplied authentication material passed to the broker `authenticate` callback. |
| Info | Optional client supplied identification string passed to BAS server-side callbacks. |
| Observer | A client that asked the broker for subscriber-count or ETID-disconnect notifications. |

All multi-byte integer fields are unsigned and encoded in network byte order
(big-endian). Strings and payloads are byte strings and are not NUL-terminated.

## 3. Architecture

### 3.1 Broker Instances

A BAS application can create one or more SMQ broker instances. Each broker is normally
bound to a URL entry point such as an LSP page or directory function. Separate broker
instances isolate topic tables, peer tables, subscriptions, and observations.

A BAS broker may also act as a server-side SMQ client. By default, the server-side
client uses ETID `1`, which lets external clients address server-side logic directly.
The broker option `rndtid` can be used by BAS server code to make the server-side
client use a random ETID instead.

### 3.2 Topic Resolution

Clients use topic names at the API level, but the wire protocol uses 32-bit TIDs.
Before publishing to a named topic, a client must know the topic's TID. A client gets
this TID by sending `Create` or by subscribing with `Subscribe`.

The broker keeps the topic-name-to-TID mapping for the lifetime of the broker
instance. BAS documentation describes broker-created TIDs as random 32-bit values
unless server-side code programmatically selects a specific TID.

### 3.3 Ephemeral Topic IDs

On successful connection, the broker assigns the client an ETID and returns it in
`Connack`. The ETID is valid only for the current connection. When the client
disconnects and reconnects, the broker may assign a new ETID.

All clients are implicitly reachable through their own ETID. A client does not need to
subscribe to its ETID in order for the broker to route direct messages to it. Higher
level APIs also support the topic name `self`, which means the current client's ETID.

### 3.4 Publisher Address

Every `Publish` packet carries the sender's ETID. The broker verifies that the sender
ETID matches the connected peer. Subscribers receive this value as `ptid`.

This makes request/response patterns simple:

1. Client A subscribes to `sensor/request`.
2. Client B publishes a request to `sensor/request`.
3. Client A receives the request and learns Client B's ETID from `ptid`.
4. Client A publishes the response directly to Client B's ETID.

No dynamically generated reply topic is required.

### 3.5 Sub-topics

Sub-topics are 32-bit values carried with every publish packet. They are transparent
to the broker's primary topic routing logic, but client APIs use them to dispatch
messages to more specific handlers. Sub-topics are primarily designed for one-to-one
messaging, where a peer ETID identifies the receiving client and the sub-topic
selects the function, method, or message type within that client.

Typical uses:

- Message type: `hello`, `status`, `alarm`, `ack`
- RPC operation: `get`, `set`, `delete`
- Object method selector when publishing to a peer ETID

Sub-topic ID `0` means no sub-topic.

### 3.6 Delivery Model

SMQ provides broker-routed message delivery to connected subscribers. The BAS SMQ
documentation does not define MQTT-style QoS levels, retained messages, durable
server-side sessions, or wildcard subscriptions. Client libraries may queue API
operations locally while disconnected, but this is client-side behavior and should not
be confused with MQTT persistent sessions.

If a message is published to a topic with no subscribers, the broker drops it unless
server-side BAS code installed an `ondrop` callback to process dropped messages.

## 4. Transport

### 4.1 Raw Socket Transport

Device clients can connect by issuing an HTTP or HTTPS request to the broker URL.
The broker morphs the request into a persistent TCP or TLS socket and then starts the
SMQ binary protocol.

In the raw socket transport, each SMQ packet has a two-byte packet length followed by
a one-byte message type.

| Field | Size | Description |
| --- | ---: | --- |
| Packet length | 2&nbsp;bytes | Total SMQ packet length, including these two length bytes. |
| Message type | 1&nbsp;byte | SMQ control packet type. |
| Variable data | N&nbsp;bytes | Packet-specific fields. |

Raw socket SMQ packets use this total-length format.

### 4.2 WebSocket Transport

Browser clients use WebSockets or secure WebSockets. The WebSocket frame length
already provides packet framing, so the SMQ payload starts with the one-byte message
type.

| Field | Size | Description |
| --- | ---: | --- |
| Message type | 1&nbsp;byte | SMQ control packet type. |
| Variable data | N&nbsp;bytes | Packet-specific fields. |

SMQ WebSocket messages are binary frames. JavaScript client APIs may encode strings
as UTF-8 before placing them in the binary payload.

### 4.3 HTTP Bootstrap

BAS raw-socket clients and browser clients both start through a normal HTTP(S) URL:

- Browser clients use the WebSocket upgrade path (`ws://` or `wss://`).
- Raw clients use an HTTP(S) request that BAS converts into a persistent socket.

Raw socket clients identify the request as SMQ by using the HTTP headers
`SimpleMQ: 1` and `SendSmqHttpResponse: true` when connecting. BAS then returns
HTTP status `200` and morphs the HTTP client object into a socket before the SMQ
`Init` packet is read.

## 5. Control Packet Types

| Name | Value | Direction | Summary |
| --- | ---: | --- | --- |
| Invalid | 0 | N/A | Invalid or reserved value. |
| Init | 1 | Server to client | Starts the SMQ handshake. |
| Connect | 2 | Client to server | Sends UID, credentials, and optional info. |
| Connack | 3 | Server to client | Accepts or rejects the connection. |
| Subscribe | 4 | Client to server | Subscribe to a topic name. |
| Suback | 5 | Server to client | Returns accepted/denied and the topic TID. |
| Create | 6 | Client to server | Resolve or create a topic TID without subscribing. |
| Createack | 7 | Server to client | Returns accepted/denied and the topic TID. |
| Publish | 8 | Both | Publish a payload to a TID or ETID. |
| Unsubscribe | 9 | Client to server | Remove a topic subscription. |
| Reserved | 10 | N/A | Reserved. |
| Disconnect | 11 | Both | Gracefully close the SMQ connection. |
| Ping | 12 | Both | Keepalive probe. |
| Pong | 13 | Both | Keepalive response. |
| Observe | 14 | Client to server | Request subscriber-count or ETID supervision events. |
| Unobserve | 15 | Client to server | Cancel a previous observation. |
| Change | 16 | Server to client | Subscriber count or ETID state changed. |
| Createsub | 17 | Client to server | Resolve or create a sub-topic ID. |
| Createsuback | 18 | Server to client | Returns accepted/denied and the sub-topic ID. |
| PubFrag | 19 | Client to server | Fragmented publish for constrained clients. |
| Reserved | 20-255 | N/A | Reserved. |

## 6. Connection State Machine

### 6.1 Normal Handshake

```text
Client                                      Broker
  | ---- HTTP(S) or WebSocket bootstrap ----> |
  | <---------------- Init ------------------ |
  | ---------------- Connect ---------------> |
  | <--------------- Connack ---------------- |
  |                  Publish/Subscribe        |
```

After `Connack` return code `0x00`, the client is connected. The broker has assigned
an ETID and may route messages to and from that client.

### 6.2 Init

`Init` is sent by the broker after the transport has been established.

| Field | Size | Description |
| --- | ---: | --- |
| Version | 1&nbsp;byte | Protocol version is `1`. |
| Seed | 4&nbsp;bytes | Random number generated by the broker. |
| IP address | N&nbsp;bytes | Client IP address as seen by the broker, encoded as text. |

The seed and IP address are provided to client authentication hooks. A client may use
them when generating a challenge/response credential.

### 6.3 Connect

`Connect` is sent by the client after receiving `Init`.

| Field | Size | Description |
| --- | ---: | --- |
| Version | 1&nbsp;byte | Client protocol version is `1`. |
| UID length | 1&nbsp;byte | Length of the UID field. |
| UID | N&nbsp;bytes | Client unique identifier. Minimum recommended length is 6 bytes. |
| Credentials length | 1&nbsp;byte | Length of the credentials field. |
| Credentials | N&nbsp;bytes | Authentication material. Empty if not used. |
| Info | N&nbsp;bytes | Optional client information string. Extends to the end of packet. |

### 6.4 Connack

`Connack` is sent by the broker in response to `Connect`.

| Field | Size | Description |
| --- | ---: | --- |
| Return code | 1&nbsp;byte | `0x00` means accepted. Non-zero means refused. |
| ETID | 4&nbsp;bytes | Client's ephemeral Topic ID. Meaningful when return code is `0x00`. |
| Message | N&nbsp;bytes | Optional human-readable refusal message when return code is non-zero. |

Return codes:

| Code | Meaning |
| ---: | --- |
| `0x00` | Connection accepted. |
| `0x01` | Connection refused: unacceptable protocol version. |
| `0x02` | Connection refused: server unavailable or broker timed out during connect. |
| `0x03` | Connection refused: incorrect credentials. |
| `0x04` | Connection refused: client certificate required. |
| `0x05` | Connection refused: client certificate not accepted. |
| `0x06` | Connection refused: access denied. |

### 6.5 Authentication Semantics

SMQ does not mandate a credential format. BAS passes the credentials string and an
information table to the broker's `authenticate(credentials, info)` callback.

The credentials field may contain:

- A shared key
- `username:password`
- A challenge/response hash derived from the `Init` seed and IP address
- Any application-defined byte string

BAS browser clients can also rely on HTTP/Web authentication. When a browser has
already authenticated with BAS, the SMQ broker can inspect the session and username
in the `info` table. This is one reason SMQ integrates well with browser applications:
the WebSocket or raw socket connection can inherit web authentication context.

SMQ can also use mutual TLS (mTLS) when the transport is TLS protected. In this
mode, the client presents an X.509 certificate during the TLS handshake and BAS
validates the certificate before or during SMQ authentication. The broker can reject
the connection with `0x04` when a client certificate is required but missing, or
`0x05` when the presented certificate is not accepted. mTLS may be used by itself or
combined with the SMQ credentials field and BAS authorization callbacks.

## 7. Topic and Sub-topic Control Packets

### 7.1 Subscribe

`Subscribe` requests delivery for a named topic. If the topic does not exist, the
broker may create a TID for it, subject to authorization.

| Field | Size | Description |
| --- | ---: | --- |
| Topic name | N&nbsp;bytes | Topic name string. |

When processing `Subscribe`, the broker may invoke the server-side
`permittop(topic, true, peer)` authorization callback. The broker returns the result
to the client in `Suback`: status `0x00` if the subscription is accepted, or status
`0x01` if it is denied.

### 7.2 Create

`Create` resolves or creates a TID for a named topic without subscribing the client.
Publishers commonly use this before publishing to a topic.

| Field | Size | Description |
| --- | ---: | --- |
| Topic name | N&nbsp;bytes | Topic name string. |

When processing `Create`, the broker may invoke the server-side
`permittop(topic, false, peer)` authorization callback. The broker returns the
result to the client in `Createack`: status `0x00` if the topic is accepted, or
status `0x01` if it is denied.

### 7.3 Createsub

`Createsub` resolves or creates a 32-bit ID for a sub-topic name.

| Field | Size | Description |
| --- | ---: | --- |
| Sub-topic name | N&nbsp;bytes | Sub-topic name string. |

When processing `Createsub`, the broker may invoke the server-side
`permitsubtop(subtopic, peer)` authorization callback. The broker returns the result
to the client in `Createsuback`: status `0x00` if the sub-topic is accepted, or
status `0x01` if it is denied.

### 7.4 Suback, Createack, and Createsuback

The acknowledgement layout is shared by `Suback`, `Createack`, and `Createsuback`.

| Field | Size | Description |
| --- | ---: | --- |
| Status | 1&nbsp;byte | `0x00` accepted, `0x01` denied. |
| ID | 4&nbsp;bytes | Topic TID or sub-topic ID. |
| Name | N&nbsp;bytes | Topic or sub-topic name from the request. |

If the status is accepted, the client should cache the name-to-ID and ID-to-name
mapping. If denied, pending publishes or subscriptions that depend on the unresolved
name should fail or be discarded according to the client API's rules.

## 8. Publish Packets

### 8.1 Publish

`Publish` sends application data to a destination topic TID or peer ETID.

| Field | Size | Description |
| --- | ---: | --- |
| Destination TID | 4&nbsp;bytes | Topic TID for one-to-many delivery or ETID for one-to-one delivery. |
| Publisher ETID | 4&nbsp;bytes | ETID assigned to the publishing client by `Connack`. |
| Sub-topic ID | 4&nbsp;bytes | Sub-topic ID, or `0` if not used. |
| Payload | N&nbsp;bytes | Application data. May be text, JSON, or binary by convention. |

The broker must verify that `Publisher ETID` matches the connected client. BAS
documentation states that if a client publishes with an incorrect ETID, the broker
drops the message and closes the connection.

The third 32-bit field is the sub-topic ID. Applications that need a message
identifier or correlation value should place it in the payload or encode it as an
application-level convention.

### 8.2 Routing Rules

If `Destination TID` is a named topic TID, the broker forwards the message to all
subscribers for that TID. If a subscriber registered sub-topic-specific callbacks,
the client library dispatches by `Sub-topic ID`. A subscription without a sub-topic
acts as a catch-all for unmatched sub-topic IDs.

If `Destination TID` is an ETID, the broker forwards the message only to the connected
client that owns the ETID. This is one-to-one delivery.

If no destination exists or no subscribers match, the broker drops the message unless
a BAS `ondrop(data, ptid, tid, subtid, peer)` callback processes it.

### 8.3 Payload Encoding

The SMQ wire protocol treats payloads as bytes. Higher level APIs define conventions:

- JavaScript `publish(string)` encodes the string as UTF-8 bytes.
- JavaScript `pubjson(value)` JSON-encodes the value and sends UTF-8 bytes.
- JavaScript subscriptions may request raw bytes, text, or JSON conversion.
- Lua strings may contain arbitrary binary data.
- Lua tables published through BAS APIs are JSON-encoded.

Applications should document the payload encoding per topic or sub-topic.

### 8.4 PubFrag

`PubFrag` is intended for constrained devices that need to publish one logical message
in chunks. `PubFrag` is structurally identical to `Publish`, with the broker
assembling fragments before forwarding the complete message.

The topic ID is set to zero for intermediate fragments. The broker treats the fragment
with a non-zero topic ID as the last fragment.

## 9. Subscription Management

### 9.1 Unsubscribe

`Unsubscribe` removes the client's subscription to a topic.

| Field | Size | Description |
| --- | ---: | --- |
| TID | 4&nbsp;bytes | Topic ID to unsubscribe from. |

There is no acknowledgement packet. Client libraries remove local callbacks for the
topic when sending `Unsubscribe`.

### 9.2 Subscriber Callbacks

Client APIs typically deliver received publishes as:

```text
onmsg(payload, ptid, tid, subtid)
```

| Argument | Meaning |
| --- | --- |
| `payload` | Application payload bytes or decoded value. |
| `ptid` | Publisher ETID, used for direct replies. |
| `tid` | Destination TID that matched the subscription or ETID. |
| `subtid` | Sub-topic ID, or `0` if not used. |

## 10. Observe and Change

### 10.1 Observe

`Observe` asks the broker to send `Change` notifications for a TID or ETID.

| Field | Size | Description |
| --- | ---: | --- |
| TID | 4&nbsp;bytes | Topic TID or peer ETID to observe. |

When observing a named topic or topic TID:

- The client receives a `Change` notification each time a client subscribes,
  unsubscribes, or disconnects from that topic.
- Notifications are limited to clients connected to the same broker.
- In an SMQ cluster, named-topic change notifications are not propagated between
  brokers.

When observing an ETID:

- The client receives a `Change` notification when that specific client disconnects.
- Since an ETID has at most one owner, this is a peer-supervision mechanism.
- ETID disconnect notifications work in an SMQ cluster environment; a client can
  observe an ETID even when the observed client is connected to another broker node.
- This is similar to MQTT will-message style supervision, except the observed client
  does not need to predefine a will message.

### 10.2 Unobserve

`Unobserve` cancels a previous observation.

| Field | Size | Description |
| --- | ---: | --- |
| TID | 4&nbsp;bytes | Topic TID or peer ETID to stop observing. |

When observing an ETID, BAS client stacks automatically stop observing after the peer
disconnect event is received.

### 10.3 Change

`Change` is sent by the broker to an observing client.

| Field | Size | Description |
| --- | ---: | --- |
| TID | 4&nbsp;bytes | Observed topic TID or ETID. |
| Subscribers | 4&nbsp;bytes | Current subscriber count. For an ETID disconnect, this becomes `0`. |

## 11. Keepalive and Shutdown

### 11.1 Ping and Pong

`Ping` and `Pong` have no payload.

A peer receiving `Ping` should respond with `Pong`. Common BAS client defaults are:

- Send `Ping` after 120 seconds without received SMQ traffic.
- Wait 10 seconds for `Pong`.
- Close the socket if no `Pong` is received.

TCP keepalive may also be configured by BAS broker options, but SMQ `Ping`/`Pong`
works at the protocol level and detects application-level liveness.

### 11.2 Disconnect

`Disconnect` gracefully closes the SMQ connection. BAS broker management APIs can
also close peers with an optional reason message; clients should therefore tolerate a
`Disconnect` packet with or without a trailing human-readable reason.

After sending or receiving `Disconnect`, the endpoint should stop publishing and close
the underlying socket.

## 12. BAS Broker Behavior

### 12.1 Peer Table

BAS maintains a peer table for each connected client. Important fields include:

| Field | Meaning |
| --- | --- |
| `sock` | Active socket object. |
| `tid` | Client ETID. |
| `topics` | Subscribed topic TIDs. |
| `uid` | Client UID from `Connect`. |
| `info` | Optional client info string from `Connect`. |
| `phantom` | Cluster manager phantom connection marker, when used. |

Server-side code may read these values and may add its own fields, but must not modify
broker-managed fields.

### 12.2 Authorization Hooks

Applications can control broker-side SMQ access with server-side callbacks:

| Callback | Purpose |
| --- | --- |
| `authenticate(credentials, info)` | Accept or reject the connection. |
| `permittop(topic, issub, peer)` | Authorize topic creation and subscriptions. |
| `permitsubtop(subtopic, peer)` | Authorize sub-topic creation. |
| `onpublish(data, ptid, tid, subtid, peer)` | Inspect or reject each publish before routing. |
| `ondrop(data, ptid, tid, subtid, peer)` | Process messages with no destination. |
| `onconnect(tid, info, peer)` | Run server-side logic after a client is connected. |
| `onclose(tid, sock, peer, err)` | Run server-side logic after a client disconnects. |

These hooks are broker behavior, not separate wire packets. Their decisions are
reported through protocol responses such as `Connack`, `Suback`, `Createack`, and
`Createsuback`, or by forwarding or dropping a `Publish` packet.

## 13. Error Handling

An implementation should treat the following as protocol errors:

- Unknown non-reserved message type received in an established state.
- Packet shorter than the fixed fields required by its message type.
- Raw socket packet length that is smaller than the fixed header.
- `Publish` whose publisher ETID does not match the connected client.
- `Connect` with an unsupported version or malformed length fields.
- `Subscribe`, `Create`, or `Createsub` with an empty or invalid name, if rejected by
  application policy.

On protocol error, the receiver should close the connection. If the error occurs
during connection setup, the broker should prefer `Connack` with a non-zero return
code when it can still parse enough of the request to respond safely.

## 14. Security Considerations

SMQ security is normally provided by TLS plus BAS authentication and authorization:

- Use `wss://` or HTTPS/TLS raw socket connections when credentials or sensitive data
  are sent.
- Do not send clear-text passwords over non-secure connections.
- If using hash-based credentials, include the `Init` seed in the hash to prevent
  replay and precomputed dictionary attacks.
- BAS can combine SMQ with HTTP authentication, form login, sessions, or client
  certificates.
- Use `permittop`, `permitsubtop`, and `onpublish` to enforce application-level
  authorization.
- Treat ETIDs as temporary addresses, not authentication credentials. A client can
  reply to a PTID it learned, but authorization should still be enforced by the
  broker for sensitive operations.

## 15. MQTT Comparison Guide

| MQTT concept | SMQ equivalent or difference |
| --- | --- |
| Broker | SMQ broker/hub instance. |
| Topic string in publish packet | SMQ resolves topic strings to 32-bit TIDs before publish. |
| Client ID | SMQ UID plus broker-assigned ETID. UID identifies the client; ETID is the temporary routing address. |
| QoS 0/1/2 | No MQTT-style QoS levels are defined by the BAS SMQ protocol documentation. |
| Retained message | No retained-message feature is defined by the BAS SMQ protocol documentation. |
| Persistent session | Clients may locally replay subscriptions and pending messages unless `cleanstart` is set, but this differs from MQTT broker-managed sessions. Local replay is difficult to implement correctly and is not recommended for most applications; a reconnecting application should normally perform a full setup and resubscribe to all required topics. |
| Response topic / correlation data | SMQ includes PTID in every received message. PTID is the publisher's TID; in other words, the publisher's ETID. A receiver can send a direct reply by publishing to this PTID. Applications may use sub-topics or payload fields for correlation. |
| Will message | SMQ `Observe` can supervise a peer ETID and receive disconnect notification without the observed client pre-registering a will. |
| Topic aliases | SMQ's TID mapping is a core protocol feature and persists for the broker lifetime. |
| Shared subscriptions | Not defined in the BAS SMQ documentation used for this specification. |
| User properties | Not defined at the SMQ wire level; applications can encode metadata in payloads or sub-topics. |

## 16. Broker Termination Example

SMQ Broker enables IoT SSL termination. An external browser can use secure
WebSockets to publish through the broker, while internal devices use non-secure
SMQ client connections on a protected LAN. The broker decrypts the browser message
and forwards the unencrypted result to the non-secure devices. The reverse direction
also works: a non-secure internal client can publish to a secure browser client
through the broker.

![SMQ Broker termination diagram](SMQ-specification_files/smq-broker-termination.jpg)

## 17. Minimal Packet Reference

Raw socket packets include the two-byte packet length before the fields shown below.
WebSocket packets omit that length and start at `type`.

| Packet | Type | Body |
| --- | ---: | --- |
| `Init` | 1 | `uint8 version`, `uint32 seed`, `bytes ipAddress` |
| `Connect` | 2 | `uint8 version`, `uint8 uidLen`, `bytes uid`, `uint8 credentialsLen`, `bytes credentials`, `bytes info` |
| `Connack` | 3 | `uint8 returnCode`, `uint32 etid`, `bytes message` |
| `Subscribe` | 4 | `bytes topicName` |
| `Suback` | 5 | `uint8 status`, `uint32 tid`, `bytes topicName` |
| `Create` | 6 | `bytes topicName` |
| `Createack` | 7 | `uint8 status`, `uint32 tid`, `bytes topicName` |
| `Publish` | 8 | `uint32 destinationTid`, `uint32 publisherEtid`, `uint32 subtid`, `bytes payload` |
| `Unsubscribe` | 9 | `uint32 tid` |
| `Disconnect` | 11 | Empty, or optional reason in BAS extensions |
| `Ping` | 12 | Empty |
| `Pong` | 13 | Empty |
| `Observe` | 14 | `uint32 tidOrEtid` |
| `Unobserve` | 15 | `uint32 tidOrEtid` |
| `Change` | 16 | `uint32 tidOrEtid`, `uint32 subscribers` |
| `Createsub` | 17 | `bytes subtopicName` |
| `Createsuback` | 18 | `uint8 status`, `uint32 subtid`, `bytes subtopicName` |
| `PubFrag` | 19 | Same fixed fields as `Publish`; fragment semantics are stack-specific |
