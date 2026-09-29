#!/usr/bin/env python3
"""The wiki as one designed page: docs/wiki/*.md -> docs/wiki/book.html.

  python3 tools/wiki_book.py ART_DIR [OUT]

ART_DIR is tools/zine_art.c's output (the deck's own picture engine, frames of
80 x 30 cells), as for the zine. OUT defaults to docs/wiki/book.html.

The book is set on the deck's character grid. The unit is its 12 x 24 cell:
every size is a whole number of cells, and the page is two faces, the deck's own,
built from the firmware's font sources (tools/deck_webfont.py) and carried
inside the file. Sixteen chapters are one bar - the index and fifteen pages - so
the contents are a step sequencer and each chapter is a step. A chapter is
titled with a line you could type, and under it is what the deck says to that
line, quoted from firmware/components/cmd/cmd.c. The cover is a frame from the
engine, set in the deck's tiles, and it turns at 124 bpm.

Swiss in the grid, the flush-left rag and the hierarchy by size alone; punk in
the numerals that overprint, the stepped titles and the black slabs that leave
the column. Paper and ink follow the reader's light setting, like the panel
and the HDMI node.
"""
import base64
import html
import os
import re
import sys
import tempfile

import markdown

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(HERE)
WIKI = os.path.join(REPO, 'docs', 'wiki')
sys.path.insert(0, HERE)
import deck_extra      # noqa: E402
import deck_webfont    # noqa: E402

ORDER = ['README', 'language', 'verbs', 'combinations', 'pictures', 'editor', 'outputs',
         'documents', 'system', 'tools', 'errata', 'research', 'completion',
         'pictures-and-type', 'next']
VERBS = {'help', 'lanes', 'list', 'dump', 'new', 'name', 'open', 'play', 'stop', 'map',
         'clear', 'toggle', 'mute', 'solo', 'send', 'route', 'split', 'save', 'close', 'run'}
TILE = 0xE000


def slugify(value, sep='-'):
    """GitHub's anchor rule: lower case, drop what is not a word, space or hyphen."""
    v = re.sub(r'<[^>]+>', '', value).strip().lower()
    v = re.sub(r'[^\w\- ]', '', v)
    return v.replace(' ', sep)


def cells_text(path):
    """An engine frame as text: codes 32-126 as themselves, tiles to the PUA."""
    d = open(path, 'rb').read()
    w, h = d[0], d[1]
    rows = []
    for y in range(h):
        row = d[2 + y * w: 2 + (y + 1) * w]
        rows.append(''.join(chr(TILE + c) if c >= 128 else (chr(c) if c >= 32 else ' ')
                            for c in row))
    return '\n'.join(html.escape(r) for r in rows)


def page_html(slug, text):
    """One page: its title and standfirst, and the body as HTML on the grid."""
    for k, v in deck_extra.SPELL.items():
        text = text.replace(k, v)
    lines = text.split('\n')
    title = lines[0].lstrip('# ').strip() if lines and lines[0].startswith('# ') else slug
    body = '\n'.join(lines[1:])
    stand = ''
    m = re.match(r'\s*\*([^\n]+(?:\n[^\n]+)*?)\*\s*\n\n', body)
    if m:
        stand = m.group(1)
        body = body[m.end():]
    md = markdown.Markdown(extensions=['tables', 'fenced_code', 'sane_lists', 'toc'],
                           extension_configs={'toc': {'slugify': slugify}})
    h = md.convert(body)
    stand_h = markdown.markdown(stand) if stand else ''
    # Every id and every link belongs to this chapter or points at another.
    h = re.sub(r'id="([^"]+)"', lambda m: 'id="%s--%s"' % (slug, m.group(1)), h)
    def link(m):
        href = m.group(1)
        mm = re.match(r'^([\w-]+)\.md(?:#(.*))?$', href)
        if mm and mm.group(1) in ORDER:
            target = mm.group(1) if mm.group(1) != 'README' else 'README'
            return 'href="#%s%s"' % (target, ('--' + mm.group(2)) if mm.group(2) else '')
        if href.startswith('#'):
            return 'href="#%s--%s"' % (slug, href[1:])
        return 'href="%s"' % href
    h = re.sub(r'href="([^"]+)"', link, h)
    stand_h = re.sub(r'href="([^"]+)"', link, stand_h)
    # Code wider than the column leaves it: a black slab out to the page's edge.
    def pre(m):
        code = m.group(2)
        widest = max((len(html.unescape(l)) for l in code.split('\n')), default=0)
        return '<pre class="slab%s"><code%s>%s</code></pre>' % (
            ' wide' if widest > 60 else '', m.group(1) or '', code)
    h = re.sub(r'<pre><code([^>]*)>(.*?)</code></pre>', pre, h, flags=re.S)
    h = h.replace('<table>', '<div class="tab"><table>').replace('</table>', '</table></div>')
    return title, stand_h, h


def fonts_css():
    tmp = tempfile.mkdtemp(prefix='deckfont_')
    big = {c: deck_webfont.bitmap_rows(g, 12) for c, g in
           deck_webfont.load_face(deck_webfont.BIG_SRC, 12, 24).items()}
    small = {c: deck_webfont.bitmap_rows(g, 6) for c, g in
             deck_webfont.load_face(deck_webfont.SMALL_SRC, 6, 12).items()}
    deck_webfont.build('Deck', big, 12, 24, 20, deck_extra.EXTRA, os.path.join(tmp, 'd.ttf'))
    deck_webfont.build('Deck Small', small, 6, 12, 9, deck_extra.SMALL, os.path.join(tmp, 's.ttf'))
    out = []
    for fam, f in (('Deck', 'd.ttf'), ('Deck Small', 's.ttf')):
        b = base64.b64encode(open(os.path.join(tmp, f), 'rb').read()).decode()
        out.append("@font-face{font-family:'%s';src:url(data:font/ttf;base64,%s) format('truetype');"
                   "font-display:block}" % (fam, b))
    return '\n'.join(out)


CSS = r"""
:root{
  --cell:12px; --line:24px;
  --paper:#ecebe4; --ink:#121212; --red:#e3001b; --mute:#6d6b66; --rule:#121212;
  --slab:#121212; --slab-ink:#ecebe4;
  color-scheme: light;
}
@media (prefers-color-scheme: dark){
  :root:not([data-theme="light"]){
    --paper:#0f0f0f; --ink:#e9e7df; --red:#ff2d2d; --mute:#8d8a83; --rule:#e9e7df;
    --slab:#1d1d1d; --slab-ink:#e9e7df; color-scheme: dark;
  }
}
:root[data-theme="dark"]{
  --paper:#0f0f0f; --ink:#e9e7df; --red:#ff2d2d; --mute:#8d8a83; --rule:#e9e7df;
  --slab:#1d1d1d; --slab-ink:#e9e7df; color-scheme: dark;
}
*{box-sizing:border-box}
html{background:var(--paper)}
body{margin:0;background:var(--paper);color:var(--ink);
  font-family:'Deck',monospace;font-size:24px;line-height:36px;
  -webkit-font-smoothing:none;font-smooth:never;text-rendering:optimizeSpeed;
  font-variant-ligatures:none;hyphens:none;overflow-x:hidden}
a{color:inherit;text-decoration:underline;text-decoration-color:var(--red);
  text-decoration-thickness:2px;text-underline-offset:6px}
a:hover{background:var(--red);color:var(--paper);text-decoration:none}
strong,b{font-weight:normal;text-shadow:1px 0 0 currentColor}
em,i{font-style:normal;color:var(--red)}
code{font-family:inherit}
p code,li code,td code,h2 code,h3 code,h4 code{
  background:linear-gradient(transparent 4px,var(--tone) 4px,var(--tone) 32px,transparent 32px);
  padding:0 2px}
:root{--tone:rgba(227,0,27,.12)}
@media (prefers-color-scheme: dark){:root:not([data-theme="light"]){--tone:rgba(255,45,45,.22)}}
:root[data-theme="dark"]{--tone:rgba(255,45,45,.22)}

/* ---------------------------------------------------------------- mast */
.mast{position:sticky;top:0;z-index:20;display:grid;
  grid-template-columns:auto 1fr auto;align-items:center;gap:var(--line);
  height:48px;padding:0 var(--line);background:var(--paper);border-bottom:2px solid var(--rule);
  font-family:'Deck Small';font-size:12px;line-height:12px;letter-spacing:0}
.mast .name{font-family:'Deck';font-size:24px;line-height:24px}
.mast .name b{color:var(--red);text-shadow:none}
.bar{display:grid;grid-template-columns:repeat(16,1fr);gap:2px;height:24px}
.bar a{display:block;border:2px solid var(--rule);text-decoration:none;position:relative}
.bar a.on{background:var(--red);border-color:var(--red)}
.bar a.beat{border-width:2px 2px 2px 6px}
.bar a:hover{background:var(--ink)}
.bar a span{position:absolute;left:0;top:28px;white-space:nowrap;display:none;
  background:var(--ink);color:var(--paper);padding:4px 6px}
.bar a:hover span{display:block}
.tools button{font:inherit;font-size:12px;background:none;border:2px solid var(--rule);
  color:var(--ink);padding:4px 6px;cursor:pointer;margin-left:6px}
.tools button:hover{background:var(--ink);color:var(--paper)}

/* the running head, up the left edge */
.spine{position:fixed;left:0;bottom:0;z-index:10;transform-origin:0 0;
  transform:rotate(-90deg) translate(0,12px);white-space:nowrap;
  font-family:'Deck Small';font-size:12px;line-height:12px;color:var(--mute);
  padding-left:var(--line)}
@media (max-width:1100px){.spine{display:none}}

/* ---------------------------------------------------------------- cover */
.cover{position:relative;isolation:isolate;min-height:calc(100vh - 48px);padding:48px 48px 0 72px;
  display:grid;grid-template-columns:minmax(0,1fr) minmax(0,1fr);gap:48px;
  border-bottom:8px solid var(--rule);overflow:hidden}
.cover h1{margin:0;font-weight:normal;font-size:240px;line-height:240px;letter-spacing:0;
  position:relative;z-index:2}
.cover h1 span{display:block}
.cover h1 span+span{margin-left:120px}
.cover .step{font-size:96px;line-height:96px;color:var(--red);margin:48px 0 0 0}
.cover .step span{display:block}
.cover .step span:nth-child(2){margin-left:96px}
.cover .step span:nth-child(3){margin-left:192px}
.cover .blurb{max-width:36ch;margin:48px 0 0 0;font-size:24px;line-height:36px}
.cover .facts{font-family:'Deck Small';font-size:12px;line-height:18px;color:var(--mute);
  margin-top:24px;max-width:60ch}
.frame{font-family:'Deck';font-size:12px;line-height:12px;white-space:pre;margin:0;
  color:var(--ink);position:relative}
.cover .art{align-self:start;justify-self:end;position:relative;margin-top:24px}
.cover .art .frame{font-size:12px;line-height:12px}
@media (min-width:1500px){.cover .art .frame{font-size:18px;line-height:18px}}
.cover .art .cap{font-family:'Deck Small';font-size:12px;line-height:12px;margin-top:12px;color:var(--mute)}
.cover .art::before{content:'';position:absolute;inset:-24px -24px auto auto;width:96px;height:96px;
  background:var(--red);mix-blend-mode:multiply;z-index:-1}
.tones{position:absolute;left:0;right:0;bottom:0;height:48px;font-size:48px;line-height:48px;
  white-space:nowrap;overflow:hidden;color:var(--ink)}

/* the contents: a bar of sixteen steps */
.index{display:grid;grid-template-columns:repeat(16,minmax(0,1fr));border-bottom:8px solid var(--rule)}
.index a{display:flex;flex-direction:column;justify-content:space-between;height:360px;
  padding:12px;border-right:2px solid var(--rule);text-decoration:none;position:relative;overflow:hidden}
.index a:nth-child(4n+1){border-left:0}
.index a:nth-child(4n){border-right-width:6px}
.index a .n{font-size:48px;line-height:48px}
.index a .t{writing-mode:vertical-rl;transform:rotate(180deg);font-family:'Deck Small';
  font-size:24px;line-height:24px;align-self:flex-start;white-space:nowrap}
.index a:hover,.index a:focus{background:var(--red);color:var(--paper)}
.index a:nth-child(1) .n{color:var(--red)}
.index a:nth-child(1):hover .n{color:var(--paper)}
@media (max-width:900px){.index{grid-template-columns:repeat(4,1fr)}.index a{height:200px}}

/* ---------------------------------------------------------------- a chapter */
.page{position:relative;padding:0 0 144px 0;border-bottom:8px solid var(--rule)}
.head{position:relative;padding:96px 48px 48px 312px;min-height:480px;overflow:hidden}
.head .num{position:absolute;left:-36px;top:-12px;font-size:432px;line-height:432px;
  color:var(--red);mix-blend-mode:multiply;z-index:0;pointer-events:none;letter-spacing:-24px}
@media (prefers-color-scheme: dark){:root:not([data-theme="light"]) .head .num{mix-blend-mode:screen}}
:root[data-theme="dark"] .head .num{mix-blend-mode:screen}
.head h1{position:relative;z-index:1;margin:0;font-weight:normal;font-size:96px;line-height:96px}
.head h1 span{display:block}
.head .cmd{position:relative;z-index:1;margin:48px 0 0 0;font-size:48px;line-height:48px}
.head .reply{position:relative;z-index:1;margin:12px 0 0 0;font-family:'Deck Small';
  font-size:24px;line-height:24px;color:var(--mute)}
.head .reply .t{color:var(--red)}
.head .title{position:relative;z-index:1;margin:48px 0 0 0;max-width:40ch;font-size:24px;line-height:36px}
.head .stand{position:relative;z-index:1;margin:24px 0 0 0;max-width:62ch;font-family:'Deck Small';
  font-size:12px;line-height:18px;color:var(--mute)}
.head .stand p{margin:0}
.head .steps{position:relative;z-index:1;margin-top:48px;font-size:24px;line-height:24px;letter-spacing:0}
.head .steps b{color:var(--red);text-shadow:none}

.body{padding:0 48px 0 312px;max-width:calc(312px + 66 * var(--cell) + 48px)}
.body > *{max-width:64ch}
.body p,.body ul,.body ol{margin:0 0 var(--line) 0}
.body ul,.body ol{padding-left:36px}
.body li{margin:0 0 12px 0}
.body ul>li{list-style:none;position:relative}
.body ul>li::before{content:'';position:absolute;left:-30px;top:14px;width:12px;height:12px;background:var(--red)}
.body ol{list-style:decimal-leading-zero}
.body ol>li::marker{color:var(--red)}
.body h2{position:relative;margin:144px 0 48px 0;padding-top:24px;border-top:8px solid var(--rule);
  font-weight:normal;font-size:48px;line-height:48px;max-width:none}
.body h2::before{content:'';position:absolute;left:-312px;right:-96px;top:-8px;height:8px;background:var(--rule)}
.body h3{float:left;clear:left;width:216px;margin:6px 0 24px -264px;font-weight:normal;
  font-family:'Deck Small';font-size:24px;line-height:24px;color:var(--red)}
.body h4{margin:48px 0 12px 0;font-weight:normal;font-size:24px;line-height:36px;color:var(--red)}
.body h3 + p,.body h3 + ul,.body h3 + ol{margin-top:0}
.body hr{border:0;height:24px;margin:48px 0;max-width:none;
  background:repeating-linear-gradient(90deg,var(--ink) 0 12px,transparent 12px 48px);
  -webkit-mask:linear-gradient(transparent 8px,#000 8px,#000 16px,transparent 16px);
  mask:linear-gradient(transparent 8px,#000 8px,#000 16px,transparent 16px)}
.body blockquote{margin:0 0 var(--line) 0;padding:0 0 0 24px;border-left:8px solid var(--red)}
.slab{position:relative;margin:0 -48px var(--line) -24px;padding:24px 48px 24px 24px;
  background:var(--slab);color:var(--slab-ink);font-size:24px;line-height:24px;
  white-space:pre;overflow-x:auto;max-width:none;border-left:8px solid var(--red)}
.slab.wide{width:calc(100vw - 288px - 24px)}
.slab code{background:none;padding:0}
.tab{margin:0 0 var(--line) 0;overflow-x:auto;max-width:none;width:calc(100vw - 312px - 48px)}
.body table{border-collapse:collapse;font-family:'Deck Small';font-size:24px;line-height:24px;
  max-width:1400px;min-width:min(100%,64ch)}
.body th{text-align:left;font-weight:normal;color:var(--paper);background:var(--ink);padding:6px 12px;
  vertical-align:bottom;white-space:nowrap}
.body td{vertical-align:top;padding:12px 12px;border-bottom:2px solid var(--rule)}
.body td:first-child{white-space:nowrap}
.body tr:hover td{background:var(--tone)}
.specimens{display:grid;grid-template-columns:repeat(4,max-content);gap:24px 24px;margin:0 0 48px 0;max-width:none}
.specimens figure{margin:0}
.specimens .frame{font-size:6px;line-height:6px;border:2px solid var(--rule);padding:6px}
.specimens figcaption{font-family:'Deck Small';font-size:12px;line-height:12px;margin-top:6px}
@media (min-width:1600px){.specimens .frame{font-size:8px;line-height:8px}}

/* the grid, shown: the deck's own cells */
body.grid::after{content:'';position:fixed;inset:0;pointer-events:none;z-index:50;
  background-image:linear-gradient(90deg,rgba(227,0,27,.18) 1px,transparent 1px),
    linear-gradient(rgba(227,0,27,.18) 1px,transparent 1px);
  background-size:12px 24px}

.colophon{padding:96px 48px 144px 312px;font-family:'Deck Small';font-size:24px;line-height:24px}
.colophon p{max-width:60ch;margin:0 0 24px 0}
.colophon .big{font-family:'Deck';font-size:96px;line-height:96px;margin-bottom:48px}

/* ---------------------------------------------------------------- narrow */
@media (max-width:1100px){
  .head{padding:72px 24px 36px 24px;min-height:0}
  .head .num{font-size:240px;line-height:240px;left:auto;right:-24px;top:-24px}
  .head h1{font-size:72px;line-height:72px}
  .body{padding:0 24px}
  .body h2::before{left:-24px;right:-24px}
  .body h3{float:none;width:auto;margin:36px 0 12px 0}
  .slab,.slab.wide{margin:0 -24px var(--line) -24px;width:auto}
  .tab{width:calc(100vw - 48px)}
  .colophon{padding:72px 24px}
  .cover{grid-template-columns:1fr;padding:36px 24px 72px 24px}
  .cover h1{font-size:144px;line-height:144px}
  .cover h1 span+span{margin-left:72px}
  .cover .art{justify-self:start}
}
@media (max-width:640px){
  body{font-size:24px;line-height:36px}
  .mast{grid-template-columns:auto 1fr;height:auto;padding:12px 16px;gap:12px}
  .mast .tools{display:none}
  .head{padding:48px 16px 24px 16px}
  .head h1{font-size:48px;line-height:48px}
  .head .cmd{font-size:24px;line-height:24px;margin-top:24px}
  .head .num{font-size:144px;line-height:144px}
  .body{padding:0 16px}
  .body h2{font-size:24px;line-height:36px}
  .slab,.slab.wide{margin:0 -16px var(--line) -16px;width:auto}
  .tab{width:calc(100vw - 32px)}
  .cover{padding:24px 16px 72px 16px}
  .cover h1{font-size:96px;line-height:96px}
  .cover h1 span+span{margin-left:48px}
  .cover .step{font-size:48px;line-height:48px}
  .cover .step span:nth-child(2){margin-left:48px}
  .cover .step span:nth-child(3){margin-left:96px}
  .cover .art .frame{font-size:6px;line-height:6px}
  .specimens{grid-template-columns:repeat(2,max-content)}
  .specimens .frame{font-size:4px;line-height:4px}
}
@media print{
  .mast,.spine,.tools{display:none}
  .page{break-before:page}
}
"""

JS = r"""
(function(){
  var root=document.documentElement, body=document.body;
  function store(k,v){try{localStorage.setItem(k,v)}catch(e){}}
  function load(k){try{return localStorage.getItem(k)}catch(e){return null}}
  var t=load('deck-theme'); if(t) root.setAttribute('data-theme',t);
  document.getElementById('ink').addEventListener('click',function(){
    var dark=root.getAttribute('data-theme')==='dark'||(!root.getAttribute('data-theme')&&
      matchMedia('(prefers-color-scheme: dark)').matches);
    var next=dark?'light':'dark'; root.setAttribute('data-theme',next); store('deck-theme',next);
  });
  function grid(){body.classList.toggle('grid')}
  document.getElementById('grid').addEventListener('click',grid);
  addEventListener('keydown',function(e){
    if(e.key==='g'&&!e.metaKey&&!e.ctrlKey&&!/INPUT|TEXTAREA/.test(e.target.tagName)) grid();
  });
  /* the step you are on lights, as the playhead does */
  var cells=[].slice.call(document.querySelectorAll('.bar a'));
  var pages=[].slice.call(document.querySelectorAll('[data-step]'));
  var io=new IntersectionObserver(function(es){
    es.forEach(function(e){
      if(!e.isIntersecting) return;
      var n=+e.target.getAttribute('data-step');
      cells.forEach(function(c,i){c.classList.toggle('on',i===n)});
    });
  },{rootMargin:'-45% 0px -50% 0px'});
  pages.forEach(function(p){io.observe(p)});
  /* the cover turns: the orbit's frames at 124 bpm, a sixteenth each */
  var frames=[].slice.call(document.querySelectorAll('.cover .frame'));
  if(frames.length>1 && !matchMedia('(prefers-reduced-motion: reduce)').matches){
    var k=0; setInterval(function(){
      frames[k].hidden=true; k=(k+1)%frames.length; frames[k].hidden=false;
    }, 60000/124/4);
  }
})();
"""


def main():
    art = sys.argv[1]
    out = sys.argv[2] if len(sys.argv) > 2 else os.path.join(WIKI, 'book.html')
    chapters = []
    for i, slug in enumerate(ORDER):
        text = open(os.path.join(WIKI, slug + '.md')).read()
        title, stand, body = page_html(slug, text)
        chapters.append((i, slug, title, stand, body))

    def name(slug):
        return 'index' if slug == 'README' else slug

    bar = ''.join('<a href="#%s"%s><span>%02d %s</span></a>' % (
        slug, ' class="beat"' if i % 4 == 0 else '', i, name(slug))
        for i, slug, *_ in chapters)
    index = ''.join('<a href="#%s"><span class="n">%02d</span><span class="t">%s</span></a>' % (
        slug, i, name(slug)) for i, slug, *_ in chapters)
    orbit = sorted(f for f in os.listdir(art) if f.startswith('orbit-'))
    frames = ''.join('<pre class="frame" aria-hidden="true"%s>%s</pre>' % (
        '' if k == 0 else ' hidden', cells_text(os.path.join(art, f))) for k, f in enumerate(orbit))
    tones = ''.join(chr(TILE + 128 + t) * 6 for t in range(9)) * 4
    specs = ''
    names = ['echo', 'move', 'spin', 'warp', 'noise', 'disc', 'box', 'turn', 'ramp', 'grid',
             'mask', 'edge', 'grow', 'thin', 'flip', 'fold']
    for n in names:
        p = os.path.join(art, 'spec-%s-0.cells' % n)
        if not os.path.exists(p):
            cand = [f for f in os.listdir(art) if f.startswith('spec-%s' % n)]
            p = os.path.join(art, cand[0]) if cand else None
        if p:
            specs += '<figure><pre class="frame">%s</pre><figcaption>&gt;%s</figcaption></figure>' % (
                cells_text(p), n)

    parts = []
    for i, slug, title, stand, body in chapters:
        cmd = 'help' if slug == 'README' else slug
        reply = 'all of this, on the deck' if cmd == 'help' else '%s? try: help' % cmd
        words = (['the', 'deck,'] if slug == 'README' else slug.split('-'))
        h1 = ''.join('<span style="margin-left:%dpx">%s</span>' % (k * 48, html.escape(w))
                     for k, w in enumerate(words))
        steps = ''.join('<b>x</b>' if k == i else '.' for k in range(16))
        extra = ('<div class="specimens">%s</div>' % specs) if slug == 'pictures' and specs else ''
        parts.append(f'''
<article class="page" id="{slug}" data-step="{i}">
  <header class="head">
    <div class="num" aria-hidden="true">{i:02d}</div>
    <h1>{h1}</h1>
    <p class="cmd">&gt;{html.escape(cmd)}</p>
    <p class="reply"><span class="t">{chr(TILE + 137)}</span> {html.escape(reply)}</p>
    <p class="title">{html.escape(title)}</p>
    <div class="stand">{stand}</div>
    <div class="steps" aria-hidden="true">{steps}</div>
  </header>
  <div class="body">{extra}{body}</div>
</article>''')

    doc = f'''<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>The Deck, top to bottom</title>
<meta name="description" content="The cYbErDeCk wiki, set in the deck's own faces on the deck's own grid.">
<style>
{fonts_css()}
{CSS}
</style>
</head>
<body>
<div class="mast">
  <div class="name">THE DECK <b>/</b> TOP TO BOTTOM</div>
  <nav class="bar" aria-label="chapters">{bar}</nav>
  <div class="tools"><button id="grid" title="the grid (g)">GRID</button><button id="ink" title="paper or ink">PAPER/INK</button></div>
</div>
<div class="spine" aria-hidden="true">THE DECK, TOP TO BOTTOM &#183; 37 VERBS &#183; 16 LANES &#183; 16 PICTURES &#183; 12 x 24 &#183; SET IN THE DECK&#8217;S OWN FACES</div>
<section class="cover" id="top" data-step="0">
  <div>
    <h1><span>THE</span><span>DECK</span></h1>
    <p class="step"><span>top</span><span>to</span><span>bottom</span></p>
    <p class="blurb">A reference for the whole instrument: every verb, every character of the
    language, every picture, key, output and document, and how each meets the rest.</p>
    <p class="facts">Fifteen chapters and an index: one bar of sixteen steps. Set in the deck&#8217;s
    two faces, the round 12&#215;24 and the 6&#215;12, built from the firmware&#8217;s own font
    sources, on the deck&#8217;s own grid: every size is a whole number of its 12&#215;24 cells.
    Press G to see the grid. The picture is ORBITALS&#8217; planet, drawn by the deck&#8217;s picture
    engine and set in its tiles, turning at 124 bpm.</p>
  </div>
  <div class="art">{frames}<div class="cap">&gt;disc:x 8876532111235678 &#183; &gt;disc:y 568886531113 &#183; &gt;echo 8</div></div>
  <div class="tones" aria-hidden="true">{tones}</div>
</section>
<nav class="index" aria-label="contents">{index}</nav>
{''.join(parts)}
<footer class="colophon">
  <div class="big">&gt;help</div>
  <p>Generated from docs/wiki by tools/wiki_book.py. The Markdown is the source; this page is
  how it looks set on the deck&#8217;s grid. The faces are the deck&#8217;s: tools/deck_webfont.py
  builds them from firmware/components/textgrid, and tools/deck_extra.py draws the dashes,
  arrows and box drawing the panel never needed, on the same grids. The pictures are
  firmware/components/viz/viz.c, run by tools/zine_art.c.</p>
</footer>
<script>{JS}</script>
</body>
</html>
'''
    open(out, 'w').write(doc)
    print('%s: %d KB, %d chapters' % (out, len(doc) // 1024, len(chapters)))


if __name__ == '__main__':
    main()
