"""Small Python client for the BAS Simple Message Queues (SMQ) protocol.

The client uses the raw SMQ transport by default: an HTTP(S) bootstrap request
with ``SimpleMQ: 1`` and ``SendSmqHttpResponse: true`` followed by SMQ binary
packets. Public methods are thread-safe. Lifecycle and message callbacks run on
the client's background receive thread.
"""

from __future__ import annotations

import json
import os
import socket
import ssl
import struct
import threading
import time
import uuid
from collections import defaultdict
from dataclasses import dataclass, field
from typing import Any, Callable, Dict, List, Mapping, Optional, Tuple, Union
from urllib.parse import urlparse


BytesLike = Union[bytes, bytearray, memoryview]
Payload = Union[str, BytesLike]
Topic = Union[str, int]
Callback = Callable[..., Any]


MSG_INIT = 1
MSG_CONNECT = 2
MSG_CONNACK = 3
MSG_SUBSCRIBE = 4
MSG_SUBACK = 5
MSG_CREATE = 6
MSG_CREATEACK = 7
MSG_PUBLISH = 8
MSG_UNSUBSCRIBE = 9
MSG_DISCONNECT = 11
MSG_PING = 12
MSG_PONG = 13
MSG_OBSERVE = 14
MSG_UNOBSERVE = 15
MSG_CHANGE = 16
MSG_CREATESUB = 17
MSG_CREATESUBACK = 18


CONNACK_ERRORS = {
    0x01: "unacceptable protocol version",
    0x02: "server unavailable",
    0x03: "incorrect credentials",
    0x04: "client certificate required",
    0x05: "client certificate not accepted",
    0x06: "access denied",
}


class SMQError(Exception):
    """Base exception raised by the SMQ client."""


class SMQProtocolError(SMQError):
    """Raised when the broker sends malformed or unexpected SMQ data."""


@dataclass
class SMQOptions:
    uid: Optional[Payload] = None
    info: Optional[Payload] = None
    timeout: float = 5.0
    ping: float = 120.0
    pong: float = 10.0
    cleanstart: bool = True
    reconnect: bool = True
    reconnect_delay: float = 5.0
    max_reconnect_delay: Optional[float] = None
    headers: Mapping[str, str] = field(default_factory=dict)


@dataclass
class _MessageCallbacks:
    catch_all: Optional[Callback] = None
    subtopics: Dict[int, Callback] = field(default_factory=dict)
    datatypes: Dict[Optional[int], Optional[str]] = field(default_factory=dict)


class _CreateDescriptor:
    def __get__(self, obj: Optional["SMQClient"], owner: type["SMQClient"]) -> Callable[..., Any]:
        if obj is None:
            def create_client(url_or_connector: Union[str, Callable[..., Any]], options: Optional[Mapping[str, Any]] = None) -> "SMQClient":
                client = owner(url_or_connector, options)
                client.start()
                return client

            return create_client
        return obj._create_topic


class SMQClient:
    """Threaded SMQ client.

    Set callback attributes directly, or pass them in the options mapping:
    ``onauth``, ``onconnect``, ``onreconnect``, ``onclose``, and ``onmsg``.
    """

    def __init__(self, url_or_connector: Union[str, Callable[..., Any]], options: Optional[Mapping[str, Any]] = None):
        if not isinstance(url_or_connector, str) and not callable(url_or_connector):
            raise TypeError("url_or_connector must be a URL string or connector callable")

        raw_options = dict(options or {})
        callbacks = {name: raw_options.pop(name, None) for name in ("onauth", "onconnect", "onreconnect", "onclose", "onmsg")}

        self.url_or_connector = url_or_connector
        self.options = SMQOptions(**raw_options)
        self.onauth: Optional[Callable[[int, str], Payload]] = callbacks["onauth"]
        self.onconnect: Optional[Callable[[int, int, str], Any]] = callbacks["onconnect"]
        self.onreconnect: Optional[Callable[[int, int, str], Any]] = callbacks["onreconnect"]
        self.onclose: Optional[Callable[[str, bool], Any]] = callbacks["onclose"]
        self.onmsg: Optional[Callable[[bytes, int, int, int], Any]] = callbacks["onmsg"]

        self._lock = threading.RLock()
        self._send_lock = threading.Lock()
        self._stop = threading.Event()
        self._connected = threading.Event()
        self._thread: Optional[threading.Thread] = None
        self._sock: Optional[socket.socket] = None
        self._recv_buffer = bytearray()
        self._ever_connected = False
        self._manual_disconnect = False

        self.etid: Optional[int] = None
        self.topic_name_to_tid: Dict[str, int] = {}
        self.tid_to_topic_name: Dict[int, str] = {}
        self.subtopic_name_to_subtid: Dict[str, int] = {}
        self.subtid_to_subtopic_name: Dict[int, str] = {}
        self.pending_topic_acks: Dict[str, List[Callback]] = defaultdict(list)
        self.pending_subscribe_acks: Dict[str, List[Callback]] = defaultdict(list)
        self.pending_subtopic_acks: Dict[str, List[Callback]] = defaultdict(list)
        self.message_callbacks: Dict[int, _MessageCallbacks] = {}
        self.observe_callbacks: Dict[int, Tuple[Topic, Callback]] = {}
        self._subscribed_tids: set[int] = set()

    create = _CreateDescriptor()

    def start(self) -> None:
        with self._lock:
            if self._thread and self._thread.is_alive():
                return
            self._stop.clear()
            self._manual_disconnect = False
            self._thread = threading.Thread(target=self._connect_loop, name="SMQClient", daemon=True)
            self._thread.start()

    def wait_connected(self, timeout: Optional[float] = None) -> bool:
        return self._connected.wait(timeout)

    def create_topic(self, topic: str, onack: Optional[Callable[[bool, str, int], Any]] = None) -> bool:
        return self.create(topic, onack)

    def _create_topic(self, topic: str, onack: Optional[Callable[[bool, str, int], Any]] = None) -> bool:
        self._validate_name(topic, "topic")
        onack = onack or (lambda *_: None)
        with self._lock:
            tid = self.topic_name_to_tid.get(topic)
            if tid is not None:
                self._call(onack, True, topic, tid)
                return True
            if not self._is_connected_locked():
                return False
            first = topic not in self.pending_topic_acks
            self.pending_topic_acks[topic].append(onack)
        if first:
            return self._send_packet(MSG_CREATE, self._to_bytes(topic))
        return True

    def createsub(self, subtopic: str, onack: Optional[Callable[[bool, str, int], Any]] = None) -> bool:
        self._validate_name(subtopic, "subtopic")
        onack = onack or (lambda *_: None)
        with self._lock:
            subtid = self.subtopic_name_to_subtid.get(subtopic)
            if subtid is not None:
                self._call(onack, True, subtopic, subtid)
                return True
            if not self._is_connected_locked():
                return False
            first = subtopic not in self.pending_subtopic_acks
            self.pending_subtopic_acks[subtopic].append(onack)
        if first:
            return self._send_packet(MSG_CREATESUB, self._to_bytes(subtopic))
        return True

    def publish(self, data: Payload, topic_or_tid_or_etid: Topic, subtopic_or_subtid: Optional[Topic] = None) -> bool:
        payload = self._payload_to_bytes(data)

        def after_topic(ok: bool, _name: str, tid: int) -> None:
            if ok:
                self._resolve_subtopic_then(subtopic_or_subtid, lambda subtid: self._publish_resolved(payload, tid, subtid))

        with self._lock:
            if not self._is_connected_locked() or self.etid is None:
                return False
            if isinstance(topic_or_tid_or_etid, int):
                tid = topic_or_tid_or_etid
            elif topic_or_tid_or_etid == "self":
                tid = self.etid
            else:
                tid = self.topic_name_to_tid.get(topic_or_tid_or_etid)

        if tid is None:
            if not isinstance(topic_or_tid_or_etid, str):
                raise TypeError("topic must be a string or integer TID/ETID")
            return self.create(topic_or_tid_or_etid, after_topic)

        return self._resolve_subtopic_then(subtopic_or_subtid, lambda subtid: self._publish_resolved(payload, tid, subtid))

    def pubjson(self, value: Any, topic_or_tid_or_etid: Topic, subtopic_or_subtid: Optional[Topic] = None) -> bool:
        return self.publish(json.dumps(value, separators=(",", ":")), topic_or_tid_or_etid, subtopic_or_subtid)

    def subscribe(
        self,
        topic_or_self: Topic,
        subtopic_or_subtid: Optional[Union[Topic, Callback, Mapping[str, Any]]] = None,
        settings: Optional[Union[Callback, Mapping[str, Any]]] = None,
    ) -> bool:
        subtopic, settings_dict = self._normalize_subscribe_args(subtopic_or_subtid, settings)
        onmsg = settings_dict.get("onmsg")
        onack = settings_dict.get("onack")
        datatype = settings_dict.get("datatype")
        if settings_dict.get("json") and datatype is None:
            datatype = "json"

        def register(tid: int, subtid: Optional[int]) -> None:
            if onmsg:
                with self._lock:
                    callbacks = self.message_callbacks.setdefault(tid, _MessageCallbacks())
                    if subtid is None:
                        callbacks.catch_all = onmsg
                        callbacks.datatypes[None] = datatype
                    else:
                        callbacks.subtopics[subtid] = onmsg
                        callbacks.datatypes[subtid] = datatype

        def after_topic(ok: bool, topic_name: Union[str, int], tid: int, subtid: Optional[int]) -> None:
            if ok:
                register(tid, subtid)
            if onack:
                self._call(onack, ok, topic_name, tid, subtopic, subtid)

        def after_subtopic(subtid: Optional[int]) -> bool:
            with self._lock:
                if not self._is_connected_locked() or self.etid is None:
                    return False
                if topic_or_self == "self":
                    after_topic(True, "self", self.etid, subtid)
                    return True
                if isinstance(topic_or_self, int):
                    after_topic(True, topic_or_self, topic_or_self, subtid)
                    return True
            if not isinstance(topic_or_self, str):
                raise TypeError("topic must be a string, 'self', or integer TID")

            def topic_ack(ok: bool, topic_name: str, tid: int) -> None:
                after_topic(ok, topic_name, tid, subtid)

            return self._subscribe_topic(topic_or_self, topic_ack)

        if subtopic is None:
            return after_subtopic(None)
        if isinstance(subtopic, int):
            return after_subtopic(subtopic)
        if not isinstance(subtopic, str):
            raise TypeError("subtopic must be a string or integer sub-topic ID")

        return self.createsub(subtopic, lambda ok, _subtopic, subtid: after_subtopic(subtid) if ok else self._call(onack, False, topic_or_self, 0, subtopic, 0))

    def unsubscribe(self, topic_or_tid: Topic) -> bool:
        with self._lock:
            tid = self._known_tid_locked(topic_or_tid)
            if tid is None:
                return False
            self.message_callbacks.pop(tid, None)
            self._subscribed_tids.discard(tid)
        return self._send_packet(MSG_UNSUBSCRIBE, struct.pack(">I", tid))

    def observe(self, topic_or_tid_or_etid: Topic, onchange: Callable[[int, Topic], Any]) -> bool:
        def send_observe(tid: int, label: Topic) -> bool:
            with self._lock:
                self.observe_callbacks[tid] = (label, onchange)
            return self._send_packet(MSG_OBSERVE, struct.pack(">I", tid))

        with self._lock:
            if not self._is_connected_locked():
                return False
            tid = self._known_tid_locked(topic_or_tid_or_etid)
        if tid is not None:
            return send_observe(tid, topic_or_tid_or_etid)
        if isinstance(topic_or_tid_or_etid, str):
            return self.create(topic_or_tid_or_etid, lambda ok, name, new_tid: send_observe(new_tid, name) if ok else None)
        raise TypeError("observe target must be a topic name, TID, or ETID")

    def unobserve(self, topic_or_tid_or_etid: Topic) -> bool:
        with self._lock:
            tid = self._known_tid_locked(topic_or_tid_or_etid)
            if tid is None:
                return False
            self.observe_callbacks.pop(tid, None)
        return self._send_packet(MSG_UNOBSERVE, struct.pack(">I", tid))

    def disconnect(self) -> None:
        self._manual_disconnect = True
        self._stop.set()
        if self._connected.is_set():
            self._send_packet(MSG_DISCONNECT, b"")
        self._close_socket()
        self._connected.clear()

    close = disconnect

    def gettid(self) -> Optional[int]:
        return self.etid

    def topic2tid(self, topic: str) -> Optional[int]:
        with self._lock:
            return self.topic_name_to_tid.get(topic)

    def tid2topic(self, tid: int) -> Optional[str]:
        with self._lock:
            return self.tid_to_topic_name.get(tid)

    def subtopic2tid(self, subtopic: str) -> Optional[int]:
        with self._lock:
            return self.subtopic_name_to_subtid.get(subtopic)

    def tid2subtopic(self, subtid: int) -> Optional[str]:
        with self._lock:
            return self.subtid_to_subtopic_name.get(subtid)

    def _connect_loop(self) -> None:
        delay = self.options.reconnect_delay
        while not self._stop.is_set():
            reason = "closed"
            can_reconnect = self.options.reconnect and not self._manual_disconnect
            try:
                self._reset_connection_state()
                sock, initial = self._open_transport()
                self._sock = sock
                self._recv_buffer = bytearray(initial)
                sock.settimeout(0.5)
                rnd, ipaddr = self._handshake()
                self._connected.set()
                delay = self.options.reconnect_delay
                self._announce_connected(rnd, ipaddr)
                reason = self._receive_loop()
                can_reconnect = self.options.reconnect and not self._manual_disconnect and reason != "disconnect"
            except Exception as exc:  # callbacks must still receive protocol/connect failures
                reason = str(exc) or exc.__class__.__name__
                can_reconnect = self.options.reconnect and not self._manual_disconnect
            finally:
                self._connected.clear()
                self.etid = None
                self._close_socket()

            if self._stop.is_set() or self._manual_disconnect:
                break
            decision = self._handle_close(reason, can_reconnect)
            if not decision:
                break
            sleep_for = decision if isinstance(decision, (int, float)) else delay
            if self.options.max_reconnect_delay is not None:
                sleep_for = min(sleep_for, self.options.max_reconnect_delay)
                delay = min(delay * 2, self.options.max_reconnect_delay)
            else:
                delay = max(delay, self.options.reconnect_delay)
            self._stop.wait(max(0.0, float(sleep_for)))

    def _open_transport(self) -> Tuple[socket.socket, bytes]:
        if callable(self.url_or_connector):
            sock = self.url_or_connector(self.options)
            if not isinstance(sock, socket.socket):
                raise TypeError("connector must return a socket.socket")
            return sock, b""

        parsed = urlparse(self.url_or_connector)
        scheme = parsed.scheme.lower()
        if scheme not in ("http", "https"):
            raise ValueError("SMQ raw transport expects an http:// or https:// broker URL")

        host = parsed.hostname
        if not host:
            raise ValueError("broker URL must include a host")
        port = parsed.port or (443 if scheme == "https" else 80)
        path = parsed.path or "/"
        if parsed.query:
            path += "?" + parsed.query

        raw = socket.create_connection((host, port), timeout=self.options.timeout)
        if scheme == "https":
            context = ssl.create_default_context()
            sock = context.wrap_socket(raw, server_hostname=host)
        else:
            sock = raw
        sock.settimeout(self.options.timeout)

        headers = {
            "Host": f"{host}:{port}" if parsed.port else host,
            "User-Agent": "smqclient.py",
            "Connection": "keep-alive",
            "SimpleMQ": "1",
            "SendSmqHttpResponse": "true",
        }
        headers.update(self.options.headers)
        request = [f"GET {path} HTTP/1.1", *(f"{k}: {v}" for k, v in headers.items()), "", ""]
        sock.sendall("\r\n".join(request).encode("ascii"))

        header_bytes = bytearray()
        while b"\r\n\r\n" not in header_bytes:
            chunk = sock.recv(4096)
            if not chunk:
                raise ConnectionError("HTTP bootstrap closed before response")
            header_bytes.extend(chunk)
            if len(header_bytes) > 65536:
                raise ConnectionError("HTTP bootstrap response too large")
        head, initial = bytes(header_bytes).split(b"\r\n\r\n", 1)
        status_line = head.split(b"\r\n", 1)[0].decode("iso-8859-1", "replace")
        parts = status_line.split()
        if len(parts) < 2 or parts[1] != "200":
            raise ConnectionError(f"HTTP bootstrap failed: {status_line}")
        return sock, initial

    def _handshake(self) -> Tuple[int, str]:
        msg_type, body = self._recv_packet(timeout=self.options.timeout)
        if msg_type != MSG_INIT or len(body) < 5:
            raise SMQProtocolError("expected Init packet")
        version = body[0]
        if version != 1:
            raise SMQProtocolError(f"unsupported SMQ version {version}")
        rnd = struct.unpack(">I", body[1:5])[0]
        ipaddr = body[5:].decode("utf-8", "replace")

        uid = self._uid_bytes(ipaddr)
        credentials = self._payload_to_bytes(self.onauth(rnd, ipaddr)) if self.onauth else b""
        info = self._payload_to_bytes(self.options.info or b"")
        if len(uid) > 255:
            raise ValueError("SMQ UID is limited to 255 bytes")
        if len(credentials) > 255:
            raise ValueError("SMQ credentials are limited to 255 bytes")
        connect_body = b"".join((b"\x01", bytes([len(uid)]), uid, bytes([len(credentials)]), credentials, info))
        self._send_packet(MSG_CONNECT, connect_body)

        msg_type, body = self._recv_packet(timeout=self.options.timeout)
        if msg_type != MSG_CONNACK or len(body) < 5:
            raise SMQProtocolError("expected Connack packet")
        status = body[0]
        etid = struct.unpack(">I", body[1:5])[0]
        if status != 0:
            message = body[5:].decode("utf-8", "replace") or CONNACK_ERRORS.get(status, f"connack refused with status {status}")
            raise ConnectionError(message)
        with self._lock:
            self.etid = etid
            self.topic_name_to_tid["self"] = etid
            self.tid_to_topic_name[etid] = "self"
        return rnd, ipaddr

    def _receive_loop(self) -> str:
        last_received = time.monotonic()
        ping_sent_at: Optional[float] = None
        while not self._stop.is_set():
            now = time.monotonic()
            if ping_sent_at is not None and now - ping_sent_at > self.options.pong:
                return "missing pong"
            if ping_sent_at is None and now - last_received > self.options.ping:
                if not self._send_packet(MSG_PING, b""):
                    return "send failed"
                ping_sent_at = now

            try:
                msg_type, body = self._recv_packet(timeout=0.5)
            except TimeoutError:
                continue
            except Exception as exc:
                return str(exc) or exc.__class__.__name__

            last_received = time.monotonic()
            ping_sent_at = None
            if msg_type == MSG_SUBACK:
                self._handle_topic_ack(body, self.pending_subscribe_acks, subscribed=True)
            elif msg_type == MSG_CREATEACK:
                self._handle_topic_ack(body, self.pending_topic_acks, subscribed=False)
            elif msg_type == MSG_CREATESUBACK:
                self._handle_subtopic_ack(body)
            elif msg_type == MSG_PUBLISH:
                self._handle_publish(body)
            elif msg_type == MSG_PING:
                self._send_packet(MSG_PONG, b"")
            elif msg_type == MSG_PONG:
                pass
            elif msg_type == MSG_CHANGE:
                self._handle_change(body)
            elif msg_type == MSG_DISCONNECT:
                return body.decode("utf-8", "replace") or "disconnect"
            else:
                return f"unexpected packet type {msg_type}"
        return "closed"

    def _recv_packet(self, timeout: Optional[float]) -> Tuple[int, bytes]:
        sock = self._sock
        if sock is None:
            raise ConnectionError("not connected")
        previous_timeout = sock.gettimeout()
        sock.settimeout(timeout)
        try:
            while len(self._recv_buffer) < 2:
                chunk = sock.recv(4096)
                if not chunk:
                    raise ConnectionError("socket closed")
                self._recv_buffer.extend(chunk)
            length = struct.unpack(">H", self._recv_buffer[:2])[0]
            if length < 3:
                raise SMQProtocolError("invalid SMQ packet length")
            while len(self._recv_buffer) < length:
                chunk = sock.recv(4096)
                if not chunk:
                    raise ConnectionError("socket closed")
                self._recv_buffer.extend(chunk)
            packet = bytes(self._recv_buffer[:length])
            del self._recv_buffer[:length]
        except socket.timeout as exc:
            raise TimeoutError() from exc
        finally:
            sock.settimeout(previous_timeout)
        return packet[2], packet[3:]

    def _send_packet(self, msg_type: int, body: bytes) -> bool:
        packet = struct.pack(">HB", len(body) + 3, msg_type) + body
        if len(packet) > 0xFFFF:
            raise ValueError("SMQ packet too large")
        sock = self._sock
        if sock is None:
            return False
        try:
            with self._send_lock:
                sock.sendall(packet)
            return True
        except OSError:
            self._close_socket()
            return False

    def _subscribe_topic(self, topic: str, onack: Callable[[bool, str, int], Any]) -> bool:
        self._validate_name(topic, "topic")
        with self._lock:
            tid = self.topic_name_to_tid.get(topic)
            if tid is not None and tid in self._subscribed_tids:
                self._call(onack, True, topic, tid)
                return True
            if not self._is_connected_locked():
                return False
            first = topic not in self.pending_subscribe_acks
            self.pending_subscribe_acks[topic].append(onack)
        if first:
            return self._send_packet(MSG_SUBSCRIBE, self._to_bytes(topic))
        return True

    def _publish_resolved(self, payload: bytes, tid: int, subtid: int) -> bool:
        with self._lock:
            if not self._is_connected_locked() or self.etid is None:
                return False
            body = struct.pack(">III", tid, self.etid, subtid) + payload
        return self._send_packet(MSG_PUBLISH, body)

    def _resolve_subtopic_then(self, subtopic_or_subtid: Optional[Topic], callback: Callable[[int], bool]) -> bool:
        if subtopic_or_subtid is None:
            return callback(0)
        if isinstance(subtopic_or_subtid, int):
            return callback(subtopic_or_subtid)
        if not isinstance(subtopic_or_subtid, str):
            raise TypeError("subtopic must be a string or integer sub-topic ID")
        with self._lock:
            subtid = self.subtopic_name_to_subtid.get(subtopic_or_subtid)
        if subtid is not None:
            return callback(subtid)
        return self.createsub(subtopic_or_subtid, lambda ok, _name, new_subtid: callback(new_subtid) if ok else None)

    def _handle_topic_ack(self, body: bytes, pending: Dict[str, List[Callback]], subscribed: bool) -> None:
        if len(body) < 5:
            raise SMQProtocolError("short topic ack")
        accepted = body[0] == 0
        tid = struct.unpack(">I", body[1:5])[0]
        topic = body[5:].decode("utf-8", "replace")
        callbacks: List[Callback]
        with self._lock:
            if accepted:
                self.topic_name_to_tid[topic] = tid
                self.tid_to_topic_name[tid] = topic
                if subscribed:
                    self._subscribed_tids.add(tid)
            callbacks = list(pending.pop(topic, []))
        for callback in callbacks:
            self._call(callback, accepted, topic, tid)

    def _handle_subtopic_ack(self, body: bytes) -> None:
        if len(body) < 5:
            raise SMQProtocolError("short subtopic ack")
        accepted = body[0] == 0
        subtid = struct.unpack(">I", body[1:5])[0]
        subtopic = body[5:].decode("utf-8", "replace")
        with self._lock:
            if accepted:
                self.subtopic_name_to_subtid[subtopic] = subtid
                self.subtid_to_subtopic_name[subtid] = subtopic
            callbacks = list(self.pending_subtopic_acks.pop(subtopic, []))
        for callback in callbacks:
            self._call(callback, accepted, subtopic, subtid)

    def _handle_publish(self, body: bytes) -> None:
        if len(body) < 12:
            raise SMQProtocolError("short publish")
        tid, ptid, subtid = struct.unpack(">III", body[:12])
        payload = body[12:]
        with self._lock:
            callbacks = self.message_callbacks.get(tid)
            callback: Optional[Callback] = None
            datatype: Optional[str] = None
            if callbacks:
                callback = callbacks.subtopics.get(subtid)
                datatype = callbacks.datatypes.get(subtid)
                if callback is None:
                    callback = callbacks.catch_all
                    datatype = callbacks.datatypes.get(None)
            callback = callback or self.onmsg

        data: Any = payload
        if datatype == "text":
            data = payload.decode("utf-8")
        elif datatype == "json":
            try:
                data = json.loads(payload.decode("utf-8"))
            except Exception:
                if callback is not self.onmsg and self.onmsg:
                    callback = self.onmsg
                    data = payload
                else:
                    return
        if callback:
            self._call(callback, data, ptid, tid, subtid)

    def _handle_change(self, body: bytes) -> None:
        if len(body) < 8:
            raise SMQProtocolError("short change")
        tid, subscribers = struct.unpack(">II", body[:8])
        with self._lock:
            entry = self.observe_callbacks.get(tid)
            topic = self.tid_to_topic_name.get(tid)
            if entry and topic is None and subscribers == 0:
                self.observe_callbacks.pop(tid, None)
        if entry:
            label, callback = entry
            self._call(callback, subscribers, topic or label)

    def _handle_close(self, reason: str, can_reconnect: bool) -> Union[bool, float]:
        decision: Any = None
        if self.onclose:
            decision = self._call(self.onclose, reason, can_reconnect)
        if decision is False:
            return False
        if isinstance(decision, (int, float)):
            return bool(can_reconnect) and float(decision)
        if decision is True:
            return bool(can_reconnect)
        return bool(can_reconnect and self.options.reconnect)

    def _announce_connected(self, rnd: int, ipaddr: str) -> None:
        callback = self.onreconnect if self._ever_connected and self.onreconnect else self.onconnect
        self._ever_connected = True
        if callback:
            self._call(callback, self.etid, rnd, ipaddr)

    def _reset_connection_state(self) -> None:
        with self._lock:
            self.etid = None
            self.topic_name_to_tid.clear()
            self.tid_to_topic_name.clear()
            self.subtopic_name_to_subtid.clear()
            self.subtid_to_subtopic_name.clear()
            self.pending_topic_acks.clear()
            self.pending_subscribe_acks.clear()
            self.pending_subtopic_acks.clear()
            self.message_callbacks.clear()
            self.observe_callbacks.clear()
            self._subscribed_tids.clear()
            self._recv_buffer.clear()

    def _close_socket(self) -> None:
        sock = self._sock
        self._sock = None
        if sock is None:
            return
        try:
            sock.shutdown(socket.SHUT_RDWR)
        except OSError:
            pass
        try:
            sock.close()
        except OSError:
            pass

    def _known_tid_locked(self, topic_or_tid: Topic) -> Optional[int]:
        if isinstance(topic_or_tid, int):
            return topic_or_tid
        if topic_or_tid == "self":
            return self.etid
        return self.topic_name_to_tid.get(topic_or_tid)

    def _is_connected_locked(self) -> bool:
        return self._connected.is_set() and self._sock is not None

    def _uid_bytes(self, ipaddr: str) -> bytes:
        if self.options.uid is None:
            self.options.uid = f"py-{uuid.uuid4().hex}"
        uid = self._payload_to_bytes(self.options.uid)
        if len(uid) < 6:
            uid += (ipaddr.encode("utf-8") or os.urandom(6))
            uid = uid[: max(6, len(uid))]
        return uid

    @staticmethod
    def _normalize_subscribe_args(
        subtopic_or_subtid: Optional[Union[Topic, Callback, Mapping[str, Any]]],
        settings: Optional[Union[Callback, Mapping[str, Any]]],
    ) -> Tuple[Optional[Topic], Dict[str, Any]]:
        subtopic: Optional[Topic] = None
        if settings is None and isinstance(subtopic_or_subtid, Mapping):
            settings_dict = dict(subtopic_or_subtid)
        elif settings is None and callable(subtopic_or_subtid):
            settings_dict = {"onmsg": subtopic_or_subtid}
        else:
            subtopic = subtopic_or_subtid if isinstance(subtopic_or_subtid, (str, int)) else None
            if settings is None:
                settings_dict = {}
            elif callable(settings):
                settings_dict = {"onmsg": settings}
            else:
                settings_dict = dict(settings)
        return subtopic, settings_dict

    @staticmethod
    def _to_bytes(value: Payload) -> bytes:
        if isinstance(value, str):
            return value.encode("utf-8")
        return SMQClient._payload_to_bytes(value)

    @staticmethod
    def _payload_to_bytes(value: Optional[Payload]) -> bytes:
        if value is None:
            return b""
        if isinstance(value, bytes):
            return value
        if isinstance(value, bytearray):
            return bytes(value)
        if isinstance(value, memoryview):
            return value.tobytes()
        if isinstance(value, str):
            return value.encode("utf-8")
        raise TypeError(f"expected bytes-like object or str, got {type(value).__name__}")

    @staticmethod
    def _validate_name(value: str, label: str) -> None:
        if not isinstance(value, str):
            raise TypeError(f"{label} must be a string")
        if not value:
            raise ValueError(f"{label} must not be empty")

    @staticmethod
    def _call(callback: Callback, *args: Any) -> Any:
        try:
            return callback(*args)
        except Exception:
            # User callbacks should not kill the receive thread.
            return None


__all__ = ["SMQClient", "SMQError", "SMQProtocolError"]
