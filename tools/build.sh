#!/usr/bin/env bash
# Render every part and run the design audit. This is what CI runs.
set -euo pipefail
cd "$(dirname "$0")/.."

mkdir -p export/stl export/reports
SCAD=${SCAD:-openscad}
RUN=""
command -v xvfb-run >/dev/null 2>&1 && RUN="xvfb-run -a"

for part in chassis backplate buttons window; do
    printf '  rendering %-12s ... ' "$part"
    $RUN "$SCAD" -D "part=\"$part\"" -o "export/stl/$part.stl" cad/cyberdeck.scad 2>/dev/null
    printf 'ok\n'
done

echo
$RUN python3 tools/validate.py --json export/reports/validation.json
