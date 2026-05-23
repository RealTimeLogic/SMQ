# SMQ C Client Library

This directory contains the SMQ C client library, C/C++ examples, and porting
layers for computers, RTOS environments, and bare-metal systems.

## Resources

- [SMQ C client API](https://realtimelogic.com/ba/doc/en/C/reference/html/group__SMQClient.html)
- [C and C++ API concept](https://realtimelogic.com/ba/doc/?url=introduction.html#oo_c)

## SMQ C/C++ Examples

The following examples are listed in the recommended study order:

1. The [two introductory](#1-introductory-smq-examples) examples
   publish.cpp and subscribe.cpp are recommended for any C or C++
   developer new to the SMQ protocol.
2. The [Light Bulb Example](#2-light-bulb-example) is the companion
   example for an online tutorial.
3. The [IoT example](#3-smq-iot-example) `LED-SMQ.c` is the companion
   example for an IoT tutorial.

## 1: Introductory SMQ Examples

The two introductory examples [publish.cpp](examples/publish.cpp) and
[subscribe.cpp](examples/subscribe.cpp) use the C++ API, which is
slightly easier to use than the C API. If you are new to the library,
start with the [C and C++ API concept](https://realtimelogic.com/ba/doc/?url=introduction.html#oo_c).

The `publish.cpp` and `subscribe.cpp` examples require the
[JSON library](https://github.com/RealTimeLogic/JSON). The following commands
install the required build tools, fetch the JSON dependency, and compile the
examples on Linux from the repository root:

``` shell
sudo apt install g++ gcc make git
git clone https://github.com/RealTimeLogic/JSON.git JSON
cd C
make
```

If the JSON directory is not present, the Makefile skips `publish` and
`subscribe` and still builds the examples that do not require JSON.

**Windows users:** Compile and run the examples using the project files in
[VcMake](VcMake/).

The publish and subscribe examples require an SMQ broker running on
the same computer. Run the broker as follows:

- Download the precompiled
  [Mako Server](https://makoserver.net/download/overview/) for your
  platform and unpack the archive.
- Copy `mako` or `mako.exe` and `mako.zip` to this directory.
- Start the broker as follows:

Linux:

``` shell
sudo ./mako -u `whoami` -l::broker
```

Windows:

``` shell
mako -l::broker
```

The file [broker/.preload](broker/.preload) will now be loaded by the
Mako Server and you will see several lines being printed, including
the following:

``` shell
Server listening on IPv6 port 80
Server listening on IPv4 port 80
Loading certificate MakoServer
SharkSSL server listening on IPv6 port 443
SharkSSL server listening on IPv4 port 443
-------------- CREATING BROKER -----------------
Registering topic EXAMPLE_STRUCT_A     with tid 2
Registering topic EXAMPLE_STRUCT_B     with tid 3
Registering topic EXAMPLE_JSTRUCT_A    with tid 4
Registering topic EXAMPLE_JSON_ARRAY   with tid 5

For your C program:
#define EXAMPLE_STRUCT_A     2
#define EXAMPLE_STRUCT_B     3
#define EXAMPLE_JSTRUCT_A    4
#define EXAMPLE_JSON_ARRAY   5
------------------------------------------------
```

As shown above, the server is listening on port 80. If the server is
not listening on port 80, open the two examples publish.cpp and
subscribe.cpp in an editor, change the port number macro closer to the
top of the two files to the port number used by the Mako Server, and
recompile the examples.

Start the publisher and subscriber examples in separate terminal
windows. The publisher publishes data and the subscriber consumes
data.

For a short walkthrough, see
[running the example bundle](https://youtu.be/YQjBWQ2PZvM).

### Simplified C/C++ Design

The two C++ examples highlight several features that simplify
designing C/C++ applications using the SMQ protocol:

- The SMQ protocol registers
  [topics by name](https://realtimelogic.com/ba/doc/?url=SMQ.html#TopicNames),
  but translates the names to topic IDs (tids). In this example setup,
  the SMQ broker initialization in the script
  [broker/.preload](broker/.preload) pre-registers all topics used and
  forces the SMQ broker to use static tids instead of dynamic
  tids. This construction simplifies the C code, which would otherwise
  have to keep track of dynamically registered tids.
- The SMQ payload can be anything from binary data to JSON. The two
  examples send C structures as binary data between two C programs.
  This construction works as long as the two C programs are compiled
  for the same architecture and alignment.
- JSON simplifies sending structured messages between different
  architectures and computer languages. Most JSON libraries operate on
  complete messages, which can be problematic in an embedded system if
  the JSON payload is large. The two examples show how to send and
  receive complete messages, but also how to send JSON messages
  in chunks and how to parse the received JSON messages in chunks.

The Mako Server printouts shown above include #define
directives. These are already included in the two example programs. The
macro names correspond to the topic name (string), and the numbers are
the topic ID (tid) enforced by the [broker/.preload](broker/.preload)
Lua script.

- Topic EXAMPLE_STRUCT_A (tid 2): Payload is ExampleStructA, see
  [examples/ExampleStruct.h](examples/ExampleStruct.h).
- Topic EXAMPLE_STRUCT_B (tid 3): Payload is ExampleStructB, see
  [examples/ExampleStruct.h](examples/ExampleStruct.h).
- Topic EXAMPLE_JSTRUCT_A (tid 4): Payload is JSON, which is sent and
  received as one chunk. The payload is a JSON representation of
  ExampleStructA.
- Topic EXAMPLE_JSON_ARRAY (tid 5): Payload is JSON, which is sent and
  received as multiple chunks. The payload is an array of JSON
  object representations of ExampleStructA.

#### JSON Library

The [JSON library](https://github.com/RealTimeLogic/JSON) used by the
examples provides streaming-friendly encoding and decoding features for
resource-constrained embedded devices. Read the
[JSON Tutorial](https://realtimelogic.com/ba/doc/en/C/reference/html/md_en_C_md_JSON.html)
prior to looking at the source code.

![Using JSON with Real Time IoT Communication](https://realtimelogic.com/GZ/images/json-iot.svg)

## 2: Light Bulb Example

The Light Bulb Example is the companion example for the tutorial
[Modern Approach to Embedding a Web Server in a Device](https://realtimelogic.com/articles/Modern-Approach-to-Embedding-a-Web-Server-in-a-Device).

Two identical examples are provided: [bulb.c](examples/bulb.c) and
[bulb.cpp](examples/bulb.cpp). You can compare the two examples, which
makes it easy to see the difference between the C and C++ API.

The light bulb connects to our
[public SMQ test broker](https://simplemq.com/). You also need the
companion
[JavaScript code](https://github.com/RealTimeLogic/LSP-Examples/tree/master/SMQ-examples/LightSwitch-And-LightBulb-App)
when testing this example.

Start the Light Bulb Example as follows:

``` shell
./bulb
```

The following screenshot shows how to compile and run the example.

![How to compile and run the IoT light bulb example](https://realtimelogic.com/blogmedia/ModernApproachEmbeddedWebServer/bulb-c-code.png)

When you have initially tested the C code and the JavaScript code with
the public test broker, change the URL in all code to your own private
broker; the broker you set up in [example 1](#1-introductory-smq-examples).


## 3: SMQ IoT Example

The IoT example shows how to design a complete SMQ-based IoT solution.
The following video shows how to use the IoT example:

[![SMQ IoT Example](https://img.youtube.com/vi/R8SjfdySPsM/mqdefault.jpg)](https://youtu.be/R8SjfdySPsM)

The IoT example `LED-SMQ.c` is a copy of the SMQ example from
[SharkSSL](https://github.com/RealTimeLogic/SharkSSL). The example
can use the secure SharkMQ library or the standard SMQ library. The
example uses a TLS connection when compiled with SharkMQ and a standard TCP/IP
connection when compiled with SMQ.

- [SMQ API Documentation](https://realtimelogic.com/ba/doc/en/C/reference/html/group__SMQClient.html)
- [SharkMQ API Documentation](https://realtimelogic.com/ba/doc/en/C/shark/structSharkMQ.html)

The standard SMQ stack includes a compatibility API that enables
programs using the SharkMQ API to be compiled with the standard SMQ
stack.

### Running the IoT Example

- **Linux:** See the Linux build instructions above, then start `./LED-SMQ` in
  a terminal window.
- **Windows:** Compile and run the example using the project files in
  [VcMake](VcMake/).
- **Embedded systems:** See the instructions below.

### Building for Embedded Systems

The example code LED-SMQ.c requires porting to your embedded board's
LED(s). See the tutorial
[Interfacing LED Demo Programs to Hardware](https://realtimelogic.com/ba/doc/en/C/shark/md_md_Examples.html#LedDemo)
for details. See [src/arch](src/arch/) for SMQ porting layer details.

#### Ready-to-Run Embedded Examples

- The
  [SharkSSL ESP32 IDE](https://realtimelogic.com/downloads/sharkssl/ESP32/)
  includes ready-to-use LED interface code for the ESP32.

- You can also
  [download the IoT example for Arduino](https://realtimelogic.com/downloads/SMQ/SMQ-Arduino.zip).


### IoT Solution Quickstart

Set up your own IoT solution as follows:

1. Download and compile the example code "as is". The example, when
   run, connects to the
   [online test broker](https://simplemq.com/m2m-led/).
2. Familiarize yourself with how the example works.
3. Follow the
   [Setting Up an Environmentally Friendly IoT Solution](https://makoserver.net/articles/Setting-up-a-Low-Cost-SMQ-IoT-Broker)
   tutorial for how to set up your own IoT solution.
4. Modify [examples/LED-SMQ.c](examples/LED-SMQ.c) and change the domain URL
   (`SMQ_DOMAIN`) to your own IoT server.
