#!/usr/bin/env bash
# Render every published view. Reproducible: the camera for each view is stated
# here, so a render in the repository can always be regenerated from source
# rather than being a screenshot somebody took once.
#
# Views are assembled from the EXPORTED STLs, not from the CSG source. See the
# header of cad/render_assembly.scad for why.
set -euo pipefail
cd "$(dirname "$0")/.."

mkdir -p docs/img export/stl
SCAD=${SCAD:-openscad}
RUN=""
command -v xvfb-run >/dev/null 2>&1 && RUN="xvfb-run -a"

T=$(mktemp -d)
trap 'rm -rf "$T"' EXIT

say() { printf '  %-24s ' "$1"; }

# --- the structural parts ---------------------------------------------------
for part in chassis backplate; do
    say "stl: $part"
    $RUN "$SCAD" -D "part=\"$part\"" -o "export/stl/$part.stl" \
         cad/cyberdeck.scad 2>/dev/null
    printf 'ok\n'
done

# --- the component mock-ups and the buttons, positioned in assembly space ----
# Written out here rather than kept as files: they are presentation scaffolding
# and have no meaning outside this script.
emit() {  # name, body
    cat > "$T/$1.scad" <<INNER
include <$PWD/cad/parameters.scad>
use <$PWD/cad/lib/util.scad>
use <$PWD/cad/lib/components.scad>
$2
INNER
    say "stl: $1"
    $RUN "$SCAD" -o "export/stl/$1.stl" "$T/$1.scad" 2>/dev/null
    printf 'ok\n'
}

emit mock-board \
  'translate([board_cx, board_cy, body_t - front_t - board_depth]) mock_board();'
# The screen is a separate body floated clear of the board: coincident or
# interpenetrating solids stripe in preview, and this one faces the camera.
emit mock-screen \
  'translate([board_cx + display_off_x, board_cy,
              body_t - front_t - board_depth + board_stack + 0.05])
       rbox(display_active_w, display_active_h, 0.4, 0.5);'
emit mock-keyboard \
  'translate([0, -body_h/2 + wall + kbd_pocket_h/2, body_t - front_t - kbd_depth])
       mock_keyboard();'
emit buttons-fitted \
  'for (i = [0 : button_count - 1])
       translate([board_cx + (i - (button_count - 1)/2) * button_pitch,
                  body_h/2 + 0.3,
                  body_t - front_t - board_w_display_front + button_w_centre])
           rotate([90, 0, 0]) rbox(button_cap_w, button_cap_h, wall + 0.6, 0.8);'

# --- views ------------------------------------------------------------------
# The body spans x +/-58.3, y +/-69.55, z -10.0..16.6, so it centres on
# (0, 0, 3.3). Every camera below orbits that point.
CX=0; CY=0; CZ=3.3
SZ=${SZ:-1600,1200}

shot() {  # name, rx, rz, dist, file, extra-D
    say "$1"
    $RUN "$SCAD" ${6:-} --camera="$CX,$CY,$CZ,$2,0,$3,$4" \
         --imgsize="$SZ" --colorscheme=Tomorrow --projection=p \
         -o "docs/img/$1.png" "$5" 2>/dev/null
    printf 'ok\n'
}

# import() resolves relative to the .scad file, not the working directory, so
# the STL directory is passed absolute.
A=cad/render_assembly.scad
D="-D stl=\"$PWD/export/stl\""
shot render-assembly-iso    58 25 430 "$A" "$D -D explode=0"
shot render-assembly-front   0  0 400 "$A" "$D -D explode=0"
shot render-exploded        60 28 640 "$A" "$D -D explode=22"

# A bare .stl is not a valid OpenSCAD input, so each single-part view goes
# through a one-line wrapper that imports it.
for v in "chassis-front 0 0 380 chassis" "chassis-iso 58 25 430 chassis" \
         "chassis-rear 180 0 380 chassis" "backplate-iso 58 25 430 backplate" \
         "backplate-rear 180 0 380 backplate"; do
    read -r n rx rz d part <<<"$v"
    echo "color(\"#cfcabf\") import(\"$PWD/export/stl/$part.stl\");" > "$T/$n.scad"
    shot "$n" "$rx" "$rz" "$d" "$T/$n.scad"
done

# Detail views for visual confirmation of the ports and controls.
for v in "detail-left-flank 78 -90 300 chassis" "detail-top-edge 78 180 260 chassis"; do
    read -r n rx rz d part <<<"$v"
    echo "color(\"#cfcabf\") import(\"$PWD/export/stl/$part.stl\");" > "$T/$n.scad"
    shot "$n" "$rx" "$rz" "$d" "$T/$n.scad"
done
