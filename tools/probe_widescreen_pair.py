#!/usr/bin/env python3
"""Build Tekken 3's 4:3-versus-16:9 widescreen picture pair from the real product.

Each aspect runs in its own process (PSXPORT_PRESENT_SINK and the settings file are process-wide),
with its tracked .ini named through PSXPORT_SETTINGS. The witness is the product's own
render_width / native_width pair. Widescreen only: both settings pin fps60=0 and an active fps60 row
is a defect. Launches the product itself and kills only PIDs it captured.

    tools/probe_widescreen_pair.py --disc "$DISC" --frame 1200

`--disc` or $PSXPORT_DISC names the image; there is no default.
"""

from __future__ import annotations

import argparse
import json
import os
import pathlib
import re
import subprocess
import sys
import time

ROOT = pathlib.Path(__file__).resolve().parents[1]
FRAMEWORK = ROOT / "external" / "psxport"
SCRATCH = ROOT / "scratch" / "wproof"
DISC_ENV = "PSXPORT_DISC"

sys.path.insert(0, str(FRAMEWORK / "tools"))
sys.path.insert(0, str(FRAMEWORK / "tools" / "port"))

# Used only to size the sink; the evidence is the product's own render_width.
NATIVE_WIDTH = 384
SINK_HEIGHT = 720

LEG_SETTINGS = {"4x3": "config/aspect_4x3.ini", "16x9": "config/aspect_16x9.ini"}
ASPECT_VALUES = {"4x3": 0, "16x9": 1}

# Keys `Mods::load` accepts. It has no comment support (every line splits on the first `=`).
SETTINGS_KEYS = frozenset({
    "aspect", "ires", "ires_auto", "ssao", "light", "shadows",
    "shadow_strength", "fps60", "ssao_strength", "ssao_radius", "ssao_bias", "ssao_range",
    "light_dir", "light_ambient", "light_diffuse",
})

PRODUCT = re.compile(r"^.*/([A-Za-z0-9_.]*_port)$")
WIDE_LINE = re.compile(r"native picture: aspect=(\d+) wide_engine=(\d+) "
                       r"native_width=(\d+) render_width=(\d+)")
SHOT_LINE = re.compile(r"\[gpu_shot\] wrote (\S+) \((\d+)x(\d+)")
PRESENT_SHOT_LINE = re.compile(
    r"present_shot.*wrote (\S+) \((\d+)x(\d+).*non-black (\d+)/(\d+) \(([\d.]+)%\)")
# No leading `\b`: `_` is a word character, so it would miss `PSXPORT_FPS60`.
FPS60_ROW = re.compile(r"fps60", re.IGNORECASE)


def other_product_running() -> str | None:
    """A running product, matched on the executable, not anywhere in the command line."""
    for line in subprocess.run(["ps", "-eo", "pid,args"], capture_output=True, text=True,
                               check=False).stdout.splitlines()[1:]:
        _, _, args = line.strip().partition(" ")
        executable = args.split(" ", 1)[0]
        if PRODUCT.match(executable) and "tekken3_port" not in executable:
            return line.strip()
    return None


def sink_width(aspect: int) -> int:
    return NATIVE_WIDTH if aspect == 0 else NATIVE_WIDTH * 16 // 12


def parse_wide_lines(log_text: str) -> list[dict]:
    """Every `[wide] native picture:` line, in order; the first is pre-publication, not steady state."""
    found = []
    for number, line in enumerate(log_text.splitlines(), start=1):
        found_match = WIDE_LINE.search(line)
        if found_match:
            found.append({
                "log_line": number,
                "aspect": int(found_match.group(1)),
                "wide_engine": int(found_match.group(2)),
                "native_width": int(found_match.group(3)),
                "render_width": int(found_match.group(4)),
                "text": line.split("] ", 1)[-1],
            })
    return found


def classify_wide(lines: list[dict]) -> dict:
    """The last `[wide]` line is the steady state (announces print on change); widened = render > native."""
    if not lines:
        return {"widened": None,
                "reason": "the product printed NO [wide] native picture line at all, so it never "
                          "announced a picture and there is nothing to read"}
    last = lines[-1]
    return {
        "steady_state_log_line": last["log_line"],
        "native_width": last["native_width"],
        "render_width": last["render_width"],
        "widened": last["render_width"] > last["native_width"],
        "announced": len(lines),
    }


def parse_telemetry(text: str) -> dict:
    """Guest-execution counters from the debug endpoint's `guest` reply, verbatim."""
    counters = {key: int(value) for key, value in re.findall(r"(\w+)=(\d+)", text)}
    if not counters:
        raise ValueError(f"the guest reply carried no counters: {text.strip()!r}")
    return counters


def find_fps60_activity(log_text: str) -> list[dict]:
    """Every log line mentioning fps60, classified active or mention.

    `fps60` followed by 1/true or `(selected)`/`(on)`/`enabled` is on; `unknown key ...` (a comment
    the parser could not read) is never on.
    """
    selection = re.compile(r"fps60\b[^\n]*\((?:selected|on|enabled)\)", re.IGNORECASE)
    value = re.compile(r"fps60\b[^\n=]*=\s*([01]|true|false)\b", re.IGNORECASE)
    found = []
    for line in log_text.splitlines():
        if not FPS60_ROW.search(line):
            continue
        if "unknown key" in line:
            active = False
        elif selection.search(line):
            active = True
        else:
            found_value = value.search(line)
            active = bool(found_value and found_value.group(1).lower() in ("1", "true"))
        found.append({"active": active, "text": line.strip()})
    return found


def run_leg(leg: str, frame: int, port: int, binary: pathlib.Path, timeout: int,
            disc: str) -> dict:
    settings = ROOT / LEG_SETTINGS[leg]
    if not settings.is_file():
        raise SystemExit(f"REFUSED: tracked settings {settings} is missing; the product would run on "
                         f"whatever untracked ini sits beside it. NOTHING WAS MEASURED.")
    aspect = ASPECT_VALUES[leg]
    SCRATCH.mkdir(parents=True, exist_ok=True)
    log = SCRATCH / f"leg_{leg}.log"
    capture = SCRATCH / "shots" / f"{leg}-f{frame}.ppm"
    capture.parent.mkdir(parents=True, exist_ok=True)

    from launch_environment import agent_environment

    busy = other_product_running()
    if busy:
        raise SystemExit(f"REFUSED: another product is already running ({busy}); the machine has one "
                         f"product slot. NOTHING WAS MEASURED.")

    environment = agent_environment(os.environ, settings=settings)
    environment.update({
        "PSXPORT_PRESENT_SINK": f"{sink_width(aspect)}x{SINK_HEIGHT}",
        "PSXPORT_LOG_FILE": str(log),
        "PSXPORT_DISC": disc,
        "PSXPORT_DEBUG_SERVER": str(port),
        "PSXPORT_WATCHDOG": "900",
        "PSXPORT_PRESENT_SHOT_AT": str(frame),
        "SDL_AUDIODRIVER": "dummy",
    })
    print(f"[{leg}] settings={settings} aspect={aspect} sink={environment['PSXPORT_PRESENT_SINK']} "
          f"disc={disc}; target presented frame {frame}", flush=True)
    process = subprocess.Popen([str(binary)], cwd=ROOT, env=environment,
                               stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    result = {"leg": leg, "aspect": aspect, "settings": str(settings),
              "sink": environment["PSXPORT_PRESENT_SINK"], "pid": process.pid,
              "target_frame": frame, "capture": str(capture), "log": str(log),
              "reached_frame": None, "shot_reply": None, "shot_size": None,
              "telemetry": None, "exit": None, "note": ""}
    try:
        from dbgclient import LiveClient
        client = None
        deadline = time.time() + timeout
        while time.time() < deadline:
            if process.poll() is not None:
                result["note"] = (f"the product EXITED on its own (rc={process.returncode}) before "
                                  f"presented frame {frame}")
                break
            try:
                if client is None:
                    client = LiveClient(port=port, timeout=120.0)
                counters = client.frames()
                if counters["frame"] >= frame:
                    result["reached_frame"] = counters["frame"]
                    result["frames_split"] = counters
                    break
                time.sleep(2.0)
            except (OSError, RuntimeError):
                client = None
                time.sleep(2.0)
        else:
            result["note"] = f"NEVER reached presented frame {frame} within {timeout}s"
        if result["reached_frame"] is not None and client is not None:
            result["shot_reply"] = client.shot(str(capture)).strip()
            try:
                result["telemetry"] = parse_telemetry(client.send("guest"))
            except (OSError, RuntimeError, ValueError) as error:
                result["note"] = f"the guest telemetry could not be read: {error}"
            try:
                client.quit()
            except (OSError, RuntimeError):
                pass
    finally:
        try:
            process.wait(timeout=120)
        except subprocess.TimeoutExpired:
            process.terminate()  # this tool's own child, by the captured PID
            process.wait(timeout=60)
        result["exit"] = process.returncode
    log_text = log.read_text(encoding="utf-8", errors="replace") if log.is_file() else ""
    result["wide_lines"] = parse_wide_lines(log_text)
    result["width_verdict"] = classify_wide(result["wide_lines"])
    shots = SHOT_LINE.findall(log_text)
    if shots:
        result["shot_size"] = f"{shots[-1][1]}x{shots[-1][2]}"
    present_shots = PRESENT_SHOT_LINE.findall(log_text)
    if present_shots:
        path, w, h, lit, area, percent = present_shots[-1]
        result["product_present_shot"] = {"path": path, "size": f"{w}x{h}",
                                          "non_black": f"{lit}/{area} ({percent}%)"}
    result["fps60_mentions"] = find_fps60_activity(log_text)
    (SCRATCH / f"leg_{leg}.json").write_text(json.dumps(result, indent=2), encoding="utf-8")
    return result


def report_leg(result: dict, out) -> None:
    out(f"[{result['leg']}] pid={result['pid']} exit={result['exit']} "
        f"reached presented frame {result['reached_frame']} of {result['target_frame']} asked")
    verdict = result["width_verdict"]
    out(f"[{result['leg']}] announced [wide] lines: {verdict.get('announced', 0)}")
    for line in result["wide_lines"]:
        out(f"    log:{line['log_line']}  {line['text']}")
    out(f"[{result['leg']}] steady state (log line {verdict.get('steady_state_log_line')}): "
        f"render_width={verdict.get('render_width')} native_width={verdict.get('native_width')} "
        f"-> render_width > native_width is {verdict.get('widened')}")
    if "reason" in verdict:
        out(f"[{result['leg']}] NO WIDTH VERDICT: {verdict['reason']}")
    out(f"[{result['leg']}] guest-resolution readback: {result['shot_reply']} "
        f"({result['shot_size']})")
    if result.get("product_present_shot"):
        own = result["product_present_shot"]
        out(f"[{result['leg']}] product's own present shot: {own['path']} {own['size']} "
            f"non-black {own['non_black']}")
    if result["telemetry"]:
        keys = ("calls", "translated_blocks", "executed_blocks", "executed_instructions",
                "host_dispatches", "cache_hits", "cache_misses", "invalidations", "faults")
        out(f"[{result['leg']}] guest telemetry: "
            + " ".join(f"{key}={result['telemetry'].get(key)}" for key in keys))
    active = [f for f in result["fps60_mentions"] if f["active"]]
    out(f"[{result['leg']}] fps60: {len(active)} line(s) reporting an ACTIVE fps60 knob, "
        f"{len(result['fps60_mentions']) - len(active)} bare mention(s)")
    for line in result["fps60_mentions"]:
        out(f"    {'ACTIVE' if line['active'] else 'mention'}: {line['text']}")
    if result["note"]:
        out(f"[{result['leg']}] NOTE: {result['note']}")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--leg", choices=sorted(LEG_SETTINGS), help="run ONE aspect leg")
    parser.add_argument("--frame", type=int, default=1200, help="presented frame to capture at")
    parser.add_argument("--port", type=int, default=5961, help="debug endpoint port")
    parser.add_argument("--timeout", type=int, default=900, help="seconds to wait for the frame")
    parser.add_argument("--binary", default="build/agent/bin/tekken3_port")
    parser.add_argument("--disc", help=f"the user's disc image; defaults to ${DISC_ENV}")
    args = parser.parse_args()
    disc = args.disc or os.environ.get(DISC_ENV)
    if not disc:
        print(f"REFUSED: no disc image. Pass --disc PATH or set {DISC_ENV}. The user's image is "
              f"runtime input and is deliberately not baked into a tracked tool. NOTHING WAS "
              f"MEASURED.", flush=True)
        return 2
    if not pathlib.Path(disc).is_file():
        print(f"REFUSED: disc image {disc} is not a file. NOTHING WAS MEASURED.", flush=True)
        return 2
    binary = ROOT / args.binary
    if not binary.is_file():
        print(f"REFUSED: {binary} does not exist. NOTHING WAS MEASURED.", flush=True)
        return 2

    legs = [args.leg] if args.leg else ["4x3", "16x9"]
    results = []
    for index, leg in enumerate(legs):
        results.append(run_leg(leg, args.frame, args.port + index, binary, args.timeout, disc))
        report_leg(results[-1], print)
    if len(results) != 2:
        return 1
    missing = [r["leg"] for r in results if not pathlib.Path(r["capture"]).is_file()]
    if missing:
        print(f"REFUSED: no capture for the {', '.join(missing)} leg. The run that was supposed to write "
              f"it never reached the checkpoint or failed before the shot — read ITS log. NOTHING WAS "
              f"COMPARED.", flush=True)
        return 2
    import widescreen_pair
    return widescreen_pair.report(results[0]["capture"], results[1]["capture"])


if __name__ == "__main__":
    raise SystemExit(main())
