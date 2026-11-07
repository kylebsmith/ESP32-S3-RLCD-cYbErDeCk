// ===========================================================================
//  v2/tools/render/bodies.scad
//  One mesh per MATERIAL, out of the project's own reference bodies.
//
//      openscad -D 'body="disp_pcb"' --export-format=binstl \
//               -o work/stl/disp_pcb.stl v2/tools/render/bodies.scad
//
//  WHAT THIS FILE IS FOR
//  ---------------------
//  v2/cad/parts/*.scad already model the display, the Feather and the
//  stripped K9 Pro, and they already carry color() calls -- but a single STL
//  cannot carry more than one material, and the whole point of using a real
//  renderer is that a PCB is green, a silkscreen legend is white and an
//  aluminium frame is aluminium. So each reference body is exported once per
//  material group, by calling the modules those files already expose.
//
//  NOTHING IS REMODELLED HERE. Every line below is a call to a module defined
//  in v2/cad/parts/, with its own defaults. This file adds no dimension, no
//  position and no shape. The material grouping is the only decision it makes,
//  and the groups follow the color() calls those files already wrote:
//  ws_board() colours ws_usbc and ws_microsd "#c8c8cc" and ws_header_2x8
//  "#101010", so usbc+microsd go in the metal group and the header in the
//  black group. Where this file differs from those colours it says so.
//
//  WHY `use` AND NOT `include`
//  ---------------------------
//  `use` imports the modules and NOT the file-scope variables, so nothing in
//  v2/cad/parts/ can be shadowed or overridden from here. These files are
//  read-only to this tool and `use` is how that is enforced by the language
//  rather than by good intentions.
//
//  THE ONE CONSEQUENCE: $fn is a special variable and a `use`d file's own
//  `$fn = ...` does not reach this file's call sites, so it is set below. It
//  affects ONLY the faceting of these reference bodies. It cannot affect the
//  fabricated parts, which are exported by v2/tools/export.py from
//  v2/cad/slab.scad, backpanel.scad and carrier.scad and are never touched by
//  this file.
// ===========================================================================

use <../../cad/parts/waveshare_rlcd42.scad>
use <../../cad/parts/feather_rp2040_dvi.scad>
use <../../cad/parts/k9pro_internals.scad>

//  [DESIGN] render-only faceting. 96 is what the reference bodies choose for
//  their own non-preview renders; this matches it so the picture is not
//  coarser than the files it is a picture of. render.py passes it with -D so
//  a quick iteration can drop it without editing anything.
fn = 96;
$fn = fn;

body = "";

// ======================================================= Waveshare RLCD 4.2
if      (body == "disp_pcb")      ws_pcb();
else if (body == "disp_glass")    ws_display();
else if (body == "disp_metal")  { ws_standoffs(); ws_usbc(); ws_microsd(); }
else if (body == "disp_black")  { ws_header_2x8(); ws_mics(); ws_speaker();
                                  ws_wroom(); ws_battery_holder(); }
else if (body == "disp_buttons")  ws_buttons();
else if (body == "disp_silk")     ws_silk();

// ================================================= Feather RP2040 DVI (5710)
else if (body == "fw_pcb")        fw_pcb();
else if (body == "fw_metal")    { fw_hdmi(); fw_usbc(); }
else if (body == "fw_black")    { fw_jst_ph(); fw_stemma_qt(); fw_passives(); }
else if (body == "fw_buttons")    fw_buttons();
else if (body == "fw_silk")       fw_silk();

// ================================================ Keychron K9 Pro, stripped
//  k9_frame() is the aluminium top frame AND the switch plate -- one solid,
//  which is why they are one body here too (k9pro_internals.scad: "It is not a
//  separate part and it cannot be separated").
else if (body == "kbd_frame")     k9_frame();
else if (body == "kbd_pcb")       k9_pcb();
else if (body == "kbd_switches")  k9_switches();
else if (body == "kbd_keycaps")   k9_keycaps();
else if (body == "kbd_sockets") { k9_sockets(); k9_bosses(); }
else if (body == "kbd_ports")     k9_ports();
else if (body == "kbd_battery")   k9_battery();

//  An unrecognised body name emits nothing, which produces an empty STL.
//  render.py treats a zero-triangle export as FATAL rather than as an empty
//  layer, because "the part silently vanished from the picture" is exactly the
//  failure this whole tool exists to prevent.
