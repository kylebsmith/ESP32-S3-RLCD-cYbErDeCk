#!/usr/bin/env bash
# Render every part and run the design audit. This is what CI runs.
set -euo pipefail
cd "$(dirname "$0")/.."

mkdir -p export/stl export/reports
SCAD=${SCAD:-openscad}
RUN=""
command -v xvfb-run >/dev/null 2>&1 && RUN="xvfb-run -a"

# A deleted parameter does not stop OpenSCAD: it substitutes undef, warns on
# stderr, and renders something small and wrong. That is how a 2 x 2 x 1 mm
# "acrylic window" once passed a 52-check audit. Treat the warning as fatal.
fail=0
# The part list, in ONE place. Adding a part is this line, and the
# dispatch in cad/cyberdeck.scad - nothing else.
PARTS="chassis backplate buttons cover assembly"
for part in $PARTS; do
    printf '  rendering %-12s ... ' "$part"
    log=$(mktemp)
    $RUN "$SCAD" -D "part=\"$part\"" -o "export/stl/$part.stl" cad/cyberdeck.scad 2>"$log" || true
    if grep -qiE "unknown variable|undefined operation|WARNING: Ignoring" "$log"; then
        printf 'FAIL\n'
        grep -iE "unknown variable|undefined operation|WARNING: Ignoring" "$log" | sed 's/^/      /' | sort -u
        fail=1
    else
        printf 'ok\n'
    fi
    rm -f "$log"
done
[ "$fail" -eq 0 ] || { echo; echo "undefined identifiers in the model - refusing to continue"; exit 1; }
rm -f export/stl/assembly.stl

# The carry case is a separate model and was, until C-41, exported by hand -
# which is how a superseded one-piece STL sat in the tree with nothing pointing
# at it. It builds here now, under the same undef-is-fatal guard, which matters
# more for this file than for any other: C-41 records a parameter that OpenSCAD
# left undef while every Python tool read it as 15.00 mm.
mkdir -p export/stl/case
for part in front back; do
    printf '  rendering %-12s ... ' "case-$part"
    log=$(mktemp)
    $RUN "$SCAD" -D "part=\"$part\"" -o "export/stl/case/carrycase-$part.stl" \
        cad/carrycase.scad 2>"$log" || true
    if grep -qiE "unknown variable|undefined operation|WARNING: Ignoring" "$log"; then
        printf 'FAIL\n'
        grep -iE "unknown variable|undefined operation|WARNING: Ignoring" "$log" | sed 's/^/      /' | sort -u
        fail=1
    else
        printf 'ok\n'
    fi
    rm -f "$log"
done
[ "$fail" -eq 0 ] || { echo; echo "undefined identifiers in the carry case - refusing to continue"; exit 1; }

# Unit tests for the geometry primitives, BEFORE the part-level gate. A broken
# primitive produces parts that pass every dimensional check while being wrong
# in a way only the eye catches - see the header of tools/test_primitives.py.
echo
$RUN python3 tools/test_primitives.py

echo
$RUN python3 tools/validate.py --json export/reports/validation.json

echo
$RUN python3 tools/check_case.py

# The reference comparisons need a clone of a third-party repository. When it
# is absent they are skipped rather than failing the gate: they audit this
# design against someone else's, which is valuable but not a precondition for
# the design being internally sound.
REFDIR=${SOLAR_TERM:-/tmp/solar_term}
echo
if [ -d "$REFDIR" ]; then
    $RUN python3 tools/audit_reference.py --reference "$REFDIR"
else
    echo "  no reference clone at $REFDIR - skipping the component-facing audit"
    echo "  (git clone --depth 1 https://github.com/nilseuropa/solar_term $REFDIR)"
fi

echo
mkdir -p export/drawings
$RUN python3 tools/drawing.py

echo
REF=""
[ -d "$REFDIR" ] && REF="--reference $REFDIR"
$RUN python3 tools/structure.py --stations 90 --plot $REF
