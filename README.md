# SMQ C Client Library

SMQ C client library for microcontrollers. The library includes porting layers for many RTOS environments, including bare metal.

SMQ, based on the publish - subscribe pattern, provides features similar to other pub/sub protocols such as MQTT. However, SMQ extends the pub/sub pattern with additional features such as one-to-one messaging and sender's address, features typically required in device management.

**[See the SMQ home page for details](https://realtimelogic.com/products/simplemq/)**

# License

The source code is released under the **Eclipse Public License - V 2.0**: [https://www.eclipse.org/legal/epl-v20.html](https://www.eclipse.org/legal/epl-v20.html)

You may compile a Program licensed under the EPL without modification and commercially license the result in accordance with the [terms of the EPL](https://www.eclipse.org/legal/epl-2.0/faq.php).

This Source Code may also be made available under the following Secondary Licenses when the conditions for such availability set forth in the Eclipse Public License, v. 2.0 are satisfied: **GNU General Public License, version 2**.

# Documentation

* [SMQ documentation](https://realtimelogic.com/ba/doc/?url=SMQ.html)
* [SMQ C client library reference manual](https://realtimelogic.com/ba/doc/en/C/reference/html/group__SMQClient.html)

https://realtimelogic.com/ba/doc/en/C/reference/html/group__SMQClient.html

# Compiling

The build includes the SMQ library and a LED demonstration program. The LED demonstration program connects to the [online public test broker](https://realtimelogic.com/IoT-LED-Cluster.html) . Initially, you may want to test the LED example by compiling and running the example code using the [ready to use online C compiler setup](https://repl.it/@RTL/SMQ-LED-Demo) .

## Build instructions for Windows, Linux, and Mac

The VcMake directory contains Visual Studio build files. The included Makefile compiles the library and the example program on any Linux/Mac computer.

## Build instructions for embedded systems

The example code m2m-led.c requires porting to your embedded board's LED(s). See the [Interfacing LED Demo Programs to Hardware](https://realtimelogic.com/ba/doc/en/C/shark/md_md_Examples.html#LedDemo) tutorial for details.

See the src/arch directory for cross compiling details.
