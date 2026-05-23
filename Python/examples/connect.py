"""Connect to an SMQ broker and print the assigned ETID.

Run with the public SimpleMQ broker:

    python examples/connect.py

Run with a local broker:

    python examples/connect.py http://localhost/smq.lsp
"""

from __future__ import annotations

import sys
import time
import uuid
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from smqclient import SMQClient


url = sys.argv[1] if len(sys.argv) > 1 else "https://simplemq.com/smq.lsp"


def onconnect(etid, rnd, ipaddr):
    print("connected")
    print("etid:", etid)
    print("broker sees ip:", ipaddr)


def onclose(reason, can_reconnect):
    print("closed:", reason)
    return False


smq = SMQClient.create(
    url,
    {
        "uid": "py-connect-" + uuid.uuid4().hex[:12],
        "info": "connect example",
        "onconnect": onconnect,
        "onclose": onclose,
        "reconnect": False,
    },
)

try:
    print("connecting to", url)
    if not smq.wait_connected(10):
        raise SystemExit("connection timed out")
    time.sleep(1)
finally:
    smq.disconnect()
