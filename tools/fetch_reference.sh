#!/usr/bin/env bash
# fetch_reference.sh - rebuild reference/ from primary vendor sources.
#
# WHY THIS EXISTS
#
# docs/PROVENANCE.md promises that no Waveshare file is redistributed by this
# repository. reference/ is therefore gitignored, which means a fresh clone has
# no datasheets in it. This script rebuilds that tree from the vendor's own
# URLs, so the reference material is reproducible without being vendored.
#
# Every fact in docs/HARDWARE.md cites a file this script fetches. If a URL
# rots, that is itself worth knowing - the script fails loudly rather than
# leaving a half-populated tree that looks complete.
#
#   tools/fetch_reference.sh          # fetch docs + datasheets (~12 MB)
#   tools/fetch_reference.sh --code   # also clone the example repos (~900 MB)

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
REF="$ROOT/reference/waveshare"
DOCS="https://docs.waveshare.com/ESP32-S3-RLCD-4.2"
FILES="https://files.waveshare.com/wiki"
WANT_CODE=0
[ "${1:-}" = "--code" ] && WANT_CODE=1

mkdir -p "$REF"
cd "$REF"

get() {  # get <url> <dest>
    if curl -fsS --retry 4 --retry-delay 2 -m 300 -o "$2.part" "$1"; then
        mv "$2.part" "$2"
        printf '  %-38s %8s bytes\n' "$2" "$(stat -c%s "$2")"
    else
        rm -f "$2.part"
        printf '  %-38s FAILED  %s\n' "$2" "$1" >&2
        return 1
    fi
}

echo "-- documentation pages --"
get "$DOCS" _docs.html
for p in Resources-And-Documents Arduino ESP-IDF FAQ ESP32-AI-Tutorials Technical-Support; do
    get "$DOCS/$p" "$p.html"
done

echo "-- datasheets and hardware --"
get "$FILES/common/ST_7305_V0_2.pdf"                        ST7305_datasheet.pdf
get "$FILES/common/ES8311.DS.pdf"                           ES8311.pdf
get "$FILES/common/SHTC3_Datasheet.pdf"                     SHTC3.pdf
get "https://documentation.espressif.com/esp32-s3_datasheet_en.pdf"      esp32-s3_datasheet.pdf
get "$FILES/ESP32-S3-RLCD-4.2/ESP32-S3-RLCD-4.2-schematic.pdf" ESP32-S3-RLCD-4.2-schematic.pdf
get "$FILES/ESP32-S3-RLCD-4.2/ESP32-S3-RLCD-4.2-3dFile.rar"    ESP32-S3-RLCD-4.2-3dFile.rar

echo "-- searchable text --"
for p in ST7305_datasheet ESP32-S3-RLCD-4.2-schematic; do
    [ -f "$p.pdf" ] && pdftotext -layout "$p.pdf" "${p%_datasheet}.txt" 2>/dev/null \
        && echo "  ${p%_datasheet}.txt" || true
done

if [ "$WANT_CODE" = 1 ]; then
    echo "-- example code (large) --"
    mkdir -p code && cd code
    clone() { [ -d "$2" ] || git clone -q --depth 1 "$1" "$2" && echo "  $2"; }
    clone https://github.com/waveshareteam/ESP32-S3-RLCD-4.2.git            waveshare-examples
    clone https://github.com/nilseuropa/solar_os.git                        solar_os
    clone https://github.com/nilseuropa/solar_term.git                      solar_term
    clone https://github.com/JasonHEngineering/waveshare_RLCD_400x300_monochrome.git rlcd-mono
fi

echo "done. reference/ is gitignored by design; see docs/PROVENANCE.md."
