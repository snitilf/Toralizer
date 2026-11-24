#!/usr/bin/env python3
"""Run the three demo commands for real and save the capture to docs/demo/session.json.

Run it from the repository root, with Tor already listening on 127.0.0.1:9050
(it does not start or stop Tor):

    make demo-record

The three runs are Homebrew curl on its own, Homebrew curl under ./toralize, and
/usr/bin/curl under ./toralize. Each argv list runs without a shell, with stderr
merged into stdout, and every output line is stamped with the milliseconds since
the process was spawned. The capture is checked, so a flaky circuit or a changed
wrapper exits 1 and writes nothing. The real address is masked in everything that
is written. Then render.py draws the SVGs. Standard library only.
"""
import datetime
import json
import os
import re
import shlex
import shutil
import socket
import subprocess
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
DEMO = ROOT / "docs" / "demo"
URL = "https://check.torproject.org/api/ip"
TIMEOUT = 90   # seconds per run
SIP_WARNING = ("[toralize] warning: /usr/bin/curl is protected by SIP, DYLD_INSERT_LIBRARIES "
               "is stripped and the connection will not be rerouted")


def die(message):
    print(f"record.py: {message}", file=sys.stderr)
    sys.exit(1)


def out(argv):
    """Stdout of a short command, or None when it cannot run."""
    try:
        done = subprocess.run(argv, capture_output=True, text=True, timeout=30)
    except (OSError, subprocess.SubprocessError):
        return None
    return done.stdout.strip() if done.returncode == 0 else None


def run(argv):
    """Run argv in the repository root. Returns (exit code, [(t_ms, line)])."""
    env = {k: v for k, v in os.environ.items()
           if k not in ("DYLD_INSERT_LIBRARIES", "TORALIZE_QUIET", "TORALIZE_PROXY_PORT")}
    start = time.monotonic()
    proc = subprocess.Popen(argv, cwd=ROOT, env=env, stdin=subprocess.DEVNULL,
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT, bufsize=0)
    lines, pending = [], b""

    def stamp():
        return int((time.monotonic() - start) * 1000)

    while True:
        if time.monotonic() - start > TIMEOUT:
            proc.kill()
            proc.wait()
            die(f"timed out after {TIMEOUT} s: {shlex.join(argv)}")
        chunk = os.read(proc.stdout.fileno(), 4096)
        if not chunk:
            break
        pending += chunk
        while b"\n" in pending:
            raw, pending = pending.split(b"\n", 1)
            lines.append((stamp(), raw))
    if pending:   # curl -s prints the JSON without a final newline
        lines.append((stamp(), pending))
    code = proc.wait()
    text = [(t, raw.decode("utf-8", "replace").rstrip("\r")) for t, raw in lines]
    return code, [(t, s) for t, s in text if s.strip()]


def parse_ip(lines):
    """The (IsTor, IP) of the one JSON line in a run, or None."""
    found = []
    for _, text in lines:
        try:
            doc = json.loads(text)
        except ValueError:
            continue
        if isinstance(doc, dict) and "IsTor" in doc and "IP" in doc:
            # render.py colours the line by its exact shape
            compact = json.dumps(doc, separators=(",", ":"))
            if text != compact or not re.fullmatch(r"\d+\.\d+\.\d+\.\d+", str(doc["IP"])):
                die("unexpected answer format")
            found.append((doc["IsTor"], doc["IP"]))
    return found[0] if len(found) == 1 else None


def main():
    brew = out(["brew", "--prefix", "curl"])
    curl = f"{brew}/bin/curl" if brew else None
    if not (ROOT / "toralize.dylib").is_file():
        die("toralize.dylib not found, run: make")
    if not (curl and os.access(curl, os.X_OK)):
        die("homebrew curl not found, run: brew install curl")
    try:
        socket.create_connection(("127.0.0.1", 9050), timeout=3).close()
    except OSError:
        die("tor is not listening on 127.0.0.1:9050, run: brew services start tor")

    argvs = [
        [curl, "-4", "-s", URL],
        ["./toralize", curl, "-4", "-s", URL],
        ["./toralize", "/usr/bin/curl", "-4", "-s", URL],
    ]
    runs = []
    for argv in argvs:
        print(f"running: {shlex.join(argv)}", file=sys.stderr)
        runs.append(run(argv))

    # the capture has to show what the README says it shows
    (c1, l1), (c2, l2), (c3, l3) = runs
    for number, code in enumerate((c1, c2, c3), 1):
        if code != 0:
            die(f"run {number} exited with {code}")
    r1, r2, r3 = parse_ip(l1), parse_ip(l2), parse_ip(l3)
    if not (r1 and r2 and r3):
        die("a run did not print exactly one JSON answer")
    if r1[0] is not False:
        die("run 1 is not direct: IsTor is not false")
    log2 = [s for _, s in l2 if s.startswith("[toralize]")]
    if len(log2) != 4 or len(l2) != 5:
        die(f"run 2 should print exactly 4 [toralize] lines and the answer, got: {[s for _, s in l2]}")
    if r2[0] is not True:
        die("run 2 did not go through Tor: IsTor is not true")
    if [s for _, s in l3 if s.startswith("[toralize]")] != [SIP_WARNING] or len(l3) != 2:
        die(f"run 3 should print the SIP warning and the answer, got: {[s for _, s in l3 if not s.startswith('{')]}")
    if r3 != r1:
        die("run 3 should be direct and show the same address as run 1")
    real = r1[1]
    if r2[1] == real:
        die("the Tor exit address equals the real address")

    # mask the real address everywhere, keep only its first octet
    masked = real.split(".")[0] + ".x.x.x"
    scenes = []
    for argv, (code, lines) in zip(argvs, runs):
        scenes.append({
            "command": shlex.join(argv),
            "exit_code": code,
            "lines": [{"t_ms": t, "text": s.replace(real, masked)} for t, s in lines],
        })

    tor = shutil.which("tor") or (out(["brew", "--prefix", "tor"]) or "") + "/bin/tor"
    tor_version = (out([tor, "--version"]) or "").split()
    curl_version = (out([curl, "--version"]) or "").split()
    session = {
        "captured": datetime.datetime.now(datetime.timezone.utc).date().isoformat(),
        "host": {
            "macos": out(["sw_vers", "-productVersion"]),
            "arch": out(["uname", "-m"]),
            "curl": curl_version[1] if len(curl_version) > 1 else None,
            "tor": tor_version[2].rstrip(".") if len(tor_version) > 2 else None,
        },
        "scenes": scenes,
    }
    if not all(session["host"].values()):
        die(f"could not read the host versions: {session['host']}")

    target = DEMO / "session.json"
    target.write_text(json.dumps(session, indent=2) + "\n")
    done = subprocess.run([sys.executable, str(DEMO / "render.py")], cwd=ROOT)

    # the real address must not be anywhere in what gets committed
    leaks = [p for p in [*DEMO.iterdir(), ROOT / "README.md"]
             if p.is_file() and real.encode() in p.read_bytes()]
    if leaks or done.returncode != 0:
        for name in ("session.json", "demo-light.svg", "demo-dark.svg"):
            (DEMO / name).unlink(missing_ok=True)
        die("the real address leaked, outputs deleted" if leaks else "render.py failed, outputs deleted")
    print(f"wrote {target.relative_to(ROOT)}", file=sys.stderr)


if __name__ == "__main__":
    main()
