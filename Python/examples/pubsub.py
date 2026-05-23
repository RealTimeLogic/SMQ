"""Publish to and subscribe from an SMQ topic.

The example subscribes to a unique topic, publishes a text message to the same
broker topic, and receives the message back through its subscription.

Run with the public SimpleMQ broker:

    python examples/pubsub.py

Run with a local broker:

    python examples/pubsub.py http://localhost/smq.lsp
"""

from __future__ import annotations

import sys
import threading
import uuid
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from smqclient import SMQClient


url = sys.argv[1] if len(sys.argv) > 1 else "https://simplemq.com/smq.lsp"
topic = "python.example.chat." + uuid.uuid4().hex[:12]
received = threading.Event()


def setup():
    def on_message(data, ptid, tid, subtid):
        print("received:", data, "from", ptid)
        received.set()

    def on_suback(accepted, topic_name, tid, subtopic, subtid):
        if not accepted:
            raise RuntimeError("subscribe denied")
        print("subscribed:", topic_name, "tid:", tid, "subtid:", subtid)
        smq.publish("hello from Python", topic_name, subtopic)

    smq.subscribe(
        topic,
        "message",
        {"datatype": "text", "onmsg": on_message, "onack": on_suback},
    )


def onconnect(etid, rnd, ipaddr):
    print("connected as", etid)
    setup()


def onclose(reason, can_reconnect):
    print("closed:", reason)
    return False


smq = SMQClient.create(
    url,
    {
        "uid": "py-pubsub-" + uuid.uuid4().hex[:12],
        "info": "pubsub example",
        "onconnect": onconnect,
        "onclose": onclose,
        "reconnect": False,
    },
)

try:
    print("connecting to", url)
    if not received.wait(15):
        raise SystemExit("message timed out")
finally:
    smq.disconnect()
