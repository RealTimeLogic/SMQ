# Simple Message Queue (SMQ)

Simple Message Queue (SMQ) is a lightweight publish-subscribe protocol for
real-time device, application, and browser communication. It provides
MQTT-like pub/sub messaging and extends the model with features such as
one-to-one messaging and sender addressing, which are useful for device
management and interactive applications.

SMQ clients connect to an SMQ broker and exchange messages by topic. Payloads
can be binary data, JSON, or application-specific formats. The protocol can be
used over plain TCP or secure transports, depending on the client
implementation and deployment requirements.

![SMQ IoT Protocol](https://makoserver.net/GZ/images/SMQ-IoT-Broker.svg)

The SMQ broker is included with
[Barracuda App Server](https://realtimelogic.com/products/barracuda-application-server/)
and can be started easily with
[Mako Server](https://makoserver.net/), a Barracuda App Server derivative.
The [C](C/README.md) and [Python](Python/README.md) documentation include
local broker setup instructions.

## SMQ Documentation

- [Product page (intro)](https://realtimelogic.com/products/simplemq/)
- [Main documentation](https://realtimelogic.com/ba/doc/en/SMQ.html)
- [Specification](specification/SMQ-specification.md)
- [AI instructions](specification/SMQ-client-generation-spec.md) - for creating new implementations

## Included Implementations

- [C client library](C/README.md)
- [Java and Android client library](Java/README.md)
- [Python client library](Python/README.md)

## License

The source code in this directory is released under the
[Eclipse Public License 2.0](https://www.eclipse.org/legal/epl-v20.html).

> EPL-2.0 is a commercial-friendly open source license that permits use in proprietary and commercial products, with limited copyleft obligations applying primarily to modifications of EPL-licensed code itself.

You may compile a Program licensed under the EPL without modification and
commercially license the result in accordance with the
[terms of the EPL](https://www.eclipse.org/legal/epl-2.0/faq.php).

This Source Code may also be made available under the following Secondary
Licenses when the conditions for such availability set forth in the Eclipse
Public License, v. 2.0 are satisfied: **GNU General Public License, version 2**.
