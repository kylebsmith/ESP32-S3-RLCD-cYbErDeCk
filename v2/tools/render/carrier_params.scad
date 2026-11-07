// ===========================================================================
//  v2/tools/render/carrier_params.scad
//  ECHO-ONLY. The two placements the scene needs that are defined in
//  v2/cad/carrier.scad rather than in parameters.scad.
//
//      openscad --export-format=echo -o out.echo v2/tools/render/carrier_params.scad
//
//  Including carrier.scad does put carrier_3d() in the tree, but
//  --export-format=echo evaluates the tree without tessellating it, so this
//  runs in ~30 ms and never touches CGAL. Measured, not assumed:
//  render.py times it and prints the time.
//
//  This file is SEPARATE from deck_params.scad on purpose. carrier.scad is
//  authored independently and may not exist. If it is absent, render.py drops
//  the Feather from the scene and lists it as an ABSENT body -- which is the
//  honest outcome, because without the carrier there is nothing for it to
//  mount to, and drawing it anyway would be a stand-in that later disagrees
//  with the real part.
// ===========================================================================

include <../../cad/carrier.scad>

module K(n, v) { echo(str("K ", n, "=", v)); }
module P(n, t, r) { echo(str("P ", n, "=", concat(t, r))); }

//  carrier.scad: feather_y = interior_y1 - feather[1]/2, the board's rear edge
//  butted to the interior rear wall so the HDMI reaches it.
K("feather_y", feather_y);

//  Rz(+90) sends body +x (the HDMI end) to deck +y (rearward) and body +y to
//  deck -x, so the translation is the board's own half-extents with those
//  signs. Derivation in deck_params.scad, under "the Feather".
P("feather",
  [feather_x + feather[0]/2, feather_y - feather[1]/2, 0],
  [0, 0, 90]);
