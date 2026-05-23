Generate a Python SMQ client library using the specifications in this directory:

- `SMQ-specification.md`
- `SMQ-client-generation-spec.md`
- Save file as `smqclient.py`

Implement the API described in `SMQ-client-generation-spec.md`. The client must support:

- Automatic reconnect as part of the connection API.
- Topic and sub-topic resolution.
- Publish and subscribe.
- `subscribe("self", "subtopic", callback)` for one-to-one messages.
- One-to-one messaging using PTID/ETID.
- Observe/change notifications.
- Ping/pong keepalive.

Use the included Mako test broker to verify the implementation.

Start the broker from this directory with:

```powershell
mako -l::test-broker
```

The broker URL is normally:

```text
http://localhost/smq.lsp
```

However, Mako may not listen on port 80. Check the Mako startup output and use the printed host/port when constructing the broker URL, for example:

```text
http://localhost:<printed-port>/smq.lsp
```

No SMQ authentication is required.

Add focused tests or smoke scripts that demonstrate:

1. Connect and receive an ETID.
2. Create or resolve a topic.
3. Publish/subscribe on a named topic.
4. Subscribe to `self` with sub-topic `rpc.ping` and a dedicated callback.
5. Send a one-to-one message and reply directly to the sender's PTID.
6. Reconnect and perform a full setup/resubscribe after reconnect.

Prefer a small, maintainable implementation over a broad framework. Keep packet parsing and serialization well isolated from the public API.
