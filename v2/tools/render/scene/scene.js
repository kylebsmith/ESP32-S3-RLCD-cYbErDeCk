// ===========================================================================
//  v2/tools/render/scene/scene.js
//  The renderer. three.js r160.1, MeshPhysicalMaterial, headless Chromium.
//
//  THIS FILE CONTAINS NO DIMENSION AND NO GEOMETRY, BY RULE.
//  ---------------------------------------------------------
//  Every mesh is an STL loaded from /stl/, exported from v2/cad by
//  v2/tools/export.py (the fabricated parts) or by v2/tools/render/bodies.scad
//  (the reference bodies). Every matrix, every material parameter and every
//  camera number arrives in /manifest.json, which render.py writes by asking
//  OpenSCAD for the values in v2/cad/parameters.scad.
//
//  So the only things decided here are the ones a renderer is actually for:
//  how light behaves, how a material responds to it, and where the camera is.
//  If you find yourself typing a millimetre into this file, the number belongs
//  in parameters.scad and the plumbing belongs in render.py.
//
//  THE ACRYLIC IS THE WHOLE POINT
//  ------------------------------
//  MeshPhysicalMaterial's `transmission` with a non-zero `thickness` is what
//  separates a 12.700 mm slab from a sheet of cling film: `thickness` is in
//  WORLD UNITS and this scene is in millimetres, so the slab is given
//  acrylic_t and the refraction bends by the amount a half-inch sheet bends
//  it. With thickness left at 0 the same material reads as a glass film laid
//  on the parts, which is the standard way a clear-acrylic render lies.
//
//  Its honest limit, stated here and in the README: three.js transmission is
//  a SCREEN-SPACE single-bounce approximation. It refracts the already-
//  rendered opaque backdrop through the front surface using the surface
//  normal, the IOR and the thickness. It is not a path tracer: there are no
//  caustics, no correct second-surface refraction, and a transmissive body
//  does not refract another transmissive body. For a flat slab viewed from
//  the front -- which is every view this tool emits -- those terms are small,
//  and that is exactly why the views are parameterised the way they are.
// ===========================================================================
import * as THREE from '/scene/vendor/three.module.js';
import { STLLoader } from '/scene/vendor/STLLoader.js';

const DEG = Math.PI / 180;

// ---------------------------------------------------------------------------
//  MATERIALS. The table is in render.py (so the README can quote it and the
//  manifest can ship it); this turns a row of it into a three.js material.
// ---------------------------------------------------------------------------
function makeMaterial(spec) {
  const p = {
    color: new THREE.Color(spec.color ?? '#808080'),
    roughness: spec.roughness ?? 0.5,
    metalness: spec.metalness ?? 0.0,
  };
  for (const k of ['clearcoat', 'clearcoatRoughness', 'ior',
                   'specularIntensity', 'envMapIntensity',
                   'transmission', 'thickness', 'attenuationDistance']) {
    if (spec[k] !== undefined) p[k] = spec[k];
  }
  if (spec.attenuationColor) {
    p.attenuationColor = new THREE.Color(spec.attenuationColor);
  }
  if (spec.map && spec._texture) p.map = spec._texture;
  const m = new THREE.MeshPhysicalMaterial(p);
  //  A transmissive body must not write depth over the things seen through
  //  it, and it is single-sided because the slab's mesh is a closed solid.
  if ((spec.transmission ?? 0) > 0) {
    m.side = THREE.FrontSide;
    m.depthWrite = true;
    m.transparent = false;   // three.js routes transmission on its own pass
  }
  return m;
}

const MASK_MATERIAL = new THREE.MeshBasicMaterial({
  color: 0xffffff, side: THREE.DoubleSide,
});

// ---------------------------------------------------------------------------
//  THE ORBIT PARAMETERISATION, and the one place a sign convention matters.
//
//  The deck frame (parameters.scad): +X right as the user sits at it, +Y
//  rearward (keyboard at -Y, display at +Y), +Z up out of the carrier.
//
//      azimuth 0    camera on the -Y side, i.e. the user's own side
//      azimuth +    swings toward +X
//      elevation 0  in the deck's plane
//      elevation 90 straight down the Z axis
//
//  `up` is derived rather than hard-coded, because [0,0,1] is degenerate at
//  elevation 90 and the plan view would silently come out rolled. h is the
//  horizontal direction from target to camera; up = -h sin(el) + Z cos(el)
//  gives [0,0,1] at el=0 and -h (i.e. +Y rearward, for az=0) at el=90, with
//  no discontinuity between.
// ---------------------------------------------------------------------------
function orbit(azDeg, elDeg) {
  const az = azDeg * DEG, el = elDeg * DEG;
  const h = new THREE.Vector3(Math.sin(az), -Math.cos(az), 0);
  const dir = new THREE.Vector3(h.x * Math.cos(el), h.y * Math.cos(el),
                                Math.sin(el));
  const up = new THREE.Vector3(-h.x * Math.sin(el), -h.y * Math.sin(el),
                               Math.cos(el));
  if (up.lengthSq() < 1e-9) up.set(0, 1, 0);
  return { dir: dir.normalize(), up: up.normalize() };
}

// ---------------------------------------------------------------------------
//  LIGHTING. A studio, not a CAD viewport, and not a grey room.
//
//  The dominant term is an IBL, and it is not decoration: it is the only
//  thing a `transmission` material can refract, since there is nothing else
//  behind the slab at the edges of the frame, and point lights alone would
//  give clear acrylic nothing to show.
//
//  IT USED TO BE three.js's RoomEnvironment, AND THAT IS WHY EVERY RENDER OF
//  THIS DECK CAME OUT GREY. RoomEnvironment is a neutral grey room whose
//  brightest source is about 1.0. A dielectric at roughness 0.02 reflects
//  what is in front of it, so if the brightest thing in the world is 1.0,
//  polished acrylic reflects 1.0 and reads as paint -- and every plastic in
//  this deck is specified matte black or very dark grey, so the IBL was
//  doing nearly all the work and had nothing bright to do it with. The
//  environment is now built here, in HDR, with sources an order of magnitude
//  over mid grey.
//
//  Then three conventional lights, all ARTISTIC, for the hard highlight a
//  prefiltered environment cannot give -- PMREM blurs a source into its
//  roughness lobe and a 0.02-roughness face wants a point of light, not a
//  lobe: a key high and camera-left, a cool fill opposite, and a rim from
//  behind to catch the slab's 12.700 mm edge, which is the one place the
//  thickness is directly visible and the brief's whole reason for the
//  half-inch sheet.
// ---------------------------------------------------------------------------
//  THE SOFTBOXES. Every number is [ARTISTIC] -- this is a lighting plan, not
//  a measurement -- but the SHAPE of it is argued. `dir` is in the DECK's own
//  frame (+X right, +Y rear, +Z up), `hu`/`hv` are the source's angular half
//  sizes in radians, and `L` is its radiance with 1.0 as mid grey, so a value
//  of 9 is a source nine times brighter than the brightest thing a
//  low-dynamic-range backdrop could be. That headroom is the entire point:
//  the old environment was three.js's RoomEnvironment, a neutral grey room,
//  and a neutral grey room is exactly what made every previous render of this
//  deck grey. A dielectric with roughness 0.02 reflects what is in front of
//  it; if the brightest thing in the world is 1.0, polished acrylic reflects
//  1.0 and reads as paint.
const SOFTBOXES = [
  //  KEY. High, and on the player's left, which is the side the perspective
  //  views orbit away from -- so it rakes the top face rather than facing it,
  //  and the specular lobe lands along the slab's long edge instead of in the
  //  middle of it. Warm, 5600 K-ish against the fill.
  { dir: [-0.42, -0.52, 0.74], hu: 0.48, hv: 0.23, L: 9.0,
    rgb: [1.00, 0.955, 0.885] },
  //  FILL. Cool, opposite, a seventh of the key. Its job is to keep the
  //  shadow side of the knobs and the carrier's pockets from going to black,
  //  because a part that is pure black in a render is a part that has not
  //  been drawn as far as a reader is concerned.
  { dir: [0.78, 0.30, 0.42], hu: 0.80, hv: 0.55, L: 2.10,
    rgb: [0.80, 0.875, 1.00] },
  //  RIM. The one source with a stated job: the slab's 12.700 mm EDGE. It is
  //  a long narrow strip (hu 1.10, hv 0.09) raked along the rear at a
  //  glancing elevation, because an edge 12.700 mm tall and 304.600 long is
  //  lit by a strip and washed out by anything rounder.
  { dir: [0.06, 0.96, 0.28], hu: 1.10, hv: 0.09, L: 7.0,
    rgb: [1.00, 0.99, 0.97] },
  //  KICKER. Low and in front, under the horizon, for the slab's FRONT edge
  //  and the feet. Weak and wide; it exists so the deck does not float on a
  //  black nothing at the bottom of the frame.
  { dir: [-0.10, -0.92, -0.38], hu: 0.95, hv: 0.22, L: 1.20,
    rgb: [0.96, 0.96, 1.00] },
];

//  Equirectangular size. 512 x 256 is well past what PMREM needs for a
//  roughness-0.02 reflection of sources this large, and it costs 1.5 MB of
//  float that is thrown away after the prefilter.
const ENV_W = 512, ENV_H = 256;

//  The ambient floor and ceiling of the room, [ARTISTIC]. Not zero: a
//  transmissive body refracts whatever is behind it, and a black world gives
//  clear acrylic nothing to show -- which is the failure mode the README's
//  section 4 describes and the reason the environment is not decoration.
const ENV_FLOOR = [0.012, 0.013, 0.015];
const ENV_SKY   = [0.072, 0.080, 0.098];

function softbox(d, s) {
  //  An orthonormal frame about the source's axis, then the two angles from
  //  the axis in that frame. Rectangular, with a smoothstep edge, which is
  //  what a softbox with a diffusion panel on it actually is.
  const ax = new THREE.Vector3(...s.dir).normalize();
  const up = Math.abs(ax.z) > 0.9 ? new THREE.Vector3(1, 0, 0)
                                  : new THREE.Vector3(0, 0, 1);
  const a = new THREE.Vector3().crossVectors(up, ax).normalize();
  const b = new THREE.Vector3().crossVectors(ax, a).normalize();
  const along = d.dot(ax);
  if (along <= 0) return 0;
  const u = Math.atan2(d.dot(a), along) / s.hu;
  const v = Math.atan2(d.dot(b), along) / s.hv;
  const r = Math.hypot(u, v);
  //  1 inside, 0 outside 1.35, smooth between: the penumbra of the panel.
  if (r >= 1.35) return 0;
  if (r <= 1.0) return 1;
  const t = (1.35 - r) / 0.35;
  return t * t * (3 - 2 * t);
}

//  THE ENVIRONMENT, GENERATED HERE AND FETCHED FROM NOWHERE.
//  An HDR equirectangular image built texel by texel, then prefiltered by
//  PMREMGenerator exactly as a loaded .hdr would be. It is a build input in
//  the same sense three.js is -- except that it has no download, no licence
//  and no vendor directory, because it is six numbers and a loop.
//
//  THE ONE CONVENTION THAT MATTERS. three.js's equirect mapping uses +Y as
//  its polar axis and this deck is +Z up, so the image is NOT authored in
//  image space. Each texel's direction is reconstructed first --
//      theta = (v - 0.5) * PI,  phi = (u - 0.5) * 2PI
//      d = [cos(theta) cos(phi), sin(theta), cos(theta) sin(phi)]
//  which is three.js's own equirectUv() inverted -- and the lighting function
//  is then evaluated on d in the DECK's frame, where d[2] is up. Authoring it
//  the other way round puts the key light out of the back of the keyboard.
function buildEnvironment(renderer) {
  const data = new Float32Array(ENV_W * ENV_H * 4);
  const d = new THREE.Vector3();
  for (let y = 0; y < ENV_H; y++) {
    const theta = ((y + 0.5) / ENV_H - 0.5) * Math.PI;
    for (let x = 0; x < ENV_W; x++) {
      const phi = ((x + 0.5) / ENV_W - 0.5) * 2 * Math.PI;
      d.set(Math.cos(theta) * Math.cos(phi), Math.sin(theta),
            Math.cos(theta) * Math.sin(phi));
      //  d[2] is the DECK's up. A smooth floor-to-sky gradient, with the
      //  horizon a little brighter than either, which is what a room with
      //  walls looks like from the middle of it.
      const up = d.z;
      const k = 0.5 + 0.5 * up;
      const horizon = Math.exp(-(up * up) / 0.012) * 0.06;
      const rgb = [0, 1, 2].map(
        i => ENV_FLOOR[i] * (1 - k) + ENV_SKY[i] * k + horizon);
      for (const s of SOFTBOXES) {
        const w = softbox(d, s);
        if (w > 0) for (let i = 0; i < 3; i++) rgb[i] += w * s.L * s.rgb[i];
      }
      //  The image's row 0 is v = 0, which is -Y; three.js's own flip on
      //  upload is bypassed for a DataTexture, so the row order here is the
      //  texture's row order and the reconstruction above is what defines it.
      const o = (y * ENV_W + x) * 4;
      data[o] = rgb[0]; data[o + 1] = rgb[1]; data[o + 2] = rgb[2];
      data[o + 3] = 1;
    }
  }
  const tex = new THREE.DataTexture(data, ENV_W, ENV_H, THREE.RGBAFormat,
                                    THREE.FloatType);
  tex.mapping = THREE.EquirectangularReflectionMapping;
  tex.minFilter = THREE.LinearFilter;
  tex.magFilter = THREE.LinearFilter;
  tex.needsUpdate = true;
  const pmrem = new THREE.PMREMGenerator(renderer);
  pmrem.compileEquirectangularShader();
  const env = pmrem.fromEquirectangular(tex);
  tex.dispose();
  return { texture: env.texture,
           dispose: () => { env.dispose?.(); pmrem.dispose(); } };
}

//  The three conventional lights that sit on top of the IBL. They are what
//  give a dielectric a hard highlight the prefiltered environment cannot --
//  PMREM blurs a source into its roughness lobe, and a 0.02-roughness face
//  wants a point of light, not a lobe.
function buildLights(scene, renderer, radius) {
  const env = buildEnvironment(renderer);
  scene.environment = env.texture;

  //  1.55, and the number was set on the PLAN view rather than on the hero.
  //  The plan looks straight down, so every surface it shows is lit by the
  //  key's own +Z component and by nothing else that matters -- which makes
  //  it the view where this intensity is visible. At 2.30 the hero was still
  //  fine (mean luma 43.2 against the old 43.9) and the plan's median went
  //  from 64.9 to 91.2 with its 95th percentile at 226.7, i.e. clipping.
  //  [MEASURED, two renders]
  const key = new THREE.DirectionalLight(0xfff4e4, 1.55);
  key.position.set(-0.42, -0.52, 0.74).multiplyScalar(radius * 3);
  scene.add(key);

  const fill = new THREE.DirectionalLight(0xccddff, 0.34);
  fill.position.set(0.78, 0.30, 0.42).multiplyScalar(radius * 3);
  scene.add(fill);

  //  The rim exists for ONE feature: the slab's 12.700 mm edge. That edge is
  //  the only place the thickness is directly visible and it is the brief's
  //  whole reason for the half-inch sheet, so it gets a light of its own,
  //  raked along the rear edge at a glancing angle. Its direction is the same
  //  as the rim softbox's, so the hard highlight lands inside the soft one.
  const rim = new THREE.DirectionalLight(0xffffff, 1.55);
  rim.position.set(0.06, 0.96, 0.28).multiplyScalar(radius * 3);
  scene.add(rim);

  return env;
}

// ---------------------------------------------------------------------------
//  THE BACKDROP. [ARTISTIC], and it is the only thing in the frame that is
//  not geometry: a vertical gradient on a canvas, which is the studio sweep a
//  product shot is taken against. It is a Texture on scene.background and NOT
//  a mesh, deliberately -- a floor plane would be a body in the scene, and
//  this tool's whole contract is that every body in the scene is a part.
//  A measurement view replaces it with flat black.
// ---------------------------------------------------------------------------
function backdrop() {
  const c = document.createElement('canvas');
  c.width = 4; c.height = 256;
  const g = c.getContext('2d').createLinearGradient(0, 0, 0, 256);
  g.addColorStop(0.00, '#1d2126');
  g.addColorStop(0.55, '#121519');
  g.addColorStop(1.00, '#07080a');
  const ctx = c.getContext('2d');
  ctx.fillStyle = g;
  ctx.fillRect(0, 0, 4, 256);
  const t = new THREE.CanvasTexture(c);
  t.colorSpace = THREE.SRGBColorSpace;
  return t;
}

// ---------------------------------------------------------------------------
//  THE SCREEN.
//
//  The panel is a 4.2 in REFLECTIVE LCD: 400 x 300, ONE BIT, bi-level, and no
//  backlight at all. So this builds the panel's framebuffer, at its own pixel
//  count, out of the character grid render.py put in the manifest, and hands
//  it over as a `map` on an ordinary dielectric. There is no emissive term
//  anywhere in it -- a reflective LCD is a mirror with a switchable mask over
//  it and every photon it sends you came out of the room.
//
//  NOTHING HERE DECIDES WHAT THE SCREEN SAYS. The lines, the grid, the cell
//  size, the pixel count and the two tones all arrive in manifest.screen,
//  which render.py assembles from v2/parts/wiring.md's own device tables; the
//  sidecar carries that provenance next to the picture. This function knows
//  how to draw characters into cells and nothing else.
//
//  THE THRESHOLD IS NOT A STYLE CHOICE. Canvas text is anti-aliased and the
//  panel is one bit per pixel, so the glyphs are quantised to the two tones
//  before they are uploaded. A greyscale screen would be a picture of a panel
//  that does not exist.
// ---------------------------------------------------------------------------
function screenTexture(screen) {
  const [W, H] = screen.px, [cw, ch] = screen.cell;
  const B = screen.bezel ?? 0;
  const ox = screen.origin[0] + B, oy = screen.origin[1] + B;
  const paper = screen.paper, ink = screen.ink, bezel = screen.bezel_color;
  const c = document.createElement('canvas');
  c.width = W + 2 * B; c.height = H + 2 * B;
  const g = c.getContext('2d', { willReadFrequently: true });
  //  the module's dark border first, then the panel inside it. Everything
  //  outside the active area clamps to this, which is what it should be.
  g.fillStyle = bezel;
  g.fillRect(0, 0, c.width, c.height);
  g.fillStyle = paper;
  g.fillRect(B, B, W, H);
  //  The cell is cw wide; a monospace advance is about 0.6 em, so 0.6 * size
  //  = cw sizes the face to the cell. Each glyph is then CENTRED in its own
  //  cell rather than laid out by the font's advance, so the grid is the
  //  panel's grid and not the font's.
  g.font = Math.round(cw / 0.6) + 'px ui-monospace, "DejaVu Sans Mono", '
           + 'monospace';
  g.textAlign = 'center';
  g.textBaseline = 'middle';
  const inv = new Set(screen.invert ?? []);
  (screen.lines ?? []).forEach((line, row) => {
    const y = oy + row * ch;
    if (inv.has(row)) {
      g.fillStyle = ink;
      g.fillRect(B, y, W, ch);
    }
    g.fillStyle = inv.has(row) ? paper : ink;
    for (let col = 0; col < line.length; col++) {
      const t = line[col];
      if (t === ' ') continue;
      g.fillText(t, ox + col * cw + cw / 2, y + ch / 2 + 1);
    }
  });
  //  Quantise to the two tones the panel has. INSIDE THE PANEL ONLY -- the
  //  bezel is the module's moulding, not a pixel, and thresholding it would
  //  make it one.
  const img = g.getImageData(B, B, W, H);
  const px = img.data;
  const lum = (h) => {
    const n = parseInt(h.slice(1), 16);
    return 0.2126 * ((n >> 16) & 255) + 0.7152 * ((n >> 8) & 255)
         + 0.0722 * (n & 255);
  };
  const mid = (lum(paper) + lum(ink)) / 2;
  const pn = parseInt(paper.slice(1), 16), kn = parseInt(ink.slice(1), 16);
  let lit = 0;
  for (let i = 0; i < px.length; i += 4) {
    const l = 0.2126 * px[i] + 0.7152 * px[i + 1] + 0.0722 * px[i + 2];
    const n = l >= mid ? pn : kn;
    if (l < mid) lit++;
    px[i] = (n >> 16) & 255; px[i + 1] = (n >> 8) & 255; px[i + 2] = n & 255;
  }
  g.putImageData(img, B, B);
  const t = new THREE.CanvasTexture(c);
  //  NEAREST on magnification, because the thing being shown IS a pixel grid:
  //  at 0.2120 mm a panel pixel is about two device pixels in the plan view,
  //  and smoothing it would turn the one honest thing about this texture --
  //  that it has exactly 400 x 300 of them -- into a blur.
  t.magFilter = THREE.NearestFilter;
  t.minFilter = THREE.LinearMipmapLinearFilter;
  t.generateMipmaps = true;
  t.colorSpace = THREE.SRGBColorSpace;
  t.anisotropy = 8;
  return { texture: t, inkPixels: lit };
}

//  Planar UV for the one body that carries the screen. The rectangle comes
//  from the manifest (ws_active_area(), echoed), in the body's OWN frame, so
//  the 400 x 300 pixels land on the active area the STEP measured and nowhere
//  else. Everything else on that mesh -- the spacer, the TFT border, the FPC
//  -- falls outside [0,1] and takes the texture's clamped edge, which is the
//  panel's own dark border and is what a reflective LCD's surround looks like.
function applyScreenUV(geom, box) {
  const [u0, v0, u1, v1] = box;
  const pos = geom.attributes.position;
  const uv = new Float32Array(pos.count * 2);
  for (let i = 0; i < pos.count; i++) {
    uv[i * 2] = (pos.getX(i) - u0) / (u1 - u0);
    uv[i * 2 + 1] = (pos.getY(i) - v0) / (v1 - v0);
  }
  geom.setAttribute('uv', new THREE.BufferAttribute(uv, 2));
}

// ---------------------------------------------------------------------------
//  THE SCENE
// ---------------------------------------------------------------------------
const state = { ready: false, error: null };
window.__deck = state;

async function boot() {
  const manifest = await (await fetch('/manifest.json')).json();
  state.manifest = manifest;

  const canvas = document.createElement('canvas');
  document.body.appendChild(canvas);
  const renderer = new THREE.WebGLRenderer({
    canvas, antialias: true, alpha: false, preserveDrawingBuffer: true,
  });
  renderer.outputColorSpace = THREE.SRGBColorSpace;
  //  ACES filmic, because a studio render with specular highlights on
  //  aluminium and a transmissive slab will clip in sRGB and the clipped
  //  highlight is what makes a render look like a screenshot.
  renderer.toneMapping = THREE.ACESFilmicToneMapping;
  //  THE EXPOSURE IS IN THE MANIFEST, not here. It used to be the literal
  //  0.78 in this file, which made it the one renderer number with no way to
  //  vary it from outside -- and the thresholds in check_render.py have to be
  //  calibrated against a SET of legitimate renders, of which a differently
  //  exposed one is the obvious example. At 1.05 the first run came out
  //  milky: the carrier is specified as "matte black or very dark grey"
  //  (v2/parts/stack.md 5.6) and it rendered mid-grey, which destroyed the
  //  contrast the brief asks for -- "a dark matte plane reads as a void, and
  //  the coloured wires and nickel nuts become the only things with
  //  brightness". Still [ARTISTIC]; now stated where every other number is.
  renderer.toneMappingExposure = manifest.exposure;

  const scene = new THREE.Scene();

  const [lo, hi] = manifest.scene_bounds;
  const centre = new THREE.Vector3((lo[0] + hi[0]) / 2, (lo[1] + hi[1]) / 2,
                                   (lo[2] + hi[2]) / 2);
  const radius = 0.5 * Math.hypot(hi[0] - lo[0], hi[1] - lo[1], hi[2] - lo[2]);
  state.radius = radius;
  state.centre = centre.toArray();

  const ibl = buildLights(scene, renderer, radius);
  const sweep = backdrop();

  //  The screen, before the materials, because one of them carries it.
  const screen = manifest.screen ?? null;
  let screenInk = 0;
  if (screen && screen.body) {
    for (const spec of Object.values(manifest.materials)) {
      if (spec.map !== 'screen') continue;
      screen.paper = spec.map_paper;
      screen.ink = spec.map_ink;
      screen.bezel_color = spec.map_bezel;
      const made = screenTexture(screen);
      spec._texture = made.texture;
      screenInk = made.inkPixels;
    }
  }
  state.screen_ink_px = screenInk;

  const materials = {};
  for (const [name, spec] of Object.entries(manifest.materials)) {
    materials[name] = makeMaterial(spec);
  }

  //  One group per layer, so the exploded view moves whole layers and not
  //  individual bodies. The layer order is the manifest's.
  const groups = {};
  for (const l of manifest.layers) {
    groups[l] = new THREE.Group();
    //  The one reserved layer name. render.py's --break blank moves every
    //  body into it, which is how the degeneracy check gets something to
    //  fail against; nothing else ever uses it.
    if (l === '__hidden__') groups[l].visible = false;
    scene.add(groups[l]);
  }

  const loader = new STLLoader();
  const loaded = [];
  //  One mesh file, one geometry, however many instances the manifest places.
  //  Six knobs are two parts and eight placements; four feet are one part and
  //  four. Loading knob_encoder.stl three times would be three copies of the
  //  same triangles in GPU memory and three chances for them to differ.
  const geomCache = new Map();
  for (const b of manifest.bodies) {
    let geom = geomCache.get(b.stl);
    if (!geom) {
      geom = await loader.loadAsync('/' + b.stl);
      geom.computeVertexNormals();
      if (screen && screen.body === b.id) applyScreenUV(geom, screen.box);
      geomCache.set(b.stl, geom);
    }
    const mesh = new THREE.Mesh(geom, materials[b.material]
                                      ?? materials.component_satin);
    //  The matrix is the manifest's, applied whole. Nothing is re-derived
    //  here: no position, no rotation, no scale.
    mesh.matrixAutoUpdate = false;
    mesh.matrix.fromArray(b.matrix);
    mesh.userData = b;
    (groups[b.layer] ?? groups[manifest.layers[0]]).add(mesh);
    loaded.push({ body: b, mesh });
  }
  state.loaded = loaded.map(l => l.body.id);

  //  The exploded stack. Layer index 0 is the back panel and it goes down;
  //  the acrylic goes up; the carrier and everything mounted to it stay put,
  //  because the components are fastened to the carrier and an explosion that
  //  separates a board from the bosses it is screwed to is a picture of an
  //  assembly nobody is going to build.
  //  `under` is the cowl and the four feet: they are screwed to the back
  //  panel, so they go down WITH it and stay in contact. `fitted` is the six
  //  knobs and the fader cap: they go up with the acrylic, because a O25.400
  //  brim cannot pass through a O22.000 bore and an explosion that slides the
  //  slab off past the knobs is a picture of an assembly nobody can build.
  const EXPLODE_DIR = { back: -1, under: -1, carrier: 0, components: 0,
                        acrylic: +1, fitted: +1 };

  state.render = (view) => {
    const w = view.width, h = view.height, dpr = view.dpr ?? 1;
    renderer.setPixelRatio(dpr);
    renderer.setSize(w, h, false);
    canvas.style.width = w + 'px';
    canvas.style.height = h + 'px';

    //  materials
      const mask = view.materials === 'mask';
    scene.background = mask ? new THREE.Color(0x000000) : sweep;
    //  A measurement view must be BINARY. MeshBasicMaterial is unlit, so the
    //  environment is irrelevant to it, but ACES filmic would map pure white
    //  to about 0.86 and put a gradient on an edge that has to be a step --
    //  so tone mapping comes off and the IBL goes with it.
    scene.environment = mask ? null : ibl.texture;
    renderer.toneMapping = mask ? THREE.NoToneMapping
                                : THREE.ACESFilmicToneMapping;
    for (const { body, mesh } of loaded) {
      mesh.material = mask ? MASK_MATERIAL
                           : (materials[body.material]
                              ?? materials.component_satin);
    }

    //  visibility: hide by layer name, by body id, or by provenance
    const hide = new Set(view.hide ?? []);
    const only = view.only ? new Set(view.only) : null;
    for (const { body, mesh } of loaded) {
      let vis = !(hide.has(body.layer) || hide.has(body.id)
                  || hide.has(body.provenance));
      if (only) vis = only.has(body.id) || only.has(body.layer);
      mesh.visible = vis;
    }

    //  explode
    const ex = view.explode ?? 0;
    for (const l of manifest.layers) {
      groups[l].position.set(0, 0, (EXPLODE_DIR[l] ?? 0) * ex);
    }

    //  camera
    const { dir, up } = orbit(view.azimuth, view.elevation);
    const target = centre.clone();
    //  framing follows the exploded extent, not the compact one, or the
    //  exploded view would crop its own top and bottom layer
    const exRadius = radius + Math.abs(ex);
    let cam;
    if (view.projection === 'orthographic') {
      const halfW = view.ortho_w / 2;
      const halfH = halfW * h / w;
      cam = new THREE.OrthographicCamera(-halfW, halfW, halfH, -halfH,
                                         0.01, exRadius * 20);
      cam.position.copy(target).addScaledVector(dir, exRadius * 6);
    } else {
      cam = new THREE.PerspectiveCamera(view.fov, w / h, 1, exRadius * 40);
      //  fit the sphere, then back off a little so the frame has air
      //  0.72, not 0.80: the fit is on the sphere through the scene's
      //  corners and the deck is a flat 8:5 plate, so the sphere is mostly
      //  air. [ARTISTIC] -- it is framing, and it moves no measurement: every
      //  checked view is orthographic with its frustum stated in millimetres.
      const d = (exRadius / Math.sin(view.fov * DEG / 2)) * 0.72;
      cam.position.copy(target).addScaledVector(dir, d);
    }
    cam.up.copy(up);
    cam.lookAt(target);
    cam.updateProjectionMatrix();
    state.camera = {
      projection: view.projection,
      position: cam.position.toArray(), up: up.toArray(),
      target: target.toArray(),
      ortho_w: view.ortho_w ?? null,
      //  mm per DEVICE pixel, which is the number a check needs
      mm_per_px: view.projection === 'orthographic'
                   ? view.ortho_w / (w * dpr) : null,
    };

    annotate(view, manifest, loaded, state.camera);
    renderer.render(scene, cam);
    return state.camera;
  };

  state.dispose = () => { ibl.dispose(); renderer.dispose(); };
  state.ready = true;
}

// ---------------------------------------------------------------------------
//  ANNOTATION. DOM, deliberately: it is a caption on a photograph, and it must
//  not be possible to mistake it for part of the model.
//
//  The legend's job is the one the task set: a body that is EXPORTED is the
//  part the fabricator gets, a body that is REFERENCE or VENDOR is a model of
//  something bought, and a body that is ABSENT is a labelled hole. A render
//  that mixes those silently is the thing that makes people trust a picture of
//  a part that does not exist.
// ---------------------------------------------------------------------------
function annotate(view, manifest, loaded, camera) {
  const el = document.getElementById('ann');
  if (!view.annotate) { el.hidden = true; el.innerHTML = ''; return; }
  el.hidden = false;
  const vis = loaded.filter(l => l.mesh.visible);
  const by = {};
  for (const { body } of vis) {
    (by[body.provenance] ??= []).push(body);
  }
  const K = manifest.params;
  const row = (cls, k, txt) =>
    `<li><span class="k ${cls}">${k}</span>${txt}</li>`;
  const sect = (cls, k, title, items) => items.length === 0 ? '' :
    `<h4>${title}</h4><ul>${items.map(t => row(cls, k, t)).join('')}</ul>`;

  //  EXPORTED is now fifteen bodies and eight distinct parts -- six knobs are
  //  two parts, four feet are one. Fifteen rows would bury the three plates,
  //  so the list is BY PART with its placement count, which is also the
  //  number a reader can check against a bill of materials.
  const parts = new Map();
  for (const b of (by.EXPORTED ?? [])) {
    const name = b.part ?? b.id;
    const e = parts.get(name) ?? { n: 0, b };
    e.n += 1;
    parts.set(name, e);
  }
  const exported = [...parts.entries()].map(([name, e]) => {
    const size = e.b.thickness_mm ? e.b.thickness_mm.toFixed(3) + ' mm'
               : e.b.height_mm ? e.b.height_mm.toFixed(3) + ' h'
               : '';
    return (e.n > 1 ? e.n + '× ' : '').padStart(4)
         + name.padEnd(13) + size.padStart(10) + '  ' + e.b.material;
  });

  //  ABSENT CARRIES ITS SIZE ONTO THE IMAGE, not only into the sidecar. The
  //  claim this tool makes about itself is that a thing nobody models is
  //  listed WITH ITS SIZE; for a while the sidecars did that and the pictures
  //  printed "6x knob", which is not the same claim.
  const absent = manifest.absent.map(
    a => `${a.count}× ${a.id}${a.size ? ' — ' + a.size : ''}`);

  el.innerHTML =
    gridOverlay(view, manifest, camera) +
    `<div class="cap">${view.caption || ''}` +
    `<div class="sub">${manifest.generated} &middot; three r${manifest.three}` +
    ` &middot; deck ${K.body_w.toFixed(3)} &times; ${K.body_h.toFixed(3)}` +
    ` &times; ${K.deck_t.toFixed(3)} mm` +
    `${view.explode ? ` &middot; exploded ${view.explode} mm` : ''}` +
    `</div></div>` +
    `<div class="pad">` +
    sect('exported', '■', 'exported geometry — this is the part',
         exported) +
    `</div>` +
    `<div class="legend">` +
    sect('reference', '□', 'reference bodies — v2/cad/parts',
         [...new Set((by.REFERENCE ?? []).map(b =>
            b.source.split(' ')[0].split('/').pop()))]) +
    sect('vendor', '△', 'vendor meshes — house asset library',
         [...new Set((by.VENDOR ?? []).map(b => b.id.split('@')[0]))]) +
    sect('absent', '×', 'absent — modelled by nothing, so drawn by '
                             + 'nothing', absent) +
    `</div>` +
    screenNote(manifest);
}

//  Where the glyphs on the panel came from, printed next to the picture that
//  shows them. The screen is the one surface in this render that could be
//  made to say anything at all, so its provenance travels with it.
function screenNote(manifest) {
  const sc = manifest.screen;
  if (!sc || !sc.body) return '';
  return `<div class="screen"><h4>the panel</h4>`
       + `<div>${sc.px[0]} × ${sc.px[1]} px &middot; ${sc.bits} bit`
       + ` &middot; reflective, no backlight &middot; `
       + `${sc.pitch_mm[0].toFixed(4)} mm pitch</div>`
       + `<div class="src">${sc.source}</div></div>`;
}

// ---------------------------------------------------------------------------
//  THE GRID. "a slave to symmetry and the swiss grid ... mathematically
//  flawless ratios between the grids" is the brief, and the annotated plan is
//  the only place that argument can be made visible rather than asserted.
//
//  THE GRID IS NOT CHOSEN HERE. The deck is 304.600 x 190.375, which is 8:5
//  exactly, so at that aspect there is exactly one square module that divides
//  both sides a whole number of times: body_w/8 and body_h/5. This function
//  COMPUTES BOTH AND PRINTS THEM, and if they ever disagree it draws no grid
//  and prints the disagreement instead -- a grid that silently rounded would
//  be an argument for symmetry made by a picture that does not have it.
//
//  Orthographic only, and that is not a limitation to apologise for: a grid
//  over a perspective view is a grid in the image plane and not on the part,
//  which is the sort of decoration this annotation layer exists not to be.
//  The exploded view is perspective and gets none.
// ---------------------------------------------------------------------------
function gridOverlay(view, manifest, camera) {
  if (!camera || view.projection !== 'orthographic') return '';
  const K = manifest.params;
  const NX = 8, NY = 5;
  const mx = K.body_w / NX, my = K.body_h / NY;
  const W = view.width, H = view.height;
  const mm = view.ortho_w / W;            // mm per CSS pixel
  const cx = camera.target[0], cy = camera.target[1];
  const X = (x) => (W / 2 + (x - cx) / mm).toFixed(2);
  const Y = (y) => (H / 2 - (y - cy) / mm).toFixed(2);
  const x0 = -K.body_w / 2, y0 = -K.body_h / 2;
  const square = Math.abs(mx - my) < 1e-9;
  let lines = '';
  if (square) {
    for (let i = 0; i <= NX; i++) {
      const x = x0 + i * mx;
      lines += `<line x1="${X(x)}" y1="${Y(y0)}" x2="${X(x)}" `
             + `y2="${Y(y0 + NY * my)}" class="${i % NX === 0 ? 'ge' : 'g'}"/>`;
    }
    for (let j = 0; j <= NY; j++) {
      const y = y0 + j * my;
      lines += `<line x1="${X(x0)}" y1="${Y(y)}" x2="${X(x0 + NX * mx)}" `
             + `y2="${Y(y)}" class="${j % NY === 0 ? 'ge' : 'g'}"/>`;
    }
    //  The two centre lines are NOT grid lines, and they are drawn apart from
    //  the grid because 8 and 5 do not behave the same way: an even count
    //  puts a line on the X centre and an odd one puts the Y centre in the
    //  middle of a row. That asymmetry is real and worth being able to see.
    lines += `<line x1="${X(0)}" y1="${Y(y0)}" x2="${X(0)}" `
           + `y2="${Y(-y0)}" class="gc"/>`;
    lines += `<line x1="${X(x0)}" y1="${Y(0)}" x2="${X(-x0)}" `
           + `y2="${Y(0)}" class="gc"/>`;
  }
  const label = square
    ? `${NX} &times; ${NY} of ${mx.toFixed(3)} mm &nbsp; `
      + `body_w/${NX} = body_h/${NY} = ${mx.toFixed(6)}`
    : `NO SQUARE MODULE: body_w/${NX} = ${mx.toFixed(6)}, `
      + `body_h/${NY} = ${my.toFixed(6)}`;
  return `<svg class="grid" width="${W}" height="${H}">${lines}</svg>`
       + `<div class="gridlab${square ? '' : ' bad'}">${label}</div>`;
}

boot().catch(e => {
  state.error = (e && e.stack) ? e.stack : String(e);
  state.ready = true;
});
