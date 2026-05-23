"""One-to-one SMQ request/reply example using ETIDs.

Client A subscribes to its own ETID with sub-topic "rpc.ping". Client B sends a
direct message to A's ETID. A replies directly to B's publisher ETID (PTID) with
sub-topic "rpc.pong".

Run with the public SimpleMQ broker:

    python examples/rpc_ping.py

Run with a local broker:

    python examples/rpc_ping.py http://localhost/smq.lsp
"""

from __future__ import annotations

import sys
import threading
import uuid
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from smqclient import SMQClient


url = sys.argv[1] if len(sys.argv) > 1 else "https://simplemq.com/smq.lsp"
ready_a = threading.Event()
ready_b = threading.Event()
reply_seen = threading.Event()
state = {"a_etid": None, "rpc_pong_subtid": None}


def on_a_connect(etid, rnd, ipaddr):
    state["a_etid"] = etid
    print("A connected as", etid)

    def on_ping(data, ptid, tid, subtid):
        print("A received:", data.decode("utf-8"), "from", ptid)
        a.publish("pong", ptid, state["rpc_pong_subtid"] or "rpc.pong")

    a.subscribe(
        "self",
        "rpc.ping",
        {
            "onmsg": on_ping,
            "onack": lambda accepted, *_: ready_a.set() if accepted else None,
        },
    )


def on_b_connect(etid, rnd, ipaddr):
    print("B connected as", etid)

    def on_pong(data, ptid, tid, subtid):
        print("B received:", data.decode("utf-8"), "from", ptid)
        reply_seen.set()

    b.subscribe(
        "self",
        "rpc.pong",
        {
            "onmsg": on_pong,
            "onack": lambda accepted, *_: ready_b.set() if accepted else None,
        },
    )


def on_close(name):
    def handle(reason, can_reconnect):
        print(f"{name} closed:", reason)
        return False

    return handle


a = SMQClient.create(
    url,
    {
        "uid": "py-rpc-a-" + uuid.uuid4().hex[:12],
        "info": "Python RPC ping example A",
        "onconnect": on_a_connect,
        "onclose": on_close("A"),
        "reconnect": False,
    },
)
b = SMQClient.create(
    url,
    {
        "uid": "py-rpc-b-" + uuid.uuid4().hex[:12],
        "info": "Python RPC ping example B",
        "onconnect": on_b_connect,
        "onclose": on_close("B"),
        "reconnect": False,
    },
)

try:
    print("connecting to", url)
    if not ready_a.wait(10):
        raise SystemExit("client A not ready")
    if not ready_b.wait(10):
        raise SystemExit("client B not ready")

    state["rpc_pong_subtid"] = b.subtopic2tid("rpc.pong")
    print("B sends ping directly to A ETID", state["a_etid"])
    b.publish("ping", state["a_etid"], "rpc.ping")

    if not reply_seen.wait(10):
        raise SystemExit("reply timed out")
finally:
    a.disconnect()
    b.disconnect()
