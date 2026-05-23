"""Broker smoke tests for smqclient.py.

By default, this test connects to the online SimpleMQ broker over HTTPS/TLS:

    python tests/smq_smoke.py

To test with a local broker, start Mako and pass the broker URL:

    mako -l::test-broker
    python tests/smq_smoke.py http://localhost/smq.lsp

If Mako prints a different port, include it in the URL, for example:

    python tests/smq_smoke.py http://localhost:9357/smq.lsp
"""

from __future__ import annotations

import sys
import threading
import time
import uuid
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from smqclient import SMQClient


URL = sys.argv[1] if len(sys.argv) > 1 else "https://simplemq.com/smq.lsp"
TIMEOUT = 10.0


def wait(event: threading.Event, label: str, timeout: float = TIMEOUT) -> None:
    if not event.wait(timeout):
        raise TimeoutError(label)


def main() -> None:
    topic = f"python.smq.smoke.{uuid.uuid4().hex}"
    first_connect = threading.Event()
    reconnected = threading.Event()
    create_ack = threading.Event()
    sub_ack = threading.Event()
    self_ack = threading.Event()
    reconnect_sub_ack = threading.Event()
    b_self_ack = threading.Event()
    broadcast_seen = threading.Event()
    rpc_reply_seen = threading.Event()
    post_reconnect_seen = threading.Event()
    errors = []

    state = {"a_etid": None, "connects": 0, "rpc_pong_subtid": None}

    def record_error(reason, can_reconnect):
        errors.append((reason, can_reconnect))
        return True

    def setup_a(client: SMQClient, after_reconnect: bool = False) -> None:
        def on_broadcast(data, ptid, tid, subtid):
            if data == b"hello":
                broadcast_seen.set()
            if data == b"after-reconnect":
                post_reconnect_seen.set()

        def on_rpc_ping(data, ptid, tid, subtid):
            client.publish(b"pong", ptid, state["rpc_pong_subtid"] or "rpc.pong")

        def on_created(accepted, _topic, _tid):
            if not accepted:
                return
            create_ack.set()
            client.subscribe(
                topic,
                "broadcast",
                {
                    "onmsg": on_broadcast,
                    "onack": lambda ok, *_: (reconnect_sub_ack if after_reconnect else sub_ack).set() if ok else None,
                },
            )

        client.create(topic, on_created)
        client.subscribe("self", "rpc.ping", {"onmsg": on_rpc_ping, "onack": lambda accepted, *_: self_ack.set() if accepted else None})
        if after_reconnect:
            reconnected.set()

    def on_a_connect(etid, rnd, ipaddr):
        state["connects"] += 1
        state["a_etid"] = etid
        setup_a(a)
        first_connect.set()

    def on_a_reconnect(etid, rnd, ipaddr):
        state["connects"] += 1
        state["a_etid"] = etid
        setup_a(a, after_reconnect=True)

    a = SMQClient.create(
        URL,
        {
            "uid": f"py-a-{uuid.uuid4().hex[:12]}",
            "info": "smqclient smoke A",
            "reconnect": True,
            "reconnect_delay": 0.25,
            "timeout": 5,
            "ping": 5,
            "pong": 2,
            "onconnect": on_a_connect,
            "onreconnect": on_a_reconnect,
            "onclose": record_error,
        },
    )

    b_connected = threading.Event()

    def on_b_connect(etid, rnd, ipaddr):
        b_connected.set()

    def on_rpc_pong(data, ptid, tid, subtid):
        if data == b"pong":
            rpc_reply_seen.set()

    b = SMQClient.create(
        URL,
        {
            "uid": f"py-b-{uuid.uuid4().hex[:12]}",
            "info": "smqclient smoke B",
            "reconnect": False,
            "timeout": 5,
            "onconnect": on_b_connect,
        },
    )

    try:
        wait(first_connect, f"client A connect and ETID via {URL}; close errors: {errors}")
        assert a.gettid(), "client A did not receive an ETID"
        wait(create_ack, "topic create ack")
        wait(sub_ack, "topic subscribe ack")
        wait(self_ack, "self rpc.ping subscribe ack")
        wait(b_connected, "client B connect and ETID")

        topic_tid = a.topic2tid(topic)
        broadcast_subtid = a.subtopic2tid("broadcast")
        rpc_ping_subtid = a.subtopic2tid("rpc.ping")
        assert topic_tid and broadcast_subtid and rpc_ping_subtid, "client A setup did not cache IDs"

        b.publish(b"hello", topic_tid, broadcast_subtid)
        wait(broadcast_seen, "publish/subscribe message")

        b.subscribe("self", "rpc.pong", {"onmsg": on_rpc_pong, "onack": lambda accepted, *_: b_self_ack.set() if accepted else None})
        wait(b_self_ack, "client B self rpc.pong subscribe ack")
        state["rpc_pong_subtid"] = b.subtopic2tid("rpc.pong")
        b.publish(b"ping", state["a_etid"], rpc_ping_subtid)
        wait(rpc_reply_seen, "one-to-one PTID reply")

        # Force a transport failure so automatic reconnect can be verified.
        a._close_socket()
        wait(reconnected, "automatic reconnect")
        wait(reconnect_sub_ack, "post-reconnect topic subscribe ack")
        b.publish(b"after-reconnect", a.topic2tid(topic), a.subtopic2tid("broadcast"))
        wait(post_reconnect_seen, "post-reconnect resubscribe")

        print("ok: connect etid", state["a_etid"])
        print("ok: create/resolve topic", topic, a.topic2tid(topic))
        print("ok: publish/subscribe")
        print("ok: self rpc.ping callback")
        print("ok: one-to-one PTID reply")
        print("ok: reconnect and resubscribe")
    finally:
        a.disconnect()
        b.disconnect()


if __name__ == "__main__":
    main()
