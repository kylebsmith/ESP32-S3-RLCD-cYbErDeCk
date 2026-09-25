#!/usr/bin/env python3
"""
check_docs.py - prove the documentation still says true things about itself.

WHY THIS EXISTS

C-23 and C-24 were cited by name in three source files for the whole life of
the button and keyboard-rib work, and neither record was ever written into
DATUMS.md. Nothing noticed, because nothing was looking. A citation to a
record that does not exist is worse than no citation: it reads as provenance
and carries none.

The same class of rot had also set into README.md, which advertised
"twenty-one recorded corrections, seven open items" against an actual 34 and 9.

These are not typos, they are the documentation quietly ceasing to be true.
This checks the four ways that happens:

  1. a C-/O-/D- record is cited somewhere but does not exist in DATUMS.md
  2. the C- sequence has a hole in it (a record was dropped, not renumbered)
  3. a relative link between documents points at a file that is not there
  4. README's stated counts disagree with what DATUMS.md actually contains

    python3 tools/check_docs.py

Exit status is non-zero when any of them fails.
"""

from __future__ import annotations

import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
DATUMS = os.path.join(ROOT, "docs", "DATUMS.md")

_ONES = ["", "one", "two", "three", "four", "five", "six", "seven", "eight",
         "nine", "ten", "eleven", "twelve", "thirteen", "fourteen", "fifteen",
         "sixteen", "seventeen", "eighteen", "nineteen"]
_TENS = {20: "twenty", 30: "thirty", 40: "forty", 50: "fifty", 60: "sixty"}


def _word(n):
    """Spell a small count. The table it replaces ran out at thirty-six and
    reported the miss as 'None', which reads as a tool bug rather than as the
    stale README it was actually looking at."""
    if n < 20:
        return _ONES[n]
    t, o = (n // 10) * 10, n % 10
    if t not in _TENS:
        return str(n)
    return _TENS[t] if o == 0 else f"{_TENS[t]}-{_ONES[o]}"


class _Words(dict):
    def get(self, n, default=None):
        return _word(n)


WORDS = _Words()

# "managed_components" is third-party code fetched by the IDF component
# manager. Its documentation links point into Espressif's own repository
# layout and cannot resolve here, and it is not ours to correct. The version
# is pinned by dependencies.lock, which IS committed.
SKIP_DIRS = {".git", "reference", "build", "stl", "__pycache__", ".github",
             "managed_components"}


def repo_files():
    for dirpath, dirnames, filenames in os.walk(ROOT):
        dirnames[:] = [d for d in dirnames if d not in SKIP_DIRS]
        for fn in filenames:
            if fn.endswith((".md", ".py", ".scad", ".sh")):
                yield os.path.join(dirpath, fn)


def main():
    if not os.path.exists(DATUMS):
        sys.exit("docs/DATUMS.md is missing")
    datums = open(DATUMS, encoding="utf-8").read()

    # What records actually exist, by their headings.
    defined = set(re.findall(r"^#{2,4} ([COD]-\d{2})", datums, re.M))
    # C-12 to C-19 is one combined record covering a range.
    for m in re.finditer(r"^#{2,4} C-(\d{2}) to C-(\d{2})\b", datums, re.M):
        for n in range(int(m.group(1)), int(m.group(2)) + 1):
            defined.add(f"C-{n:02d}")

    fails = []

    # 1. every cited record exists
    cited = {}
    for path in repo_files():
        rel = os.path.relpath(path, ROOT)
        try:
            text = open(path, encoding="utf-8").read()
        except (OSError, UnicodeDecodeError):
            continue
        for m in re.finditer(r"\b([COD]-\d{2})\b", text):
            cited.setdefault(m.group(1), set()).add(rel)
    for rec in sorted(set(cited) - defined):
        where = ", ".join(sorted(cited[rec])[:4])
        fails.append(f"{rec} is cited in {where} but has no record in DATUMS.md")

    # 2. no hole in the C- sequence
    cnums = sorted(int(c[2:]) for c in defined if c.startswith("C-"))
    if cnums:
        holes = [n for n in range(1, max(cnums) + 1) if n not in cnums]
        for n in holes:
            fails.append(f"C-{n:02d} is missing; the C- sequence runs 01..{max(cnums):02d}")

    # 3. relative links resolve
    for path in repo_files():
        if not path.endswith(".md"):
            continue
        rel = os.path.relpath(path, ROOT)
        text = open(path, encoding="utf-8").read()
        for m in re.finditer(r"\[[^\]]*\]\(([^)#\s]+)(#[^)\s]*)?\)", text):
            target = m.group(1)
            if target.startswith(("http://", "https://", "mailto:")):
                continue
            resolved = os.path.normpath(
                os.path.join(os.path.dirname(path), target))
            if not os.path.exists(resolved):
                fails.append(f"{rel}: link to '{target}' does not resolve")

    # 4. README's counts match reality
    n_c = len([c for c in defined if c.startswith("C-")])
    n_o = len(re.findall(r"^#{2,4} O-\d{2}", datums, re.M))
    readme_path = os.path.join(ROOT, "README.md")
    if os.path.exists(readme_path):
        readme = open(readme_path, encoding="utf-8").read()
        m = re.search(r"([a-z-]+) recorded corrections, ([a-z-]+) open items", readme)
        if m:
            want_c, want_o = WORDS.get(n_c), WORDS.get(n_o)
            if m.group(1) != want_c:
                fails.append(f"README says '{m.group(1)} recorded corrections'; "
                             f"DATUMS.md has {n_c} ({want_c})")
            if m.group(2) != want_o:
                fails.append(f"README says '{m.group(2)} open items'; "
                             f"DATUMS.md has {n_o} ({want_o})")
        else:
            fails.append("README no longer states its correction/open-item counts")

    # 5. README's advertised check count matches the last validation run
    vj = os.path.join(ROOT, "export", "reports", "validation.json")
    if os.path.exists(readme_path) and os.path.exists(vj):
        import json
        try:
            total = json.load(open(vj))["total"]
        except (ValueError, KeyError):
            total = None
        if total:
            for m in re.finditer(r"(\d+)-check design audit", readme):
                if int(m.group(1)) != total:
                    fails.append(f"README advertises a {m.group(1)}-check audit; "
                                 f"validation.json reports {total}")

    print(f"-- DOCS --  {n_c} corrections, {n_o} open items, "
          f"{len(cited)} distinct records cited across the repo")
    for f in fails:
        print(f"  [FAIL] {f}")
    if fails:
        print(f"\n  {len(fails)} documentation defect(s).")
        return 1
    print("  [PASS] every cited record exists, the sequence is whole, "
          "links resolve, counts agree")
    return 0


if __name__ == "__main__":
    sys.exit(main())
