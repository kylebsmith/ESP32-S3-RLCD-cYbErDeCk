// The Strudel conformance corpus - generator.
//
// docs/NEXT.md §11: build the CORPUS, not the compiler. Each entry is a Strudel
// mini-notation string, its translation into this deck's step grammar, and the
// events STRUDEL ITSELF produces for it - so tools/test_corpus.c can compile the
// translation with the shipping compiler and check the deck plays the same notes
// at the same times. Anything outside the notation is listed as out of scope, with
// the reason, which is the one-page "what transfers and what does not" the brief
// asks for.
//
// The expected events come from Strudel, not from anyone's reading of Strudel:
// that is the whole value of a corpus. This script is run by hand, when Strudel or
// the grammar changes; the host check only ever reads the file it writes.
//
//   mkdir -p /tmp/strudel && cd /tmp/strudel && npm init -y
//   npm install @strudel/core@1.2.6 @strudel/mini@1.2.6
//   node <repo>/tools/corpus/gen.mjs <repo>/tools/corpus > <repo>/tools/corpus/strudel.txt
//
// and the real-world set, which is NOT committed - Strudel's example tunes are
// other people's work - but is how docs/STRUDEL.md's numbers were measured:
//
//   git clone --depth 1 https://codeberg.org/uzu/strudel.git /tmp/strudel-src
//   node <repo>/tools/corpus/gen.mjs <repo>/tools/corpus \
//        /tmp/strudel-src/website/src/repl/tunes.mjs > /tmp/world.txt
//   test_corpus /tmp/world.txt
//
// (If Node says @kabelsalat/web "does not provide an export named SalatRepl":
// the published package's CommonJS entry lacks it and only Strudel's REPL uses it.
// Copy node_modules/@kabelsalat/web/dist/index.mjs over dist/index.js.)
//
// Strudel is AGPL-3.0. Nothing of it is vendored here or linked into the deck: it
// is run as an oracle, and what is committed is pattern strings and event times.

import fs from 'fs';
import path from 'path';
import { createRequire } from 'module';

const require = createRequire(path.join(process.cwd(), 'noop.js'));
const mini = await import(require.resolve('@strudel/mini'));

const dir = process.argv[2] || '.';
const CYCLES_FEATURE = 12;       // covers every period 1, 2, 3, 4, 6 and 12
const CYCLES_WORLD = 4;

class Out extends Error {}
const out = (why) => { throw new Out(why); };

// Strudel's parse tree -> this deck's step grammar. `words` maps any word to 'x'
// (the real-world set compares rhythm, since a step here is one character and a
// name - 'bd', 'c3' - is a binding, not a step). Returns the list of ITEMS a node
// becomes at its level, because '!n' makes one element into several.
function items(node, words) {
    if (node.type_ === 'atom') {
        const s = node.source_;
        if (s === '~' || s === '-') return ['.'];
        // The real-world set compares RHYTHM: any value - 'bd', 'c3', 'B^7', '-5',
        // '.3' - is a hit there, because what it means is the receiver's and when
        // it happens is the notation's.
        if (words) return ['x'];
        if (/^[x0-9]$/.test(s)) return [s];
        if (/^-?[0-9]+(\.[0-9]+)?$/.test(s)) out('a number past 9 - a digit is 0-9 here');
        out('a word - a step here is one character; a name is a binding');
    }
    if (node.type_ === 'element') {
        const o = node.options_;
        let inner = items(node.source_, words);
        let prob = null;
        let reps = 1;
        for (const op of o.ops) {
            const a = op.arguments_;
            if (op.type_ === 'degradeBy') {
                const amt = (a.amount === undefined || a.amount === null) ? 0.5 : Number(a.amount);
                prob = (prob ?? 100) * (1 - amt);
            } else if (op.type_ === 'replicate') {
                reps = a.amount;
            } else if (op.type_ === 'stretch') {
                const amt = a.amount;
                if (!amt || amt.type_ !== 'atom' || !/^[0-9]+$/.test(amt.source_)) {
                    out(`${a.type === 'slow' ? '/' : '*'}<pattern> - a patterned speed is a function`);
                }
                // '/n' on a step makes ONE note n passes long - '<0 3>/2' is six
                // onsets in twelve cycles, not twelve - and a note here ends with
                // its lane's pass. (The corpus caught this: '<a a b b>' was the
                // first guess, and Strudel disagreed.)
                if (a.type === 'slow') out('/n on a step - one note over n passes; a tie ends with the pass');
                const n = Number(amt.source_);
                if (inner.length === 1 && inner[0].startsWith('<')) {
                    out('* on <> - one member per pass; a faster alternation is a function');
                }
                // x*3 is [xxx]; [a b]*2 is [abab]
                const body = (inner.length === 1 && inner[0].startsWith('['))
                    ? inner[0].slice(1, -1) : inner.join(' ');
                if (body.includes(',')) out('* on a stack - write the repeats out');
                inner = ['[' + Array(n).fill(body).join(' ') + ']'];
            } else if (op.type_ === 'bjorklund') {
                out('(p,s) Euclidean rhythm - MAP.md §9.2, still open');
            } else if (op.type_ === 'tail') {
                out(':n sample index - what a note means is the receiver\'s (MAP.md §9.4)');
            } else if (op.type_ === 'range') {
                out('.. a range - a function, not notation');
            } else {
                out(`${op.type_} - not notation here`);
            }
        }
        const weight = o.weight / reps;           // replicate sets weight to reps
        if (!Number.isInteger(weight)) out('a fractional weight');
        if (weight > 1 && (inner.length !== 1 || inner[0].length !== 1 || inner[0] === '.')) {
            out('@ on a group - a tie holds a note here; Strudel stretches the group');
        }
        let one = inner.length === 1 ? inner[0] : '[' + inner.join(' ') + ']';
        if (prob !== null) {
            if (one === '.') out('odds on a rest');
            one += '%' + Math.round(prob);
        }
        const res = [];
        for (let r = 0; r < reps; r++) {
            res.push(one);
            for (let w = 1; w < weight; w++) res.push('_');
        }
        return res;
    }
    if (node.type_ === 'pattern') {
        const al = node.arguments_.alignment;
        const kids = node.source_;
        if (al === 'fastcat') {
            if (node.arguments_._steps) out('^ step marking - a Strudel-only extension');
            return kids.flatMap((k) => items(k, words));
        }
        if (al === 'stack') {
            return ['[' + kids.map((k) => items(k, words).join(' ')).join(', ') + ']'];
        }
        if (al === 'feet') {
            return kids.map((k) => '[' + items(k, words).join(' ') + ']');
        }
        if (al === 'polymeter_slowcat') {
            const members = kids.map((k) => items(k, words));
            if (members.some((m) => m.includes('_'))) {
                out('@ inside <> - an alternative is one step here, not a length');
            }
            return ['<' + members.map((m) => m.join(' ')).join(',') + '>'];
        }
        if (al === 'rand') out('| a random choice - a function, not notation here');
        if (al === 'polymeter') out('{} polymeter - one lane has one length; two lanes are two');
        out(`${al} - not notation here`);
    }
    out(`${node.type_} - not notation here`);
}

// A copy of the tree with every '?' taken out: the notes that CAN play.
function strip(node) {
    if (node.type_ === 'element') {
        return { ...node, options_: { ...node.options_,
            ops: node.options_.ops.filter((o) => o.type_ !== 'degradeBy') },
            source_: strip(node.source_) };
    }
    if (node.type_ === 'pattern') {
        return { ...node, source_: node.source_.map(strip) };
    }
    return node;
}

const frac = (f) => f.toFraction();

function translate(src, words) {
    const ast = mini.mini2ast('"' + src + '"');
    // Spaced, as the deck allows: a space is for the eye and never a step, and
    // without one '[0,4]%50 7' would run together into odds of 507.
    const top = items(ast, words);
    return top.join(' ');
}

function events(src, cycles, words) {
    const ast = strip(mini.mini2ast('"' + src + '"'));
    const pat = mini.patternifyAST(ast, '"' + src + '"');
    const ev = [];
    for (let c = 0; c < cycles; c++) {
        for (const h of pat.queryArc(c, c + 1)) {
            if (!h.hasOnset()) continue;
            if (h.value === '~' || h.value === '-') continue;
            const v = String(h.value);
            const val = /^[0-9]$/.test(v) ? v : 'x';
            ev.push([h.whole.begin, h.whole.end.sub(h.whole.begin), val]);
        }
    }
    ev.sort((a, b) => a[0].compare(b[0]) || a[2].localeCompare(b[2]));
    return ev.map(([b, d, v]) => `${frac(b)}:${frac(d)}:${v}`).join(' ');
}

function entry(src, cycles, words, note) {
    const lines = [`S ${src}`];
    if (note) lines.push(`# ${note}`);
    try {
        const deck = translate(src, words);
        lines.push(`D ${deck}`);
        lines.push(`N ${cycles}`);
        lines.push(`E ${events(src, cycles, words)}`);
    } catch (e) {
        if (!(e instanceof Out)) throw e;
        lines.push(`X ${e.message}`);
    }
    return lines.join('\n');
}

const feature = fs.readFileSync(path.join(dir, 'features.txt'), 'utf8')
    .split('\n').map((l) => l.replace(/\s+$/, '')).filter((l) => l && !l.startsWith('#'));

console.log('# The Strudel conformance corpus. GENERATED by tools/corpus/gen.mjs from');
console.log('# Strudel ' + JSON.parse(fs.readFileSync(require.resolve('@strudel/mini/package.json'))).version +
            ' - do not edit by hand. Checked by tools/test_corpus.c.');
console.log('#');
console.log('# S  the Strudel mini-notation string');
console.log('# D  its translation into the deck\'s step grammar');
console.log('# N  cycles compared');
console.log('# E  what Strudel plays: begin:duration:value, in cycles, every onset');
console.log('#    ("?" is compared as the notes that CAN play, and their odds separately)');
console.log('# X  out of scope, and why');
console.log('# W  a real-world pattern from Strudel\'s own tunes: words are hits, and only');
console.log('#    the rhythm is compared');
console.log('');
console.log('## features');
for (const f of feature) {
    console.log(entry(f, CYCLES_FEATURE, false));
    console.log('');
}

// THE REAL-WORLD SET: every mini-notation string in Strudel's example tunes,
// passed in as a file of one string per line.
const worldFile = process.argv[3];
if (worldFile) {
    // every double-quoted string in the tunes file; the ones that are not
    // mini-notation fail to parse and are skipped below
    const src = fs.readFileSync(worldFile, 'utf8');
    const world = [...new Set([...src.matchAll(/"((?:[^"\\\n]|\\.)*)"/g)].map((m) => m[1]))];
    console.log('## world');
    for (const w of world) {
        try { mini.mini2ast('"' + w + '"'); } catch { continue; }
        console.log(entry(w, CYCLES_WORLD, true).replace(/^S /, 'W '));
        console.log('');
    }
}
