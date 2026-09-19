// ===========================================================================
//  render_assembly.scad - PRESENTATION ONLY. No geometry is defined here.
//
//  Why this file exists. Rendering the assembly straight from cyberdeck.scad
//  goes through OpenSCAD's OpenCSG preview, which depth-peels a limited number
//  of CSG layers. The chassis is a deep difference tree - shell, two flared
//  apertures, port tunnels, bosses, grille, groove - and past that limit the
//  preview stripes surfaces it cannot resolve. The striping is an artefact of
//  the previewer, not of the model: the exported STLs are correct.
//
//  So the published views are assembled from the STLs instead. Each one has
//  already been through CGAL, so it is a plain mesh with no CSG left to peel,
//  and it renders cleanly with colour.
//
//    openscad -D 'stl="export/stl"' -D 'explode=22' render_assembly.scad
// ===========================================================================

stl     = "../export/stl";   // directory holding the rendered parts
explode = 0;

e = explode;

// The explode multipliers follow the assembly order: the shell stays put, the
// components withdraw rearward in the order they are loaded, and the back
// plate - which closes over all of them - travels furthest.
color("#cfcabf")                        import(str(stl, "/chassis.stl"));
color("#b6b0a4") translate([0,0,-e*3.0]) import(str(stl, "/backplate.stl"));
color("#2f6b45") translate([0,0,-e*1.8]) import(str(stl, "/mock-board.stl"));
color("#14171a") translate([0,0,-e*1.8]) import(str(stl, "/mock-screen.stl"));
color("#2b2e33") translate([0,0,-e*0.9]) import(str(stl, "/mock-keyboard.stl"));
color("#a8342b") translate([0,e*0.5,0])  import(str(stl, "/buttons-fitted.stl"));
