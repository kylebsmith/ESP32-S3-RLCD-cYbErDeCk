# Tools, tests and the documents

*Part of [the deck, top to bottom](README.md). Snapshot: commit `85d1e6a`, 2026-09-28.
What checks the deck, what draws its documents, and what every file in `docs/` is for.*

## 1. Tools, host tests and CI

### 1.1 CI — `.github/workflows/ci.yml`

One job, `audit`: ubuntu-latest, 45 min (ci.yml:12-15). It triggers on every push, every PR and manual dispatch. Steps in order:

1. apt `openscad xvfb`; Python 3.11; `pip install -r tools/requirements.txt` (ci.yml:20-39).
2. Fetch `nilseuropa/solar_term` to `/tmp/solar_term`, **pinned to `c4053c6`**, the commit the design was audited against — non-fatal. Pinned on 2026-09-28, when the upstream moved `stl/ata` to `stl/rlcd_ata` and the unpinned clone broke every push.
3. `./tools/build.sh` (54-55).
4. `measure_reference.py --reference /tmp/solar_term --json export/reports/measurements.json` — non-fatal (59-63).
5. Each host C check compiled with `cc`/`gcc` into `/tmp` and run (81-325; table §6.2).
6. `grep -q HAVE_O_NONBLOCK=1 firmware/components/ssh/CMakeLists.txt` (120-122).
7. `make_font.py` output diffed against both committed faces, then `check_fonts.py` (330-336).
8. `make_font.py --view` diffed against `view/deckview/deckfont.h` (349-350).
9. `check_docs.py` (352-353).
10. A grep for unresolved-work markers (the three usual capitalised words) over `cad/ docs/ tools/`, in `*.scad *.md *.py *.sh *.txt *.yml` (355-363) — which is why no document here spells them.
11. Upload STLs, reports and drawings (365-373).

**CI never builds the firmware** — there is no ESP-IDF/`idf.py` step anywhere.

### 1.2 Host C tests (`tools/test_*.c`) — all 19 run in CI

Most include the **shipping** header, or `#include` the shipping `.c`, rather than a copy.

| Test | Checks | Compiled against | CI (ci.yml) |
|---|---|---|---|
| test_st7305_addr.c | CASET/RASET window arithmetic; must tell the mirrored rule from the naive one (NARROW_LEFT) | st7305_addr.h | 81-86, `-std=c99 -Werror` |
| test_textgrid.c | The pre-turned face equals the per-row path: every glyph × attribute × both faces × 4 orientations, byte for byte; 2 deliberate breaks must be caught | `#include`s textgrid.c + both font .c | 96-104 |
| test_vitals.c | Every vitals verdict, including STOPPED DEAD / RESTART HUNG, which the bench cannot produce | vitals_verdict.h | 110-114 |
| test_clock.c | 1 h simulation with a 2 ppm follower: ticks follow the grid; the old periodic model must fail | seq_clock.h, ens_count.h | 130-136, `-O2 -lm` |
| test_ask.c | Secret prompt stars/hands over/wipes; old-style password cut; `ssh-keygen -lf` fingerprint format | ask.h, secret_line.h, ssh_fp.h | 146-153 |
| test_mirror_path.c | One SD file per document, `+` buffers not mirrored, FAT-safe names | mirror_path.h | 160-165 |
| test_seq_pattern.c | Step→column mapping (spaced patterns), refusal positions, slot timing incl. 5/7-way splits | seq_pattern.h | 172-177 |
| test_scale.c | Mode table, `>scale` parser (e.g. `cblues`), degree→note | seq_scale.h | 182-187 |
| test_pieces.c (+ viz.c) | Every line of `pieces/*.txt`, and each set doubled, through the compiler, names, key and picture table, with the verb table read from builtins.c; `-s` prints the notes | seq_pattern.h, lane_name.h, seq_scale.h, ui_text.h, viz.c | 195-204 |
| test_corpus.c | The deck plays what Strudel plays: onsets, lengths, values per cycle, from `tools/corpus/strudel.txt` | seq_pattern.h | 211-216 |
| test_lane_name.c | Canonical lane addresses (`disc:1` = `disc`); definitions refuse what they cannot mean | lane_name.h | 222-227 |
| test_ui_text.c | Every fixed string, and guide/boot line, fits 30 columns; the guide teaches no password-on-a-line | ui_text.h (+ pattern, lane, secret headers) | 234-240 |
| test_cell_attr.c | Cursor/playhead/command bits compose (the blink bug) | cell_attr.h | 257-263 |
| test_osc.c | OSC pack/parse round trip; then inline Python proves `tools/osc_listen.py`'s parser reads firmware-format packets | osc_pack.h, osc_parse.h | 270-291 |
| test_serialkbd_map.c | CR = Enter, LF = Ctrl+Enter, arrows, Ctrl-letters | serialkbd_map.h | 293-298 |
| test_keymap_agree.c | `SKB_*` constants equal kbd.h's enum | kbd.h, serialkbd_map.h | 299-302 |
| test_midi_wire.c | Data-byte count for every status; 0xF9 is the only internal status | midi_len.h | 307-312, `gcc`, **no -Werror** |
| test_viz.c (+ viz.c) | Help names resolve; `s_names`/`s_draw` in the same order; pipeline runs in table order; echo only fades | viz.c | 319-325, `gcc`, **no -Werror** |
| test_view_wire.c | DKV1 frames packed by view_wire.h are read by the node's view_read.h, including after junk and torn frames | view_wire.h, view/deckview/view_read.h | 343-348 |

**Run locally this session** (Apple clang, CI flags, output in the scratchpad): all 19 pass. `test_pieces` passes, 6 files (3 pieces + 3 doubled). `test_corpus`: 51 in scope, 51 match, 13 out of scope. Both font diffs, `check_fonts.py`, the `deckfont.h` diff and `check_docs.py` (49 corrections, 9 open items) pass. `build.sh` was **not** run: it rewrites tracked `export/` files.

### 1.3 Scripts (`tools/*.py`, `*.sh`)

| Tool | Does | Run by |
|---|---|---|
| build.sh | Renders `chassis backplate buttons cover assembly` and the two case halves; an OpenSCAD undef warning is fatal (build.sh:17-54). Then `test_primitives`, `validate --json`, `check_case`, `audit_reference` (if the ref clone exists), `drawing`, `structure --stations 90 --plot` (56-88) | CI step 3 |
| validate.py | The 119-check design gate (MESH ENVELOPE FIT OBSTRUCTION STACK INTERFACE PRINT DATUM) → `export/reports/validation.json` | build.sh |
| test_primitives.py | Unit tests for `cad/lib/util.scad` helpers, measured from renders | build.sh |
| check_case.py | Carry case measured from its mesh: bores, metal around them, spacing | build.sh |
| audit_reference.py | Component-facing geometry vs `solar_term`: MATCH / INTENDED / REVIEW; non-zero exit on REVIEW | build.sh (only with the clone) |
| drawing.py | Dimensioned GA sheets measured from the mesh → `export/drawings/` | build.sh |
| structure.py | Section properties (I, Z, Bredt) from mesh sections | build.sh |
| measure_reference.py | Metrology harness → `export/reports/measurements.json` | CI step 4 (non-fatal) |
| params.py | Parses `cad/parameters.scad` into a dict (shared library) | imported |
| check_docs.py | Cited C-/O-/D- records exist, the C- sequence is whole, relative links resolve, README counts agree | CI |
| check_fonts.py | Decodes the committed font C back to its art; tile-arc continuity | CI |
| make_font.py | Generates font6x12.c (default), font12x24.c (`--12x24`) and `view/deckview/deckfont.h` (`--view`) | CI (diffed) |
| font12x24_art.py / font_tiles.py | The 12×24 art (source of truth) / geometric tiles 128-155 for both faces | imported |
| check_golden.py | v1 parameters and mesh extents vs `tests/golden/v1.json` (`--stl`, `--update`) | **manual only — not in CI or build.sh** |
| render.sh | Published renders in `docs/img` from exported STLs, cameras stated | manual |
| fetch_reference.sh | Rebuilds the gitignored `reference/` from Waveshare URLs (~12 MB; `--code` ~900 MB) | manual |
| osc_listen.py | Zero-dependency reference OSC receiver (default port 9000) | manual; its parser is exec'd in CI (ci.yml:276-291) |
| osc_send.py | Sends one OSC message to `>osc in` (float or int) | manual |
| viewrelay.py | Holds the deck's console, lifts `ESC]view;…BEL` frames out and writes them to the RP2040 node; needs pyserial | manual |
| preview_ui.py | Renders a PNG mock of the screen from the font art and layout constants parsed from the firmware; needs Pillow | manual |
| cmf.py | Draws the `docs/img/cmf-*.png` specimens (glyphs, tiles, metrics, pictures, marks, corner) from sources; needs Pillow | manual |
| zine.py / zine_pages.py / zine_refs.py | Sets zine #0 (16 pages, `hello-pages.pdf`, `hello-booklet.pdf`, PNGs) in the deck's faces / page layouts / numbered refs; needs Pillow | manual (`zine/README.md:39-43`) |
| zine_art.c | Links viz.c, plays scripted scenes, writes `.cells` frames for zine.py and cmf.py | manual compile |
| mock_pictures.py / mock_frames.c | The picture mock-ups in `wiki/pictures-and-type.md`, drawn by viz.c and by a square-dot copy of it: dots, banding, the screens, the motion GIF, and each screen's truth and sparkle; needs a C compiler, Pillow and numpy | manual |
| mock_view.py | The view node's proposed modes (plain, scan, phosphor, feedback, riso, poster) from the engine's real frames, still and moving; needs a C compiler, Pillow and numpy | manual |
| view_demo.py | Drives a real view node with no deck: the engine's own frames, packed as the deck packs them, through all six modes (`--mode` for one); needs a C compiler and pyserial | manual |
| mock_type.py | The type proposal sheet, from the deck's own faces | manual |
| type_programme.py | Letters drawn from the superellipse rule: two drafts, set aside for breaking letters | manual |
| type_round.py | The round face, laid by hand, and `check()`: pieces, corner contacts, strays, spurs, necks, gap ink; exits non-zero on any failure | manual |
| requirements.txt | CI's Python deps: trimesh numpy scipy shapely ezdxf matplotlib + networkx rtree manifold3d | CI |
| corpus/features.txt | Hand-written Strudel patterns, by feature | input to gen.mjs |
| corpus/gen.mjs | Node script: runs `@strudel/core`/`mini` 1.2.6 as an oracle and writes the corpus | manual |
| corpus/strudel.txt | Generated corpus: 64 entries, 51 checked, 13 out of scope (`X`) | read by test_corpus |
| hostshim/esp_err.h, esp_log.h | Minimal IDF shims so firmware headers and sources compile on the host | included by tests |

**Not covered by CI:**
- the firmware build;
- `seq.c` itself: the realtime core needs esp_timer/FreeRTOS, so only its pure headers are tested (test_viz.c:11-15);
- `check_golden.py`, `render.sh`, cmf/zine/preview/relay tools, the mock-up and type tools, `gen.mjs`;
- the marker grep does not look in `firmware/`.

## 2. Documents and the other directories

### 2.1 `docs/` (24 + img)

| Doc | Covers |
|---|---|
| ASSEMBLY.md | BOM (printed + hardware), print settings, orientations, assembly, rigging, servicing; "not built" warning |
| CASE.md | The two-part bolted carry case (`cad/carrycase.scad`): form, fasteners, printing, open items |
| CMF.md | Colour, material, finish: object, panel, both faces and every glyph, pictures, marks, printed matter; brief for the type overhaul |
| COMMANDS.md | Calling convention (`>` sigil), capabilities, buffer kinds, guide files, save model, journal as archive, output as buffer, pattern grammar |
| CONCRETE.md | Cast concrete jacket over the printed chassis (`cad/concrete.scad`): mould, mix, procedure |
| DATUMS.md | Dimensional datum sheet D-01…D-09 with provenance; 49 corrections (C-); 9 open items (O-); measure-before-print list |
| DESIGN.md | Enclosure rationale: footprint, one form, form language, material, back loading, battery cowl, fasteners, rig points |
| GRAPHICS.md | 2026-09-27 analysis and brief: panel cost model, where the cycles go, the 16 primitives, live-coding landscape, next steps |
| HARDWARE.md | Board reference: SoC/memory, ST7305 (window addressing, CASET mirroring), prior art, open questions, DIN-MIDI wiring |
| MANIFESTO.md | The syntax on one page, two adversarial reviews, and which proposals were decided |
| MAP.md | Every verb and what it touches; the subtractive map; unification and nesting done; proposals judged |
| MEASURE.md | Caliper checklist (10 items) for someone holding the real board and keyboard |
| METHODOLOGY.md | How datums were measured and validated, how to reproduce them, limits |
| NETWORK.md | Wi-Fi AP/STA feasibility and numbers, SSH as built, OSC in vs keyboard scan, ESP-NOW ensemble |
| NEXT.md | The next development push: ground rules, step decision, repetition, satellites, view node, Link, OSC in, optimisation, SSH, Strudel, thesis |
| OS.md | Firmware design: thesis, hardware constraints, IDF 5.5 base, architecture, display, input, editing, discovery, voice, MIDI/network, SD mirror |
| PROVENANCE.md | What was taken from whom (reference enclosure, Waveshare, Riitek, standards); licence |
| SATELLITES.md | Brief for satellite nodes: 6-pin magnetic cable, CAN 500 kbit/s, frames, 4-encoder and 8-button nodes (none built) |
| STRUDEL.md | Coming from Strudel: what transfers, what is spelled differently, measured coverage |
| SUBSTRATE.md | The conceptual core: one data structure (a rectangle of characters), buffer kinds, addressing |
| TESTING.md | On-device test pass: numbered steps with expected results, drivable over the serial cable |
| THESIS.md | Running note of decisions made for legibility, for the paper |
| VERBS.md | All 34 verbs on one page (clock 6, connecting 7, looking 6, documents 8, when-wrong 7) |
| VIEW.md | RP2040 DVI HDMI view node: size, DKV1 wire format, transport (console relay today), measurements |
| img/ | 23 PNGs: enclosure and case renders, CMF specimens, graphics mosaic |

### 2.2 Other directories

| Path | What is there |
|---|---|
| pieces/ | README plus three performance sets as deck documents: `ground` (four grooves), `lift` (five sketches after live coders), `orbitals` (the piece). Checked in CI by test_pieces |
| zine/ | *hello* zine #0: `hello-booklet.pdf`, `hello-pages.pdf` (16 pp), `hello.txt` (the zine as a deck document), README with build steps and full references |
| cad/ | OpenSCAD sources. `parameters.scad` is the single source of truth, `variant = 2` (parameters.scad:1170). `cyberdeck.scad` dispatches chassis/backplate/buttons/cover/assembly. Also `carrycase.scad`, `concrete.scad`, `render_assembly.scad` (presentation only), `lib/util.scad` (helpers) and `lib/components.scad` (worst-case component envelopes) |
| view/deckview/ | Arduino sketch for the Feather RP2040 DVI node: `deckview.ino`, `view_read.h` (tested in CI), `deckfont.h` (generated) |
| export/ | Tracked outputs regenerated by build.sh: `stl/` (parts, case, concrete, cover), `reports/` (validation 119/119, measurements), `drawings/` (6 sheets) |
| tests/golden/v1.json | Golden v1 parameters and meshes for check_golden.py |
| root | README.md (overview), HANDOFF.md (night-shift brief), STATUS.md (2026-09-20 status snapshot) |

