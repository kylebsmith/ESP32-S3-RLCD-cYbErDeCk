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
for part in chassis backplate buttons window assembly; do
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

echo
$RUN python3 tools/validate.py --json export/reports/validation.json
