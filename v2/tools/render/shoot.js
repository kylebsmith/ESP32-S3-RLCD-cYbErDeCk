// ===========================================================================
//  v2/tools/render/shoot.js
//  Drives the scene headlessly and writes the PNGs. Called by render.py.
//
//      node v2/tools/render/shoot.js --work <workdir> --scene <scenedir> \
//                                    --out <outdir>
//
//  WHY A SERVER AND NOT file://
//  ----------------------------
//  Chromium refuses fetch() against file:// (opaque origin), and the whole
//  scene is fetch(): the manifest and every STL. So this starts a static
//  server on 127.0.0.1 on an ephemeral port, bound to loopback only, serving
//  exactly two roots and nothing else:
//
//      /scene/*        the scene directory (html, scene.js, vendored three.js)
//      /stl/*          the staged meshes in the work directory
//      /manifest.json  the work directory's manifest
//
//  Any other path is 404. The server lives for the length of this process.
//
//  Playwright and Chromium are PRE-INSTALLED in this environment and the
//  versions are matched. Do not npm-install playwright here and do not run
//  `playwright install`: a different Playwright will not find this browser.
// ===========================================================================
'use strict';
const fs = require('fs');
const http = require('http');
const path = require('path');
const { chromium } = require('playwright');

function arg(name, dflt) {
  const i = process.argv.indexOf('--' + name);
  return i >= 0 && process.argv[i + 1] ? process.argv[i + 1] : dflt;
}
const WORK = path.resolve(arg('work'));
const SCENE = path.resolve(arg('scene'));
const OUT = path.resolve(arg('out'));

const MIME = {
  '.html': 'text/html; charset=utf-8',
  '.js': 'text/javascript; charset=utf-8',
  '.json': 'application/json; charset=utf-8',
  '.stl': 'model/stl',
  '.png': 'image/png',
};

//  Resolve a URL path to a file under one of the two roots, refusing anything
//  that escapes them. A render tool has no business reading the filesystem at
//  large just because a path contained "..".
function resolveUrl(urlPath) {
  const clean = decodeURIComponent(urlPath.split('?')[0]);
  let root, rest;
  if (clean === '/' || clean === '/index.html') {
    root = SCENE; rest = 'index.html';
  } else if (clean.startsWith('/scene/')) {
    root = SCENE; rest = clean.slice('/scene/'.length);
  } else if (clean.startsWith('/stl/')) {
    root = WORK; rest = 'stl/' + clean.slice('/stl/'.length);
  } else if (clean === '/manifest.json') {
    root = WORK; rest = 'manifest.json';
  } else {
    return null;
  }
  const abs = path.resolve(root, rest);
  if (abs !== root && !abs.startsWith(root + path.sep)) return null;
  return abs;
}

function serve() {
  const srv = http.createServer((req, res) => {
    const abs = resolveUrl(req.url);
    if (!abs || !fs.existsSync(abs) || !fs.statSync(abs).isFile()) {
      res.writeHead(404); res.end('no'); return;
    }
    res.writeHead(200, {
      'Content-Type': MIME[path.extname(abs)] || 'application/octet-stream',
      'Cache-Control': 'no-store',
    });
    fs.createReadStream(abs).pipe(res);
  });
  return new Promise(ok => srv.listen(0, '127.0.0.1',
                                      () => ok([srv, srv.address().port])));
}

(async () => {
  const views = JSON.parse(fs.readFileSync(path.join(WORK, 'views.json')));
  const [srv, port] = await serve();
  const base = `http://127.0.0.1:${port}/`;
  fs.mkdirSync(OUT, { recursive: true });

  //  --use-angle=swiftshader is not passed: Chromium picks its own software
  //  GL here and MeshPhysicalMaterial's transmission pass needs a working
  //  WebGL2 context, which it has. If this ever starts failing, the symptom
  //  is an all-background PNG, and check_render.py's degeneracy check is the
  //  thing that catches it rather than a person noticing.
  const browser = await chromium.launch();
  let bad = 0;

  //  One context per device pixel ratio: deviceScaleFactor is a context
  //  property in Playwright, not a page one, so a 1x measurement view and a
  //  2x picture cannot share a context.
  const byDpr = new Map();
  for (const v of views) {
    const dpr = v.dpr ?? 2;
    if (!byDpr.has(dpr)) byDpr.set(dpr, []);
    byDpr.get(dpr).push(v);
  }

  for (const [dpr, group] of byDpr) {
    const ctx = await browser.newContext({ deviceScaleFactor: dpr });
    const page = await ctx.newPage();
    const errs = [];
    page.on('pageerror', e => errs.push('pageerror: ' + e.message));
    page.on('console', m => {
      if (m.type() === 'error') errs.push('console: ' + m.text());
    });
    await page.setViewportSize({ width: group[0].width,
                                 height: group[0].height });
    await page.goto(base, { waitUntil: 'load' });
    try {
      await page.waitForFunction('window.__deck && window.__deck.ready',
                                 null, { timeout: 180000 });
    } catch (e) {
      //  Report what the page said BEFORE throwing. A module that fails to
      //  resolve never runs scene.js at all, so window.__deck is never
      //  assigned and the only evidence is in these two handlers.
      console.log('THE SCENE NEVER BECAME READY. What the page said:');
      if (errs.length === 0) console.log('  (nothing)');
      errs.slice(0, 20).forEach(x => console.log('  ' + x));
      throw e;
    }
    const err = await page.evaluate('window.__deck.error');
    if (err) { console.log('SCENE ERROR\n' + err); bad++; break; }
    const loaded = await page.evaluate('window.__deck.loaded');
    console.log(`dpr ${dpr}: ${loaded.length} meshes loaded`);

    for (const v of group) {
      await page.setViewportSize({ width: v.width, height: v.height });
      const cam = await page.evaluate(
        view => window.__deck.render(view), v);
      //  Two animation frames after the render: the transmission pass runs
      //  inside render(), but the compositor still needs a frame to present
      //  it, and screenshotting too early gives a blank or half-drawn canvas.
      await page.evaluate(() => new Promise(r =>
        requestAnimationFrame(() => requestAnimationFrame(r))));
      const dest = path.join(OUT, v.name + '.png');
      await page.screenshot({ path: dest, type: 'png' });
      const kb = (fs.statSync(dest).size / 1024).toFixed(1);
      console.log(`  ${v.name.padEnd(16)} ${v.width}x${v.height}@${dpr}  ` +
                  `${v.projection.padEnd(13)} az ${String(v.azimuth).padStart(4)}` +
                  ` el ${String(v.elevation).padStart(3)}` +
                  ` ex ${String(v.explode ?? 0).padStart(3)}` +
                  (cam.mm_per_px
                    ? `  ${cam.mm_per_px.toFixed(6)} mm/px` : '') +
                  `  ${kb} kB`);
    }
    if (errs.length) {
      console.log('PAGE ERRORS:'); errs.slice(0, 12).forEach(
        e => console.log('  ' + e));
      bad += errs.length;
    }
    await ctx.close();
  }

  await browser.close();
  srv.close();
  process.exit(bad ? 1 : 0);
})().catch(e => { console.error(e); process.exit(1); });
