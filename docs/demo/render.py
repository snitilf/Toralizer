#!/usr/bin/env python3
"""Render docs/demo/session.json into two looping SVGs, one per GitHub theme.

Every command and every output line on screen comes from session.json, which
record.py captures from a real run. This script adds the window, the prompt,
colour and pacing, nothing else. Standard library only, deterministic output.

usage: python3 docs/demo/render.py          write demo-light.svg and demo-dark.svg
       python3 docs/demo/render.py --text   print the same session as plain text
"""
import json
import re
import sys
from html import escape
from pathlib import Path

HERE = Path(__file__).resolve().parent

# canvas: a macOS terminal window with room around it for its shadow
W = 868
MX, MT = 24, 14            # transparent margin: sides, top
RADIUS = 12
BAR = 36                   # title bar height, holds the three window buttons only
PADX, PADT, PADB = 16, 12, 16
FONT = 13.5
CW = 8.1                   # cell width, 0.6 em, forced with textLength
LH = 20
BASE = 14.5                # baseline offset inside a row
COLS = int((W - 2 * MX - 2 * PADX) / CW)   # a longer output line wraps like in a terminal

PROMPT = "Toralizer % "    # zsh prompt in the repo directory, no user or host name
MONO = 'ui-monospace,SFMono-Regular,"SF Mono",Menlo,Consolas,"Liberation Mono",monospace'
LIGHTS = (("#ff5f57", "#e0443e"), ("#febc2e", "#d89e24"), ("#28c840", "#1aab29"))

# the window follows the reader's GitHub theme, as Terminal follows macOS appearance.
# accent is Tor's purple. depth is a shadow on the light page and a hairline on the dark one.
# mb is the bottom margin, tall on the light page to fit the shadow and short on the dark one
THEMES = {
    "light": dict(bg="#ffffff", bar="#f6f8fa", rule="#d1d9e0", edge=None,
                  fg="#1f2328", muted="#59636e", accent="#7d4698", mb=36),
    "dark": dict(bg="#151b23", bar="#212830", rule="#0d1117", edge="#3d444d",
                 fg="#f0f6fc", muted="#9198a1", accent="#c998e1", mb=14),
}

# pacing, in seconds
FADE_IN = 0.25
THINK = 0.55       # pause before the first keystroke
RECALL = 0.6       # a fresh prompt idles this long before the last command is recalled
TYPE = 0.018       # per character when a whole line is typed
EDIT = 0.055       # per character when a recalled command is edited
ERASE = 0.009      # per character deleted
ENTER = 0.25       # pause between the last keystroke and the first output
GAP_MIN, GAP_MAX = 0.22, 0.90   # real gaps between output lines are clamped to this
NEXT = 0.3         # the next prompt appears this long after the last output line
HOLD = 4.0         # the finished screen stays this long
FADE_OUT = 0.3
BLANK = 0.15


def split_edit(prev, new):
    """Return (head, old, mid, tail): prev == head+old+tail, new == head+mid+tail.

    The cut points fall on word boundaries, so whole words are replaced.
    """
    if not prev:
        return "", "", new, ""
    p, n = prev.split(" "), new.split(" ")
    a = 0
    while a < min(len(p), len(n)) and p[a] == n[a]:
        a += 1
    b = 0
    while b < min(len(p), len(n)) - a and p[-1 - b] == n[-1 - b]:
        b += 1
    head = " ".join(n[:a]) + (" " if a else "")
    old = " ".join(p[a:len(p) - b])
    mid = " ".join(n[a:len(n) - b])
    tail = " ".join(n[len(n) - b:]) if b else ""
    if tail and old:
        tail = " " + tail
    elif tail:
        mid += " "
    assert head + old + tail == prev, (prev, head, old, tail)
    assert head + mid + tail == new, (new, head, mid, tail)
    return head, old, mid, tail


def command_runs(text):
    """Colour a piece of a typed line. Returns (offset, text, colour, bold)."""
    runs, col = [], 0
    for word in text.split(" "):
        if word:
            is_tool = word == "./toralize"
            runs.append((col, word, "accent" if is_tool else "fg", is_tool))
        col += len(word) + 1
    return runs


def output_runs(text):
    """Colour an output line. Purple marks what toralize and Tor produced."""
    m = re.fullmatch(r'(\[toralize\]) (warning:.*)', text)
    if m:   # the wrapper's SIP warning: nothing went through Tor, so no accent
        return [(0, m.group(1), "muted", False), (11, m.group(2), "fg", False)]
    m = re.fullmatch(r'(\[toralize\]) (.*)', text)
    if m:
        return [(0, m.group(1), "accent", False), (11, m.group(2), "fg", False)]
    m = re.fullmatch(r'(\{"IsTor":)(true|false)(,"IP":")([^"]*)("\})', text)
    if m:
        value = "accent" if m.group(2) == "true" else "fg"
        runs, col = [], 0
        for i, part in enumerate(m.groups()):
            runs.append((col, part, value if i in (1, 3) else "muted", i in (1, 3)))
            col += len(part)
        return runs
    return [(0, text, "fg", False)]


def clip_runs(runs, a, b):
    """The part of a line's runs that falls in columns [a, b), shifted to start at 0."""
    out = []
    for off, s, colour, bold in runs:
        lo, hi = max(off, a), min(off + len(s), b)
        if lo < hi:
            out.append((lo - a, s[lo - off:hi - off], colour, bold))
    return out


def rows_of(text):
    """How many terminal rows an output line takes."""
    return max(1, -(-len(text.rstrip()) // COLS))


def build(session, theme):
    c = THEMES[theme]
    css, body, defs = [], [], []
    count = [0]

    def uid():
        count[0] += 1
        return count[0]

    def x(col):
        return MX + PADX + col * CW

    def top(row):
        return MT + BAR + PADT + row * LH

    def runs_svg(runs, col, row):
        out = []
        for off, s, colour, bold in runs:
            off += len(s) - len(s.lstrip(" "))
            s = s.strip(" ")
            if not s:
                continue
            out.append(
                f'<text x="{x(col + off):.2f}" y="{top(row) + BASE}" fill="{c[colour]}" textLength="{len(s) * CW:.2f}"'
                f'{" font-weight=" + chr(34) + "600" + chr(34) if bold else ""}>{escape(s)}</text>')
        return "".join(out)

    # pass 1: one entry per typed line, with its row and its times
    entries, t, row, prev = [], FADE_IN + 0.1, 0, ""
    for scene in session["scenes"]:
        text = scene["command"]
        parts = split_edit(prev, text)
        head, old, mid, tail = parts
        whole = not (old or tail or head)
        e = dict(parts=parts, row=row, lines=[], t_prompt=t)
        e["t_recall"] = t if whole else t + RECALL
        e["t_erase"] = e["t_recall"] + THINK
        e["t_erased"] = e["t_erase"] + len(old) * ERASE
        e["t_insert"] = e["t_erased"] + (0.12 if old else 0)
        e["t_done"] = e["t_insert"] + len(mid) * (TYPE if whole else EDIT)
        t = e["t_done"] + ENTER
        row += 1
        last = 0
        for line in scene["lines"]:
            t += min(max((line["t_ms"] - last) / 1000, GAP_MIN), GAP_MAX)
            last = line["t_ms"]
            e["lines"].append((row, line["text"], t))
            row += rows_of(line["text"])
        prev = text
        t += NEXT
        row += 1   # the next prompt
        entries.append(e)
    idle_row, t_idle = row, t
    t_end = t_idle + HOLD
    total = round(t_end + FADE_OUT + BLANK, 2)
    win_h = BAR + PADT + (idle_row + 1) * LH + PADB
    height = MT + win_h + c["mb"]

    def pct(sec):
        return f"{100 * sec / total:.3f}%"

    def shown(sec, until=None):
        """Animation name: hidden until `sec`, then visible (until `until`, if given)."""
        k = uid()
        end = f"{pct(until)}{{opacity:1}}{pct(until + 0.002)},100%{{opacity:0}}" if until else "100%{opacity:1}"
        css.append(f"@keyframes s{k}{{0%,{pct(sec)}{{opacity:0}}{pct(sec + 0.002)},{end}}}")
        return f"s{k}"

    # The base state of every element is the finished screen: what is only on screen for a
    # while (a recalled word, an edit cursor) carries opacity="0", and an animation takes over.
    # With animation off or unsupported the reader gets the final frame.
    def group(inner, sec, until=None, hidden=False):
        base = ' opacity="0"' if hidden else ""
        return f'<g class="a"{base} style="animation-name:{shown(sec, until)}">{inner}</g>'

    def cursor(col, r, sec, until, blink=False):
        bar = f'<rect x="{x(col) - 0.5:.2f}" y="{top(r) + 3}" width="2" height="{LH - 5}" fill="{c["fg"]}"{" class=" + chr(34) + "blink" + chr(34) if blink else ""}/>'
        return group(bar, sec, until, hidden=until is not None)

    # pass 2: emit
    p = len(PROMPT)
    for e in entries:
        head, old, mid, tail = e["parts"]
        r = e["row"]
        body.append(group(runs_svg([(0, PROMPT.rstrip(), "muted", False)], 0, r), e["t_prompt"]))
        if e["t_recall"] > e["t_prompt"]:
            body.append(cursor(p, r, e["t_prompt"], e["t_recall"]))
        c_edit = p + len(head)            # where the edited word starts
        c_tail = c_edit + len(mid)        # where the tail rests when the edit is done
        if head:
            body.append(group(runs_svg(command_runs(head), p, r), e["t_recall"]))
        if old:
            body.append(group(runs_svg(command_runs(old), c_edit, r), e["t_recall"], e["t_erased"], hidden=True))
        body.append(group(runs_svg(command_runs(mid), c_edit, r), e["t_erased"] if old else e["t_recall"]))

        # the tail slides in whole cells. it carries a strip of background that hides the
        # word under it, and the cursor on its left edge, so one transform does the edit
        k = uid()
        start = (len(old) - len(mid)) * CW
        frames = f"0%,{pct(e['t_erase'])}{{transform:translateX({start:.2f}px)"
        if old:
            frames += f";animation-timing-function:steps({len(old)},end)}}"
            frames += f"{pct(e['t_erased'])},{pct(e['t_insert'])}{{transform:translateX({-len(mid) * CW:.2f}px)"
        frames += f";animation-timing-function:steps({len(mid)},end)}}{pct(e['t_done'])},100%{{transform:translateX(0)}}"
        css.append(f"@keyframes e{k}{{{frames}}}")
        defs.append(f'<clipPath id="c{k}"><rect x="{x(c_edit) - 2:.2f}" y="{top(r)}" width="{W - MX - 2 - x(c_edit):.2f}" height="{LH}"/></clipPath>')
        cover = max(len(old), len(mid)) * CW + 2
        body.append(
            f'<g class="a" clip-path="url(#c{k})" style="animation-name:{shown(e["t_recall"])}">'
            f'<g class="a" style="animation-name:e{k}">'
            f'<rect x="{x(c_tail) - 1:.2f}" y="{top(r)}" width="{cover:.2f}" height="{LH}" fill="{c["bg"]}"/>'
            f'{runs_svg(command_runs(tail), c_tail, r)}{cursor(c_tail, r, e["t_recall"], e["t_done"] + ENTER)}</g></g>')

        for lrow, text, sec in e["lines"]:
            runs = output_runs(text)
            inner = "".join(runs_svg(clip_runs(runs, a, a + COLS), 0, lrow + a // COLS)
                            for a in range(0, len(text.rstrip()), COLS))
            body.append(group(inner, sec))

    # the session ends on an idle prompt
    body.append(group(runs_svg([(0, PROMPT.rstrip(), "muted", False)], 0, idle_row), t_idle))
    body.append(cursor(p, idle_row, t_idle, None, blink=True))

    css.insert(0, (
        "text{white-space:pre}"
        f".a{{animation-duration:{total}s;animation-timing-function:linear;animation-iteration-count:infinite}}"
        f"@keyframes loop{{0%{{opacity:0}}{pct(FADE_IN)},{pct(t_end)}{{opacity:1}}{pct(t_end + FADE_OUT)},100%{{opacity:0}}}}"
        f"#run{{animation:loop {total}s cubic-bezier(.16,1,.3,1) infinite}}"
        "@keyframes blink{0%,55%{opacity:1}55.01%,100%{opacity:0}}"
        ".blink{animation:blink 1.1s linear infinite}"
        "@media (prefers-reduced-motion:reduce){*{animation:none!important}}"))

    # the window: body, title bar with the three buttons and no title text
    wx, wy, ww = MX, MT, W - 2 * MX
    if c["edge"]:
        frame = (f'<rect x="{wx + 0.5}" y="{wy + 0.5}" width="{ww - 1}" height="{win_h - 1}" rx="{RADIUS - 0.5}" '
                 f'fill="{c["bg"]}" stroke="{c["edge"]}"/>')
        inset = 1
    else:
        defs.append('<filter id="shadow" x="-10%" y="-10%" width="120%" height="130%" color-interpolation-filters="sRGB">'
                    '<feDropShadow dx="0" dy="1" stdDeviation="1.2" flood-color="#1f2328" flood-opacity="0.16"/>'
                    '<feDropShadow dx="0" dy="9" stdDeviation="9" flood-color="#1f2328" flood-opacity="0.13"/></filter>')
        frame = f'<rect x="{wx}" y="{wy}" width="{ww}" height="{win_h}" rx="{RADIUS}" fill="{c["bg"]}" filter="url(#shadow)"/>'
        inset = 0
    r_in = RADIUS - inset
    bar = (f'<path d="M{wx + inset} {wy + BAR}V{wy + inset + r_in}a{r_in} {r_in} 0 0 1 {r_in}-{r_in}h{ww - 2 * inset - 2 * r_in}'
           f'a{r_in} {r_in} 0 0 1 {r_in} {r_in}V{wy + BAR}z" fill="{c["bar"]}"/>'
           f'<rect x="{wx + inset}" y="{wy + BAR - 1}" width="{ww - 2 * inset}" height="1" fill="{c["rule"]}"/>')
    lights = "".join(
        f'<circle cx="{wx + 20 + 20 * i}" cy="{wy + BAR / 2}" r="5.75" fill="{fill}" stroke="{ring}" stroke-width="0.5"/>'
        for i, (fill, ring) in enumerate(LIGHTS))

    label = ("Terminal session. Homebrew curl run directly reports IsTor false. "
             "The same command with ./toralize in front logs four [toralize] lines and reports IsTor true with a Tor exit address. "
             "./toralize with /usr/bin/curl prints a SIP warning and reports IsTor false again, because SIP strips DYLD_INSERT_LIBRARIES from system binaries.")
    svg = (
        f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {W} {height}" width="{W}" height="{height}" '
        f'font-family="{escape(MONO)}" font-size="{FONT}" role="img" aria-label="{escape(label)}">'
        f"<title>{escape(label)}</title><style>{''.join(css)}</style><defs>{''.join(defs)}</defs>"
        f'{frame}{bar}{lights}<g id="run">{"".join(body)}</g></svg>\n')
    return svg, total


def main():
    session = json.loads((HERE / "session.json").read_text())
    if "--text" in sys.argv[1:]:
        blocks = []
        for scene in session["scenes"]:
            lines = ["% " + scene["command"]] + [l["text"] for l in scene["lines"]]
            blocks.append("\n".join(lines))
        print("\n\n".join(blocks))
        return
    for scene in session["scenes"]:
        if len(PROMPT) + len(scene["command"]) > COLS:
            raise SystemExit(f"a command is wider than {COLS} columns: {scene['command']}")
    for theme in THEMES:
        svg, total = build(session, theme)
        out = HERE / f"demo-{theme}.svg"
        out.write_text(svg)
        print(f"{out.name}  {len(svg.encode())} bytes  loop {total}s")


if __name__ == "__main__":
    main()
