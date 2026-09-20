#!/usr/bin/env python3
"""
params.py - read cad/parameters.scad into a Python dict.

Shared by validate.py and drawing.py so the two can never disagree about what
the model says. Both tools read the SCAD source rather than carrying their own
copy of any dimension; a tool that hard-codes a number stops being a check.

Parsing is statement-based, not line-based. An earlier line-based version joined
any line ending in '=' onto the next, which silently ate every parameter
following a '// ======' section rule - and the checks depending on them then
died with a KeyError instead of reporting a result.
"""

from __future__ import annotations

import math
import os
import re

HERE = os.path.dirname(os.path.abspath(__file__))
PARAMS = os.path.join(os.path.dirname(HERE), "cad", "parameters.scad")

_ENV = {
    "sqrt": math.sqrt, "min": min, "max": max, "abs": abs, "pow": pow,
    "cos": lambda d: math.cos(math.radians(d)),
    "sin": lambda d: math.sin(math.radians(d)),
    "tan": lambda d: math.tan(math.radians(d)),
}

# OpenSCAD `cond ? a : b` -> Python `(a) if (cond) else (b)`. Only simple,
# non-nested ternaries are translated; anything hairier is left to fail and be
# supplied by a seed below.
_TERNARY = re.compile(r"^(?P<c>[^?]+)\?(?P<a>[^:]+):(?P<b>.+)$", re.S)


def _evaluate(expr, scope):
    # Collapse the statement onto one line FIRST. A SCAD expression may be
    # wrapped across lines for readability; Python only tolerates that inside
    # brackets, so `a + b\n    + c` was a SyntaxError, the value was silently
    # dropped, and the check that needed it died with a KeyError several
    # hundred lines away. Whitespace carries no meaning in either language.
    expr = " ".join(expr.split())
    m = _TERNARY.match(expr)
    if m:
        expr = f"({m.group('a')}) if ({m.group('c')}) else ({m.group('b')})"
    expr = expr.replace("&&", " and ").replace("||", " or ")
    v = eval(expr, {"__builtins__": {}}, scope)
    # A SCAD vector is a Python list already. float() cannot take one, so every
    # list-valued parameter used to raise here and be dropped WITHOUT A WORD -
    # the same silent-drop failure the line-joining bug had. Keep vectors as
    # lists of floats; everything else is still coerced, so a stray string or
    # bool is still rejected.
    if isinstance(v, (list, tuple)):
        return [float(q) for q in v]
    # STRINGS ARE VALUES TOO. parameters.scad opens with
    #     preset = "overbuilt";
    #     wall   = (preset == "compact") ? 2.4 : 3.2;
    # and float("overbuilt") raises, so `preset` was dropped, so `wall` was
    # dropped, and with it body_w, body_h, board_bay_cy and every value derived
    # from them. load_with_defaults() then seeded those four by hand AFTER
    # parsing, which hid the breakage for the tools that existed at the time and
    # silently dropped every NEW parameter that depended on them. Keep strings.
    if isinstance(v, str):
        return v
    if isinstance(v, bool):
        return v
    return float(v)


def load(path=PARAMS, seeds=None):
    """Return (values, provisional_names)."""
    src = open(path).read()

    provisional = set()
    for m in re.finditer(r"([A-Za-z_]\w*)\s*=[^;]*;([^\n]*)", src):
        if "[PROVISIONAL]" in m.group(2):
            provisional.add(m.group(1))

    code = re.sub(r"//[^\n]*", "", src)
    code = re.sub(r"/\*.*?\*/", "", code, flags=re.S)

    p = dict(seeds or {})
    statements = []
    for stmt in code.split(";"):
        m = re.match(r"\s*([A-Za-z_]\w*)\s*=\s*(.+)\s*$", stmt, flags=re.S)
        if m:
            statements.append((m.group(1), m.group(2).strip()))

    # Several passes: a value may depend on one declared later, or on a seed
    # that only becomes available once an unparseable ternary has been skipped.
    for _ in range(4):
        progressed = False
        for name, expr in statements:
            if name in p:
                continue
            try:
                p[name] = _evaluate(expr, dict(_ENV, **p))
                progressed = True
            except Exception:
                pass
        if not progressed:
            break

    return p, provisional


def load_with_defaults(path=PARAMS):
    """As load(), but guarantees the handful of values every tool needs."""
    p, prov = load(path)
    # A second pass with the first pass's results as seeds. Anything that could
    # not resolve the first time because its input came later, or came from a
    # fallback below, gets another chance with a fuller scope. Without this, a
    # seeded value is a dead end: nothing derived from it ever resolves.
    if p:
        p2, prov = load(path, seeds=p)
        p = p2
    p.setdefault("wall", 3.2)
    p.setdefault("spine", p["wall"])
    for name, expr in (("body_w", lambda q: q["wall"] + max(q["kbd_pocket_w"], q["board_pocket_w"]) + q["wall"]),
                       ("body_h", lambda q: q.get("bottom_wall", q["wall"]) + q["kbd_pocket_h"] + q["spine"] + q["board_pocket_h"] + q["wall"]),
                       ("body_t", lambda q: q["back_t"] + q["board_depth"] + q["front_t"])):
        if name not in p:
            p[name] = expr(p)
    p.setdefault("board_bay_cy", p["body_h"] / 2 - p["wall"] - p["board_pocket_h"] / 2)
    p.setdefault("kbd_bay_cy", -p["body_h"] / 2 + p.get("bottom_wall", p["wall"]) + p["kbd_pocket_h"] / 2)
    p.setdefault("z_front_inner", p["body_t"] - p["front_t"])
    p.setdefault("kbd_keeper", p["board_depth"] - p["kbd_depth"])
    p["_provisional"] = prov
    return p


if __name__ == "__main__":
    v = load_with_defaults()
    prov = v.pop("_provisional")
    for k in sorted(v):
        print(f"{k:<28} {v[k]}")
    print(f"\n{len(v)} values, {len(prov)} provisional: {sorted(prov)}")
