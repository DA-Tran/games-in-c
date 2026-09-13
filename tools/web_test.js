/* web_test.js - headless harness for the browser build.
 *
 * Loads engine.js and every game module against a minimal DOM/canvas stub,
 * then actually plays each game: hundreds of random keypresses, real ticks
 * for the real-time ones, and a full draw after every step. A game passes
 * only if it never throws and never writes outside its terminal grid.
 *
 * Run: node tools/web_test.js
 */
'use strict';

const fs = require('fs');
const path = require('path');
const vm = require('vm');

const ROOT = path.join(__dirname, '..');
const WEB = path.join(ROOT, 'web');

/* ----------------------------------------------------------- DOM stubs */

function makeCtx() {
  return {
    fillStyle: '', font: '', textBaseline: '', textAlign: '',
    setTransform() {}, fillRect() {}, fillText() {}
  };
}

function makeCanvas() {
  return { width: 0, height: 0, style: {}, getContext: makeCtx };
}

let now = 0;
const listeners = {};

const sandbox = {
  console,
  performance: { now: () => (now += 16) },
  requestAnimationFrame: () => 1,
  cancelAnimationFrame: () => {},
  localStorage: (() => {
    const m = new Map();
    return {
      getItem: k => (m.has(k) ? m.get(k) : null),
      setItem: (k, v) => m.set(k, String(v))
    };
  })(),
  devicePixelRatio: 1,
  innerWidth: 1000,
  addEventListener: (t, f) => { (listeners[t] = listeners[t] || []).push(f); },
  removeEventListener: () => {},
  AudioContext: function () {
    return {
      currentTime: 0,
      createOscillator: () => ({
        type: '', frequency: { value: 0 },
        connect: o => o, start() {}, stop() {}
      }),
      createGain: () => ({
        gain: { setValueAtTime() {}, exponentialRampToValueAtTime() {} },
        connect: o => o
      }),
      destination: {}
    };
  },
  GICApp: { closeOverlay() {} }
};
sandbox.window = sandbox;
sandbox.global = sandbox;

vm.createContext(sandbox);

function load(rel) {
  const file = path.join(WEB, rel);
  vm.runInContext(fs.readFileSync(file, 'utf8'), sandbox, { filename: rel });
}

load('js/catalog.js');
load('js/engine.js');
['board', 'puzzle', 'arcade', 'cards', 'word'].forEach(m => load('js/games/' + m + '.js'));

const GIC = sandbox.GIC;
const CATALOG = sandbox.CATALOG;

/* -------------------------------------------------------------- checks */

let failures = [];
function fail(what, why) {
  failures.push(what + ': ' + why);
  console.log('  FAIL  ' + what + ' — ' + why);
}

console.log('catalog integrity');
{
  const slugs = new Set();
  let dupes = 0, bad = 0;
  CATALOG.forEach(g => {
    if (slugs.has(g.slug)) dupes++;
    slugs.add(g.slug);
    if (!g.title || !g.genre || !g.mechanic || !g.blurb) bad++;
    if (!(g.difficulty >= 1 && g.difficulty <= 5)) bad++;
  });
  if (CATALOG.length !== 1000) fail('catalog size', 'expected 1000, got ' + CATALOG.length);
  if (dupes) fail('catalog slugs', dupes + ' duplicates');
  if (bad) fail('catalog fields', bad + ' entries with missing or invalid fields');
  console.log('  ' + CATALOG.length + ' entries, ' + slugs.size + ' unique slugs');
}

console.log('registry cross-check');
{
  const impl = CATALOG.filter(g => g.implemented).map(g => g.slug);
  const wired = Object.keys(GIC.GAMES);
  impl.forEach(s => { if (!GIC.GAMES[s]) fail('missing JS port', s); });
  wired.forEach(s => {
    const e = CATALOG.filter(g => g.slug === s)[0];
    if (!e) fail('JS game not in catalog', s);
    else if (!e.implemented) fail('JS game not marked implemented', s);
  });
  console.log('  ' + impl.length + ' marked implemented, ' + wired.length + ' wired in JS');
}

/* ---------------------------------------------------------- play tests */

const KEYS = ['up','down','left','right','enter','space','tab','back',
              '1','2','3','4','5','6','7','8','9','0',
              'a','b','c','d','e','f','g','h','l','n','p','r','s','t','u','x','y','>'];

console.log('playing every game');
Object.keys(GIC.GAMES).sort().forEach(slug => {
  const canvas = makeCanvas();
  const host = new GIC.Host(canvas, {}, { textContent: '' });
  let term;

  try {
    host.start(slug);
    term = host.term;
  } catch (e) {
    fail(slug, 'start threw: ' + e.message);
    return;
  }

  const def = GIC.GAMES[slug];
  let steps = 0;

  try {
    for (let i = 0; i < 400; i++) {
      const k = KEYS[Math.floor(Math.random() * KEYS.length)];
      host.game.key(k);
      if (def.realtime && host.game.tick) { host.game.tick(); steps++; }
      host.game.draw(term);
    }
    /* Real-time games also need to survive a long idle run with no input. */
    if (def.realtime && host.game.tick) {
      for (let i = 0; i < 600; i++) { host.game.tick(); steps++; host.game.draw(term); }
    }
    term.flush();
  } catch (e) {
    fail(slug, e.message);
    return;
  }

  /* Every glyph must have landed inside the declared grid. */
  if (term.buf.length !== term.rows || term.buf[0].length !== term.cols)
    fail(slug, 'terminal buffer resized unexpectedly');

  console.log('  ok    ' + slug.padEnd(22) + (def.realtime ? steps + ' ticks' : 'turn-based'));
});

console.log('');
if (failures.length) {
  console.log(failures.length + ' failure(s):');
  failures.forEach(f => console.log('  - ' + f));
  process.exit(1);
}
console.log('web build verified: catalog intact, ' +
            Object.keys(GIC.GAMES).length + ' games played clean');
