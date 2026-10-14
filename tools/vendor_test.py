#!/usr/bin/env python3
"""Exercises the aitum-multistream obs-websocket vendor against a running OBS.

Usage: python3 tools/vendor_test.py [--password PW] [--port 4455] [--output NAME]

Needs the `websockets` package, an OBS with obs-websocket enabled, and at least
one output configured in the Multistream dock. It starts and stops real streams
against whatever those outputs point at, so point them somewhere harmless -- a
local RTMP listener such as

    ffmpeg -f flv -listen 1 -i rtmp://127.0.0.1:1936/live -f null -

works and keeps everything on the machine.

Every step prints PASS / FAIL / SKIP together with the actual response, so a
failure shows what came back rather than just that something was wrong.
"""
import argparse
import asyncio
import base64
import hashlib
import json
import sys

import websockets

VENDOR = "aitum-multistream"


class Obs:
    def __init__(self, ws):
        self.ws = ws
        self._id = 0
        self.events = []

    async def _recv_until(self, op):
        """Return the next message with the given op, banking events on the way."""
        while True:
            msg = json.loads(await self.ws.recv())
            if msg["op"] == 5:
                self.events.append(msg["d"])
                continue
            if msg["op"] == op:
                return msg

    async def request(self, request_type, data=None):
        self._id += 1
        rid = str(self._id)
        await self.ws.send(json.dumps({
            "op": 6,
            "d": {"requestType": request_type, "requestId": rid, "requestData": data or {}},
        }))
        while True:
            msg = await self._recv_until(7)
            if msg["d"]["requestId"] == rid:
                return msg["d"]

    async def vendor(self, request_type, data=None):
        """CallVendorRequest, unwrapped to the vendor's own responseData."""
        r = await self.request("CallVendorRequest", {
            "vendorName": VENDOR, "requestType": request_type, "requestData": data or {},
        })
        status = r["requestStatus"]
        if not status["result"]:
            return {"__transport_error": status.get("comment"), "__code": status.get("code")}
        return r.get("responseData", {}).get("responseData", {})

    async def pump(self, seconds):
        """Read from the socket for a while so async events are actually banked.

        Without this, events that arrive while the test is merely sleeping stay
        unread in the socket buffer and look like they were never sent.
        """
        loop = asyncio.get_event_loop()
        end = loop.time() + seconds
        while True:
            remaining = end - loop.time()
            if remaining <= 0:
                return
            try:
                msg = json.loads(await asyncio.wait_for(self.ws.recv(), timeout=remaining))
            except asyncio.TimeoutError:
                return
            if msg["op"] == 5:
                self.events.append(msg["d"])

    def drain_vendor_events(self):
        out = []
        for e in self.events:
            if e.get("eventType") == "VendorEvent" and e["eventData"].get("vendorName") == VENDOR:
                out.append(e["eventData"])
        self.events = []
        return out


async def connect(port, password):
    ws = await websockets.connect(f"ws://127.0.0.1:{port}", max_size=None)
    hello = json.loads(await ws.recv())
    # General | Outputs | Vendors. Vendors is bit 9 -- bit 16 is InputVolumeMeters.
    d = {"rpcVersion": hello["d"]["rpcVersion"],
         "eventSubscriptions": (1 << 0) | (1 << 6) | (1 << 9)}
    auth = hello["d"].get("authentication")
    if auth:
        if not password:
            raise SystemExit("obs-websocket requires a password; pass --password")
        secret = base64.b64encode(hashlib.sha256((password + auth["salt"]).encode()).digest())
        d["authentication"] = base64.b64encode(
            hashlib.sha256(secret + auth["challenge"].encode()).digest()).decode()
    await ws.send(json.dumps({"op": 1, "d": d}))
    obs = Obs(ws)
    await obs._recv_until(2)
    return obs


RESULTS = []


def check(name, ok, detail):
    RESULTS.append((name, ok))
    tag = "PASS" if ok is True else ("SKIP" if ok is None else "FAIL")
    print(f"[{tag}] {name}\n       {detail}", flush=True)


async def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--password", default="")
    ap.add_argument("--port", type=int, default=4455)
    ap.add_argument("--output", default=None, help="name of a main-canvas output to exercise")
    ap.add_argument("--vertical-output", help="optional Vertical Canvas output to exercise via the same requests")
    ap.add_argument("--stress-vertical-start", action="store_true",
                    help="retry vertical start during main-stream teardown; may crash OBS (isolated testing only)")
    args = ap.parse_args()
    if args.stress_vertical_start and not args.vertical_output:
        ap.error("--stress-vertical-start requires --vertical-output")

    obs = await connect(args.port, args.password)
    print("connected to obs-websocket\n")

    # --- vendor is registered at all -------------------------------------
    st = await obs.vendor("version")
    check("vendor 'aitum-multistream' is registered and answers 'version'",
          st.get("success") is True and bool(st.get("version")), json.dumps(st))
    if st.get("__transport_error"):
        print("\nThe vendor did not respond -- is the plugin loaded?")
        return 1

    # --- get_outputs, and the stream-key check ---------------------------
    go = await obs.vendor("get_outputs")
    all_outputs = go.get("outputs", [])
    check("get_outputs lists both canvases with upstream's vertical discriminator",
          go.get("success") is True
          and all(type(o.get("vertical")) is bool for o in all_outputs)
          and "vertical_outputs" not in go, json.dumps(go)[:400])
    outs = [o for o in all_outputs if o.get("vertical") is False]

    blob = json.dumps(go)
    leaked = [k for k in ("stream_key", "key", "bearer_token") if f'"{k}"' in blob]
    check("no stream key field in the get_outputs response", not leaked,
          f"fields present: {leaked}" if leaked else "none of stream_key/key/bearer_token present")

    servers = [o.get("stream_server", "") for o in outs]
    trimmed = all(s.count("/") <= 2 for s in servers)
    check("stream_server is trimmed to scheme://host", trimmed, f"servers={servers}")

    name = args.output or (outs[0]["name"] if outs else None)
    if not name:
        print("\nNo outputs configured -- add one in the dock to run the rest.")
        return 1
    target = next((o for o in outs if o["name"] == name), None)
    print(f"\nexercising output {name!r}: {json.dumps(target)}\n")

    # --- unknown name ----------------------------------------------------
    r = await obs.vendor("start_output", {"name": "__no_such_output__"})
    check("start_output preserves upstream's unknown-name error",
          r.get("success") is False and r.get("error") == "Output not found", json.dumps(r))

    r = await obs.vendor("stop_output", {"name": "__no_such_output__"})
    check("stop_output preserves upstream's unknown-name error",
          r.get("success") is False and r.get("error") == "Output not found", json.dumps(r))

    for action in ("start_output", "stop_output"):
        r = await obs.vendor(action)
        check(f"{action} preserves upstream's missing-name error",
              r.get("success") is False and r.get("error") == "Missing name", json.dumps(r))

    # --- main stream down ------------------------------------------------
    stream = (await obs.request("GetStreamStatus"))["responseData"]
    if stream["outputActive"]:
        await obs.request("StopStream")
        await asyncio.sleep(2)
    r = await obs.vendor("start_output", {"name": name})
    if target and target.get("requires_main"):
        check("start_output with the main stream down reports main_output_not_active",
              r.get("success") is False and r.get("error") == "main_output_not_active", json.dumps(r))
    else:
        check("start_output with the main stream down (output does not require main)",
              None, json.dumps(r))
        r2 = await obs.vendor("stop_output", {"name": name})
        print(f"       (stopped again: {json.dumps(r2)})")

    # --- with the main stream up -----------------------------------------
    ss = await obs.request("StartStream")
    if not ss["requestStatus"]["result"]:
        check("StartStream succeeded (needed for the remaining steps)", False,
              json.dumps(ss["requestStatus"]))
        return 1
    await asyncio.sleep(4)
    obs.drain_vendor_events()

    r = await obs.vendor("start_output", {"name": name})
    check("start_output with the main stream live succeeds",
          r.get("success") is True, json.dumps(r))

    # --- the double-start guard, hit immediately while connecting --------
    r2 = await obs.vendor("start_output", {"name": name})
    check("an immediate second start_output is an idempotent success",
          r2.get("success") is True, json.dumps(r2))

    await obs.pump(6)
    go2 = await obs.vendor("get_outputs")
    still = next((o for o in go2.get("outputs", [])
                  if o["name"] == name and o.get("vertical") is False), {})
    check("the output survived the second start and is active",
          still.get("active") is True, json.dumps(still))

    # --- vendor events ----------------------------------------------------
    evs = obs.drain_vendor_events()
    event_name = f"aitum_multi_output_{name}"
    started = [e for e in evs
               if e.get("eventType") == "output_started"
               and e.get("eventData") == {"name": event_name}]
    check("exactly one upstream output_started event was emitted", len(started) == 1,
          json.dumps(evs)[:300] if evs else "no vendor events received")

    # --- stop -------------------------------------------------------------
    r = await obs.vendor("stop_output", {"name": name})
    check("stop_output succeeds", r.get("success") is True, json.dumps(r))
    await obs.pump(6)
    evs = obs.drain_vendor_events()
    stopped = [e for e in evs
               if e.get("eventType") == "output_stopped"
               and e.get("eventData") == {"name": event_name}]
    check("exactly one upstream output_stopped event was emitted", len(stopped) == 1,
          json.dumps(evs)[:300] if evs else "no vendor events received")

    r = await obs.vendor("stop_output", {"name": name})
    check("stop_output on an already-stopped output is an idempotent success",
          r.get("success") is True, json.dumps(r))

    # --- the output can be started again after a stop ---------------------
    r = await obs.vendor("start_output", {"name": name})
    check("the output can be started again (busy guard did not wedge)",
          r.get("success") is True, json.dumps(r))
    await asyncio.sleep(4)
    await obs.vendor("stop_output", {"name": name})
    await asyncio.sleep(2)
    await obs.request("StopStream")
    if not args.stress_vertical_start:
        await obs.pump(2)

    if args.vertical_output:
        vertical_name = args.vertical_output
        check("get_outputs includes the selected vertical output in the combined list",
              any(o["name"] == vertical_name and o.get("vertical") is True for o in all_outputs),
              json.dumps(all_outputs)[:600])
        r = await obs.vendor("start_output", {"name": vertical_name})
        check("start_output routes to Vertical Canvas", r.get("success") is True, json.dumps(r))
        if args.stress_vertical_start:
            r = await obs.vendor("start_output", {"name": vertical_name})
            check("vertical start retry during connection succeeds", r.get("success") is True, json.dumps(r))
        await obs.pump(6)
        state = await obs.vendor("get_outputs")
        active = next((o for o in state.get("outputs", [])
                       if o["name"] == vertical_name and o.get("vertical") is True), {})
        check("the vertical output is active", active.get("active") is True, json.dumps(active))
        r = await obs.vendor("start_output", {"name": vertical_name})
        check("starting an active vertical output is an idempotent success", r.get("success") is True, json.dumps(r))
        r = await obs.vendor("stop_output", {"name": vertical_name})
        check("stop_output routes to Vertical Canvas", r.get("success") is True, json.dumps(r))
        await obs.pump(6)
        state = await obs.vendor("get_outputs")
        stopped = next((o for o in state.get("outputs", [])
                        if o["name"] == vertical_name and o.get("vertical") is True), {})
        check("the vertical output stopped", stopped.get("active") is False, json.dumps(stopped))
        r = await obs.vendor("stop_output", {"name": vertical_name})
        check("repeated vertical stop is an idempotent success", r.get("success") is True, json.dumps(r))

    await obs.ws.close()
    print()
    failed = [n for n, ok in RESULTS if ok is False]
    print(f"{sum(1 for _, ok in RESULTS if ok is True)} passed, {len(failed)} failed, "
          f"{sum(1 for _, ok in RESULTS if ok is None)} skipped")
    for n in failed:
        print(f"  FAILED: {n}")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(asyncio.run(main()))
