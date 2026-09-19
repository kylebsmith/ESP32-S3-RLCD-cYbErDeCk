#!/usr/bin/env bash
# Render every published view. Reproducible: the camera for each view is
# stated here, so a render in the repository can always be regenerated from
# source rather than being a screenshot somebody took once.
set -euo pipefail
cd "$(dirname "$0")/.."

mkdir -p docs/img
SCAD=${SCAD:-openscad}
RUN=""
command -v xvfb-run >/dev/null 2>&1 && RUN="xvfb-run -a"

# The body spans x +/-58.3, y +/-69.55, z -10.0..16.6, so it centres on
# (0, 0, 3.3). Every camera below orbits that point.
CX=0; CY=0; CZ=3.3
SZ=${SZ:-1600,1200}

# name                 rx  rz  dist  part
VIEWS=(
  "render-assembly-iso    58  25  430  assembly"
  "render-assembly-front   0   0  400  assembly"
  "render-exploded        62  28  520  exploded"
  "chassis-front           0   0  380  chassis"
  "chassis-iso            58  25  430  chassis"
  "chassis-rear          180   0  380  chassis"
  "backplate-iso          58  25  430  backplate"
  "backplate-rear        180   0  380  backplate"
)

for v in "${VIEWS[@]}"; do
    read -r name rx rz dist part <<<"$v"
    printf '  %-24s ' "$name"
    $RUN "$SCAD" -D "part=\"$part\"" \
         --camera="$CX,$CY,$CZ,$rx,0,$rz,$dist" \
         --imgsize="$SZ" --colorscheme=Tomorrow --projection=p \
         -o "docs/img/$name.png" cad/cyberdeck.scad 2>/dev/null
    printf 'ok\n'
done
