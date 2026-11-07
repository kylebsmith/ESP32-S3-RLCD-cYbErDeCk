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
import { RoomEnvironment } from '/scene/vendor/RoomEnvironment.js';

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
//  LIGHTING. A neutral studio, not a CAD viewport.
//
//  The dominant term is an IBL: three.js's own RoomEnvironment through
//  PMREMGenerator. It is a neutral grey room with broad area sources, which
//  is what makes a metal read as metal and -- critically -- it is the only
//  thing a `transmission` material can refract, since there is nothing else
//  behind the slab at the edges of the frame. Point lights alone would give
//  clear acrylic nothing to show.
//
//  Then three conventional lights, all ARTISTIC, for shape: a key high and
//  camera-left, a cool fill opposite at a fifth of the key, and a rim from
//  behind to catch the slab's 12.700 mm edge -- that edge is the one place the
//  thickness is directly visible and the brief's whole reason for the
//  half-inch sheet, so it gets its own light.
// ---------------------------------------------------------------------------
function buildLights(scene, renderer, radius) {
  const pmrem = new THREE.PMREMGenerator(renderer);
  pmrem.compileEquirectangularShader();
  const env = pmrem.fromScene(new RoomEnvironment(renderer), 0.04);
  scene.environment = env.texture;

  const key = new THREE.DirectionalLight(0xfff6ec, 1.45);
  key.position.set(-0.55, -0.70, 1.00).multiplyScalar(radius * 3);
  scene.add(key);

  const fill = new THREE.DirectionalLight(0xdfeaff, 0.30);
  fill.position.set(0.85, 0.35, 0.45).multiplyScalar(radius * 3);
  scene.add(fill);

  //  The rim exists for ONE feature: the slab's 12.700 mm edge. That edge is
  //  the only place the thickness is directly visible and it is the brief's
  //  whole reason for the half-inch sheet, so it gets a light of its own,
  //  raked along the rear edge at a glancing angle.
  const rim = new THREE.DirectionalLight(0xffffff, 1.05);
  rim.position.set(0.10, 1.00, 0.22).multiplyScalar(radius * 3);
  scene.add(rim);

  return { texture: env.texture,
           dispose: () => { env.dispose?.(); pmrem.dispose(); } };
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
  //  [ARTISTIC] 0.62. At 1.05 the first run came out milky: the carrier is
  //  specified as "matte black or very dark grey" (v2/parts/stack.md 5.6) and
  //  it rendered mid-grey, which destroyed the contrast the brief is actually
  //  asking for -- "a dark matte plane reads as a void, and the coloured
  //  wires and nickel nuts become the only things with brightness".
  renderer.toneMappingExposure = 0.78;

  const scene = new THREE.Scene();

  const [lo, hi] = manifest.scene_bounds;
  const centre = new THREE.Vector3((lo[0] + hi[0]) / 2, (lo[1] + hi[1]) / 2,
                                   (lo[2] + hi[2]) / 2);
  const radius = 0.5 * Math.hypot(hi[0] - lo[0], hi[1] - lo[1], hi[2] - lo[2]);
  state.radius = radius;
  state.centre = centre.toArray();

  const ibl = buildLights(scene, renderer, radius);
  const sweep = backdrop();

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
  for (const b of manifest.bodies) {
    const geom = await loader.loadAsync('/' + b.stl);
    geom.computeVertexNormals();
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
  const EXPLODE_DIR = { back: -1, carrier: 0, components: 0, acrylic: +1 };

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
      const d = (exRadius / Math.sin(view.fov * DEG / 2)) * 0.80;
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

    annotate(view, manifest, loaded);
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
function annotate(view, manifest, loaded) {
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

  el.innerHTML =
    `<div class="cap">${view.caption || ''}` +
    `<div class="sub">${manifest.generated} &middot; three r${manifest.three}` +
    ` &middot; deck ${K.body_w.toFixed(3)} &times; ${K.body_h.toFixed(3)}` +
    ` &times; ${K.deck_t.toFixed(3)} mm` +
    `${view.explode ? ` &middot; exploded ${view.explode} mm` : ''}` +
    `</div></div>` +
    `<div class="pad">` +
    sect('exported', '■', 'exported geometry — this is the part',
         (by.EXPORTED ?? []).map(b =>
           `${b.id.padEnd(10)} ${b.thickness_mm
              ? b.thickness_mm.toFixed(3).padStart(7) + ' mm' : '       '}` +
           `  ${b.material}`)) +
    `</div>` +
    `<div class="legend">` +
    sect('reference', '□', 'reference bodies — v2/cad/parts',
         [...new Set((by.REFERENCE ?? []).map(b =>
            b.source.split(' ')[0].split('/').pop()))]) +
    sect('vendor', '△', 'vendor meshes — house asset library',
         [...new Set((by.VENDOR ?? []).map(b => b.id.split('@')[0]))]) +
    sect('absent', '×', 'absent — modelled by nothing, so drawn by '
                             + 'nothing',
         manifest.absent.map(a => `${a.count}× ${a.id}`)) +
    `</div>`;
}

boot().catch(e => {
  state.error = (e && e.stack) ? e.stack : String(e);
  state.ready = true;
});
