/* engine.js - the browser counterpart of src/engine.
 *
 * The C games draw to a character grid, so the browser draws one too: a
 * canvas-rendered terminal. That keeps every JS port line-for-line faithful
 * to its C original instead of being a loose reimplementation.
 */
(function (global) {
'use strict';

/* Screen palette, matching the ANSI names the C side uses. */
var COL = {
  fg:      '#d6d9d2',
  dim:     '#6d747c',
  grey:    '#6d747c',
  white:   '#eef1ea',
  black:   '#14161a',
  red:     '#e1503c',
  green:   '#63b755',
  yellow:  '#d8a43c',
  blue:    '#4d8fd6',
  magenta: '#b06ed0',
  cyan:    '#49b3b8'
};

/* ------------------------------------------------------------------ Term */

function Term(canvas, cols, rows) {
  this.canvas = canvas;
  this.cols = cols;
  this.rows = rows;
  this.ctx = canvas.getContext('2d');
  this.cell = { w: 0, h: 0 };
  this.buf = [];
  this.clear();
  this.resize();
}

Term.prototype.resize = function () {
  var avail = Math.min(960, (global.innerWidth || 960) - 60);
  var cw = Math.max(7, Math.floor(avail / this.cols));
  var ch = Math.round(cw * 1.9);
  var dpr = global.devicePixelRatio || 1;

  this.cell.w = cw;
  this.cell.h = ch;
  this.canvas.width = cw * this.cols * dpr;
  this.canvas.height = ch * this.rows * dpr;
  this.canvas.style.width = (cw * this.cols) + 'px';
  this.canvas.style.height = (ch * this.rows) + 'px';
  this.ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
  this.font = Math.round(ch * 0.78) + 'px ui-monospace, Menlo, Consolas, monospace';
};

Term.prototype.clear = function () {
  var r, c, row;
  this.buf = [];
  for (r = 0; r < this.rows; r++) {
    row = [];
    for (c = 0; c < this.cols; c++) row.push({ ch: ' ', fg: COL.fg, bg: null, bold: false });
    this.buf.push(row);
  }
};

Term.prototype.put = function (x, y, ch, fg, bg, bold) {
  x = Math.round(x); y = Math.round(y);
  if (x < 0 || x >= this.cols || y < 0 || y >= this.rows) return;
  var cell = this.buf[y][x];
  cell.ch = ch;
  cell.fg = fg || COL.fg;
  cell.bg = bg || null;
  cell.bold = !!bold;
};

Term.prototype.text = function (x, y, str, fg, bg, bold) {
  str = String(str);
  for (var i = 0; i < str.length; i++) this.put(x + i, y, str[i], fg, bg, bold);
};

Term.prototype.center = function (y, str, fg, bold) {
  this.text(Math.max(0, Math.floor((this.cols - String(str).length) / 2)), y, str, fg, null, bold);
};

Term.prototype.box = function (x, y, w, h, fg) {
  var i;
  this.put(x, y, '┌', fg); this.put(x + w - 1, y, '┐', fg);
  this.put(x, y + h - 1, '└', fg); this.put(x + w - 1, y + h - 1, '┘', fg);
  for (i = 1; i < w - 1; i++) { this.put(x + i, y, '─', fg); this.put(x + i, y + h - 1, '─', fg); }
  for (i = 1; i < h - 1; i++) { this.put(x, y + i, '│', fg); this.put(x + w - 1, y + i, '│', fg); }
};

Term.prototype.hline = function (x, y, w, fg) {
  for (var i = 0; i < w; i++) this.put(x + i, y, '─', fg);
};

/* Standard game header, mirroring draw_title() in the C engine. */
Term.prototype.header = function (title, subtitle) {
  this.clear();
  this.center(0, title, COL.cyan, true);
  if (subtitle) this.center(1, subtitle, COL.dim);
  this.hline(1, 2, this.cols - 2, '#2e333a');
};

Term.prototype.flush = function () {
  var ctx = this.ctx, r, c, cell,
      w = this.cell.w, h = this.cell.h;

  ctx.fillStyle = COL.black;
  ctx.fillRect(0, 0, w * this.cols, h * this.rows);
  ctx.textBaseline = 'middle';
  ctx.textAlign = 'center';

  for (r = 0; r < this.rows; r++) {
    for (c = 0; c < this.cols; c++) {
      cell = this.buf[r][c];
      if (cell.bg) {
        ctx.fillStyle = cell.bg;
        ctx.fillRect(c * w, r * h, w, h);
      }
      if (cell.ch === ' ') continue;
      ctx.fillStyle = cell.fg;
      ctx.font = (cell.bold ? 'bold ' : '') + this.font;
      ctx.fillText(cell.ch, c * w + w / 2, r * h + h / 2 + 1);
    }
  }
};

/* --------------------------------------------------------------- helpers */

/* The same xorshift the C build uses, so a pinned seed produces the same
 * sequence in both. Unseeded it falls back to Math.random, which keeps every
 * ordinary entry as varied as before. */
var rngState = 0;
function rngSeed(s) { rngState = (s >>> 0) || 0; }
function rnd(n) {
  if (n <= 0) return 0;
  if (!rngState) return Math.floor(Math.random() * n);
  rngState ^= rngState << 13; rngState >>>= 0;
  rngState ^= rngState >>> 17;
  rngState ^= rngState << 5;  rngState >>>= 0;
  return rngState % n;
}
function rndRange(lo, hi) { return lo + rnd(hi - lo + 1); }
function shuffle(a) {
  for (var i = a.length - 1; i > 0; i--) {
    var j = rnd(i + 1), t = a[i]; a[i] = a[j]; a[j] = t;
  }
  return a;
}

var Scores = {
  key: function (slug) { return 'gic:' + slug; },
  load: function (slug) {
    try { return parseInt(global.localStorage.getItem(this.key(slug)) || '0', 10) || 0; }
    catch (e) { return 0; }
  },
  save: function (slug, v) {
    try {
      if (v > this.load(slug)) global.localStorage.setItem(this.key(slug), String(v));
    } catch (e) { /* private mode: scores simply do not persist */ }
  }
};

/* ---------------------------------------------------------------- Host */

var GAMES = {};

function register(slug, def) { GAMES[slug] = def; }

/* Runs one game against a Term, wiring keyboard, touch pad and the clock. */
function Host(canvas, padEl, helpEl) {
  this.canvas = canvas;
  this.padEl = padEl;
  this.helpEl = helpEl;
  this.term = null;
  this.game = null;
  this.slug = null;
  this.raf = null;
  this.last = 0;
  this.acc = 0;
  var self = this;
  this._onKey = function (e) { self.handleKey(e); };
}

/* Start `family` configured by `params`. `slug` only scopes the high score,
 * so every catalogue entry keeps its own best even when they share an engine. */
Host.prototype.start = function (family, params, slug) {
  var def = GAMES[family];
  if (!def) return false;

  this.stop();
  this.family = family;
  this.slug = slug || family;
  this.params = params || {};
  /* A catalogue entry with a pinned seed is a fixed puzzle - the same grid
   * for everyone, every time - which is what separates "Daily Sudoku 3" from
   * the ordinary 9x9 entry it otherwise matches parameter for parameter. */
  rngSeed(this.params.seed || 0);
  this.def = def;
  this.term = new Term(this.canvas, def.cols || 84, def.rows || 28);
  this.game = def.start(this, this.params);
  this.helpEl.textContent = def.help || '';

  global.addEventListener('keydown', this._onKey, { passive: false });
  this.draw();

  if (def.realtime) {
    var self = this;
    this.last = performance.now();
    this.acc = 0;
    var loop = function (now) {
      var dt = now - self.last;
      self.last = now;
      self.acc += dt;
      var step = self.game.step || def.step || 100;
      var guard = 0;
      while (self.acc >= step && guard++ < 5) {
        self.acc -= step;
        if (!self.game.over) self.game.tick();
      }
      self.draw();
      self.raf = global.requestAnimationFrame(loop);
    };
    this.raf = global.requestAnimationFrame(loop);
  }
  return true;
};

Host.prototype.stop = function () {
  if (this.raf) global.cancelAnimationFrame(this.raf);
  this.raf = null;
  global.removeEventListener('keydown', this._onKey);
  this.game = null;
};

Host.prototype.draw = function () {
  if (!this.game || !this.term) return;
  this.game.draw(this.term);
  this.term.flush();
};

/* Normalise browser key events into the same names the games expect. */
Host.prototype.handleKey = function (e) {
  var map = {
    ArrowUp: 'up', ArrowDown: 'down', ArrowLeft: 'left', ArrowRight: 'right',
    Enter: 'enter', ' ': 'space', Escape: 'esc', Backspace: 'back', Tab: 'tab'
  };
  var k = map[e.key] || (e.key.length === 1 ? e.key.toLowerCase() : null);
  if (!k) return;
  if (k !== 'esc') e.preventDefault();
  this.key(k);
};

Host.prototype.key = function (k) {
  if (!this.game) return;
  if (k === 'q' || k === 'esc') { global.GICApp.closeOverlay(); return; }
  this.game.key(k);
  if (!this.def.realtime) this.draw();
};

Host.prototype.saveScore = function (v) {
  if (this.slug) Scores.save(this.slug, v);
};

Host.prototype.best = function () {
  return this.slug ? Scores.load(this.slug) : 0;
};

global.GIC = {
  Term: Term, Host: Host, COL: COL, GAMES: GAMES,
  register: register, rnd: rnd, rndRange: rndRange, shuffle: shuffle,
  Scores: Scores
};

}(window));
