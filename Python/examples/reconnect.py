"""Reconnect and resubscribe example.

This example keeps running until Ctrl+C. Stop and restart the broker to watch the
client reconnect and run setup again. With the public broker, it simply stays
connected and demonstrates where reconnect setup belongs.

Run with the public SimpleMQ broker:

    python examples/reconnect.py

Run with a local broker:

    python examples/reconnect.py http://localhost/smq.lsp
"""

from __future__ import annotations

import sys
import time
import uuid
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from smqclient import SMQClient


url = sys.argv[1] if len(sys.argv) > 1 else "https://simplemq.com/smq.lsp"
client_uid = "python-reconnect-" + uuid.uuid4().hex[:12]


def setup():
    smq.subscribe("python.example.status", "update", {"onmsg": on_status})
    smq.subscribe("self", "rpc.status", {"onmsg": on_status_rpc})
    print("setup complete")


def on_status(data, ptid, tid, subtid):
    print("status update:", data, "from", ptid)


def on_status_rpc(data, ptid, tid, subtid):
    print("status RPC from", ptid)
    smq.publish(b"ok", ptid, "rpc.status.reply")


def onconnect(etid, rnd, ipaddr):
    print("connected", etid)
    setup()


def onreconnect(etid, rnd, ipaddr):
    print("reconnected", etid)
    setup()


def onclose(reason, can_reconnect):
    print("closed:", reason, "can_reconnect:", can_reconnect)
    return 2 if can_reconnect else False


smq = SMQClient.create(
    url,
    {
        "uid": client_uid,
        "info": "Python reconnect example",
        "onconnect": onconnect,
        "onreconnect": onreconnect,
        "onclose": onclose,
        "reconnect": True,
        "reconnect_delay": 2,
    },
)

try:
    print("connecting to", url)
    print("press Ctrl+C to stop")
    while True:
        time.sleep(1)
except KeyboardInterrupt:
    print("stopping")
finally:
    smq.disconnect()
