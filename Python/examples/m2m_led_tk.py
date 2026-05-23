"""Tkinter client for the SimpleMQ M2M LED demo.

This example mirrors the browser client from:

    https://simplemq.com/m2m-led/

It is a display/control client, not a simulated device. Run a compatible LED
device or simulator, then start this UI. The default URL uses Real Time Logic's
public broker; pass a local broker URL on the command line to override it.

    python examples/m2m_led_tk.py
    python examples/m2m_led_tk.py http://localhost/smq.lsp
"""

from __future__ import annotations

import queue
import sys
import threading
import tkinter as tk
import uuid
from dataclasses import dataclass, field
from pathlib import Path
from tkinter import ttk
from typing import Any, Dict, List, Optional

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from smqclient import SMQClient


DEFAULT_URL = "https://simplemq.com/smq.lsp"
DEVICE_TOPIC = "/m2m/led/device"
DISPLAY_TOPIC = "/m2m/led/display"
TEMP_TOPIC = "/m2m/temp"


@dataclass
class LedWidget:
    led_id: int
    name: str
    color: str
    var: tk.BooleanVar
    canvas: tk.Canvas
    item: int
    switch: ttk.Checkbutton


@dataclass
class DeviceView:
    ptid: int
    devname: str
    ipaddr: str
    frame: ttk.Frame
    temp_var: tk.StringVar
    leds: Dict[int, LedWidget] = field(default_factory=dict)


class M2MLedApp:
    def __init__(self, root: tk.Tk, url: str):
        self.root = root
        self.url = url
        self.events: "queue.Queue[tuple[str, tuple[Any, ...]]]" = queue.Queue()
        self.devices: Dict[int, DeviceView] = {}
        self.setup_lock = threading.Lock()
        self.pending_setup_acks = 0

        self.root.title("SimpleMQ M2M LED Demo")
        self.root.geometry("760x520")
        self.root.minsize(620, 420)

        self.status_var = tk.StringVar(value="Connecting...")
        self.detail_var = tk.StringVar(value=self.url)

        self._build_ui()
        self._connect()
        self.root.protocol("WM_DELETE_WINDOW", self.close)
        self.root.after(50, self._drain_events)

    def _build_ui(self) -> None:
        top = ttk.Frame(self.root, padding=(12, 10))
        top.pack(fill=tk.X)

        ttk.Label(top, text="SimpleMQ LED Control", font=("", 16, "bold")).pack(side=tk.LEFT)
        self.status_label = ttk.Label(top, textvariable=self.status_var)
        self.status_label.pack(side=tk.RIGHT)

        detail = ttk.Frame(self.root, padding=(12, 0, 12, 8))
        detail.pack(fill=tk.X)
        ttk.Label(detail, textvariable=self.detail_var).pack(side=tk.LEFT)
        ttk.Button(detail, text="Rediscover", command=self.rediscover).pack(side=tk.RIGHT)

        body = ttk.Frame(self.root, padding=(12, 0, 12, 12))
        body.pack(fill=tk.BOTH, expand=True)

        self.notebook = ttk.Notebook(body)
        self.notebook.pack(fill=tk.BOTH, expand=True)

        self.empty_frame = ttk.Frame(self.notebook, padding=24)
        self.notebook.add(self.empty_frame, text="Devices")
        ttk.Label(
            self.empty_frame,
            text="No devices connected",
            font=("", 14, "bold"),
        ).pack(anchor=tk.W)
        ttk.Label(
            self.empty_frame,
            text=(
                "Start an SMQ LED device or simulator, then use Rediscover. "
                "Devices publish their capabilities to this display client."
            ),
            wraplength=560,
            justify=tk.LEFT,
        ).pack(anchor=tk.W, pady=(8, 0))

    def _connect(self) -> None:
        self.smq = SMQClient.create(
            self.url,
            {
                "uid": "py-led-ui-" + uuid.uuid4().hex[:16],
                "info": "Python Tkinter M2M LED UI",
                "timeout": 8,
                "ping": 30,
                "pong": 8,
                "reconnect": True,
                "reconnect_delay": 3,
                "onconnect": self._on_connect,
                "onreconnect": self._on_reconnect,
                "onclose": self._on_close,
                "onmsg": self._on_unexpected_msg,
            },
        )

    def _on_connect(self, etid: int, rnd: int, ipaddr: str) -> None:
        self._post("connected", etid)
        self._setup_subscriptions()

    def _on_reconnect(self, etid: int, rnd: int, ipaddr: str) -> None:
        self._post("reconnected", etid)
        self._setup_subscriptions()

    def _on_close(self, reason: str, can_reconnect: bool) -> bool:
        self._post("closed", reason, can_reconnect)
        return can_reconnect

    def _on_unexpected_msg(self, data: bytes, ptid: int, tid: int, subtid: int) -> None:
        self._post("unexpected", len(data), ptid, tid, subtid)

    def _setup_subscriptions(self) -> None:
        with self.setup_lock:
            self.pending_setup_acks = 4

        def ack(accepted: bool, topic: Any, tid: int, subtopic: Any = None, subtid: Any = None) -> None:
            self._post("setup_ack", accepted, topic, subtopic)

        self.smq.subscribe(DEVICE_TOPIC, "devinfo", {"datatype": "json", "onmsg": self._on_devinfo, "onack": ack})
        self.smq.subscribe("self", "devinfo", {"datatype": "json", "onmsg": self._on_devinfo, "onack": ack})
        self.smq.subscribe(DEVICE_TOPIC, "led", {"onmsg": self._on_led, "onack": ack})
        self.smq.subscribe(TEMP_TOPIC, {"onmsg": self._on_temp, "onack": ack})

    def _on_devinfo(self, info: Dict[str, Any], ptid: int, tid: int, subtid: int) -> None:
        self._post("devinfo", info, ptid)

    def _on_led(self, data: bytes, ptid: int, tid: int, subtid: int) -> None:
        if len(data) >= 2:
            self._post("led", ptid, data[0], bool(data[1]))

    def _on_temp(self, data: bytes, ptid: int, tid: int, subtid: int) -> None:
        if len(data) >= 2:
            temp = int.from_bytes(data[:2], byteorder="big", signed=True)
            self._post("temp", ptid, temp)

    def _on_observe_change(self, subscribers: int, ptid: int) -> None:
        if subscribers == 0:
            self._post("remove_device", ptid)

    def _post(self, name: str, *args: Any) -> None:
        self.events.put((name, args))

    def _drain_events(self) -> None:
        while True:
            try:
                name, args = self.events.get_nowait()
            except queue.Empty:
                break
            self._handle_event(name, *args)
        self.root.after(50, self._drain_events)

    def _handle_event(self, name: str, *args: Any) -> None:
        if name == "connected":
            self.status_var.set(f"Connected as ETID {args[0]}")
            self.detail_var.set("Subscribing and discovering devices...")
            self._clear_devices()
        elif name == "reconnected":
            self.status_var.set(f"Reconnected as ETID {args[0]}")
            self.detail_var.set("Re-subscribing and discovering devices...")
            self._clear_devices()
        elif name == "closed":
            reason, can_reconnect = args
            self.status_var.set("Disconnected")
            suffix = "reconnecting" if can_reconnect else "closed"
            self.detail_var.set(f"{reason}; {suffix}")
            self._clear_devices()
        elif name == "setup_ack":
            accepted, topic, subtopic = args
            if not accepted:
                self.detail_var.set(f"Subscription denied: {topic} {subtopic or ''}")
                return
            with self.setup_lock:
                self.pending_setup_acks -= 1
                pending = self.pending_setup_acks
            if pending == 0:
                self.rediscover()
        elif name == "devinfo":
            self._add_or_update_device(args[0], args[1])
        elif name == "led":
            self._set_led(args[0], args[1], args[2])
        elif name == "temp":
            self._set_temp(args[0], args[1])
        elif name == "remove_device":
            self._remove_device(args[0])
        elif name == "unexpected":
            size, ptid, tid, subtid = args
            self.detail_var.set(f"Unexpected message: {size} bytes from {ptid}, tid={tid}, subtid={subtid}")

    def rediscover(self) -> None:
        if self.smq.publish("Hello", DISPLAY_TOPIC):
            self.detail_var.set("Discovery request sent to /m2m/led/display")
        else:
            self.detail_var.set("Discovery request not sent: client is not connected")

    def _add_or_update_device(self, info: Dict[str, Any], ptid: int) -> None:
        leds = info.get("leds")
        if not isinstance(leds, list):
            self.detail_var.set(f"Ignoring device {ptid}: missing LED capability list")
            return

        existing = self.devices.get(ptid)
        if existing is not None:
            self.notebook.forget(existing.frame)
            self.devices.pop(ptid, None)

        devname = str(info.get("devname", f"Device {ptid}"))
        ipaddr = str(info.get("ipaddr", ptid))
        frame = ttk.Frame(self.notebook, padding=14)
        temp_var = tk.StringVar()
        device = DeviceView(ptid=ptid, devname=devname, ipaddr=ipaddr, frame=frame, temp_var=temp_var)

        ttk.Label(frame, text=devname, font=("", 14, "bold"), wraplength=620).grid(row=0, column=0, columnspan=3, sticky=tk.W)
        ttk.Label(frame, text=f"ETID: {ptid}    IP: {ipaddr}").grid(row=1, column=0, columnspan=3, sticky=tk.W, pady=(4, 10))

        if "temp" in info:
            temp_var.set(self._format_temp(info["temp"]))
            ttk.Label(frame, textvariable=temp_var).grid(row=2, column=0, columnspan=3, sticky=tk.W, pady=(0, 10))
            start_row = 3
        else:
            temp_var.set("")
            start_row = 2

        ttk.Label(frame, text="LED").grid(row=start_row, column=0, sticky=tk.W, padx=(0, 16))
        ttk.Label(frame, text="State").grid(row=start_row, column=1, sticky=tk.W, padx=(0, 16))
        ttk.Label(frame, text="Control").grid(row=start_row, column=2, sticky=tk.W)

        for row_offset, led in enumerate(leds, start=1):
            self._add_led_row(device, led, start_row + row_offset)

        frame.columnconfigure(0, weight=1)
        self.devices[ptid] = device
        self.notebook.add(frame, text=ipaddr)
        self.notebook.select(frame)
        self.smq.observe(ptid, lambda subscribers, _topic, device_ptid=ptid: self._on_observe_change(subscribers, device_ptid))
        self.status_var.set(f"{len(self.devices)} device(s) connected")
        self.detail_var.set(f"Device discovered: {devname}")

    def _add_led_row(self, device: DeviceView, led: Dict[str, Any], row: int) -> None:
        led_id = int(led.get("id", row))
        name = str(led.get("name", f"LED {led_id}"))
        color = self._normalize_color(str(led.get("color", "green")))
        is_on = bool(led.get("on", False))

        ttk.Label(device.frame, text=name).grid(row=row, column=0, sticky=tk.W, pady=6, padx=(0, 16))

        canvas = tk.Canvas(device.frame, width=30, height=30, highlightthickness=0)
        item = canvas.create_oval(5, 5, 25, 25, fill=self._led_fill(color, is_on), outline="#333333")
        canvas.grid(row=row, column=1, sticky=tk.W, pady=6, padx=(0, 16))

        var = tk.BooleanVar(value=is_on)
        switch = ttk.Checkbutton(
            device.frame,
            text="On",
            variable=var,
            command=lambda ptid=device.ptid, current_led_id=led_id, current_var=var: self._send_led_command(ptid, current_led_id, current_var.get()),
        )
        switch.grid(row=row, column=2, sticky=tk.W, pady=6)

        device.leds[led_id] = LedWidget(
            led_id=led_id,
            name=name,
            color=color,
            var=var,
            canvas=canvas,
            item=item,
            switch=switch,
        )

    def _send_led_command(self, ptid: int, led_id: int, enabled: bool) -> None:
        if not 0 <= led_id <= 255:
            self.detail_var.set(f"Cannot send LED {led_id}: ID is outside one-byte payload range")
            return
        payload = bytes((led_id, 1 if enabled else 0))
        if self.smq.publish(payload, ptid):
            self.detail_var.set(f"Sent LED {led_id} {'on' if enabled else 'off'} to ETID {ptid}")
        else:
            self.detail_var.set("LED command not sent: client is not connected")

    def _set_led(self, ptid: int, led_id: int, enabled: bool) -> None:
        device = self.devices.get(ptid)
        if device is None:
            return
        led = device.leds.get(led_id)
        if led is None:
            return
        led.var.set(enabled)
        led.canvas.itemconfigure(led.item, fill=self._led_fill(led.color, enabled))
        self.detail_var.set(f"{device.devname}: {led.name} is {'on' if enabled else 'off'}")

    def _set_temp(self, ptid: int, temp_x10: int) -> None:
        device = self.devices.get(ptid)
        if device is None:
            return
        device.temp_var.set(self._format_temp(temp_x10))

    def _remove_device(self, ptid: int) -> None:
        device = self.devices.pop(ptid, None)
        if device is None:
            return
        self.notebook.forget(device.frame)
        device.frame.destroy()
        if self.devices:
            self.status_var.set(f"{len(self.devices)} device(s) connected")
            self.detail_var.set(f"Device disconnected: {device.devname}")
        else:
            self.status_var.set("Connected")
            self.detail_var.set("No devices connected")

    def _clear_devices(self) -> None:
        for device in list(self.devices.values()):
            self.notebook.forget(device.frame)
            device.frame.destroy()
        self.devices.clear()
        self.notebook.select(self.empty_frame)

    def close(self) -> None:
        try:
            self.smq.disconnect()
        finally:
            self.root.destroy()

    @staticmethod
    def _format_temp(temp_x10: Any) -> str:
        try:
            celsius = int(temp_x10) / 10.0
        except (TypeError, ValueError):
            return ""
        fahrenheit = round(celsius * 9 / 5 + 32)
        return f"Temperature: {celsius:.1f} C ({fahrenheit} F)"

    @staticmethod
    def _normalize_color(color: str) -> str:
        table = {
            "red": "#d92d20",
            "green": "#16a34a",
            "blue": "#2563eb",
            "yellow": "#eab308",
            "orange": "#f97316",
            "white": "#f8fafc",
        }
        return table.get(color.lower(), color if color.startswith("#") else "#16a34a")

    @staticmethod
    def _led_fill(color: str, enabled: bool) -> str:
        return color if enabled else "#3f3f46"


def main() -> None:
    url = sys.argv[1] if len(sys.argv) > 1 else DEFAULT_URL
    root = tk.Tk()
    M2MLedApp(root, url)
    root.mainloop()


if __name__ == "__main__":
    main()
