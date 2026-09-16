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
  this.clipped = 0;   /* glyphs lost past the growth cap - always a bug */
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

/* Extend the grid, keeping everything already drawn.
 *
 * The terminal build has to warn when a board is bigger than the window,
 * because it cannot make the window bigger. Here the terminal is a canvas, so
 * the honest answer is to grow it and show the player the whole board. Growing
 * mid-frame is safe: existing rows are extended rather than rebuilt, so the
 * part of the frame already drawn survives and the rest lands in the right
 * place. The cap is there so that one bad coordinate cannot ask the browser
 * for a canvas of a hundred thousand cells. */
var MAXCOLS = 200, MAXROWS = 120;

Term.prototype.grow = function (cols, rows) {
  var r, c, row;
  cols = Math.min(Math.max(cols, this.cols), MAXCOLS);
  rows = Math.min(Math.max(rows, this.rows), MAXROWS);
  if (cols === this.cols && rows === this.rows) return;

  for (r = 0; r < this.buf.length; r++) {
    for (c = this.buf[r].length; c < cols; c++) {
      this.buf[r].push({ ch: ' ', fg: COL.fg, bg: null, bold: false });
    }
  }
  for (r = this.buf.length; r < rows; r++) {
    row = [];
    for (c = 0; c < cols; c++) row.push({ ch: ' ', fg: COL.fg, bg: null, bold: false });
    this.buf.push(row);
  }
  this.cols = cols;
  this.rows = rows;
  this.resize();
};

Term.prototype.put = function (x, y, ch, fg, bg, bold) {
  x = Math.round(x); y = Math.round(y);
  if (x < 0 || y < 0) return;
  if (x >= this.cols || y >= this.rows) {
    this.grow(x + 1, y + 1);
    if (x >= this.cols || y >= this.rows) { this.clipped++; return; }
  }
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
  /* No rule under the header, for the same reason draw_title() in the C engine
   * no longer draws one. Row 2 here is row 4 there - the first row a game may
   * use - so the rule was something for a game to collide with. Pong puts its
   * score exactly there, and had it reading as a break in a horizontal line.
   * Leaving the row clear also keeps the two builds looking the same, which is
   * the point of mirroring the C engine rather than reimplementing it. */
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

function rngNext() {
  rngState ^= rngState << 13; rngState >>>= 0;
  rngState ^= rngState >>> 17;
  rngState ^= rngState << 5;  rngState >>>= 0;
  return rngState;
}

function rnd(n) {
  if (n <= 0) return 0;
  if (!rngState) return Math.floor(Math.random() * n);
  return rngNext() % n;
}

/* A fraction in [0,1), matching the C rnd_f: the top 24 bits over 2^24.
 * Games that want a direction or a speed need this rather than rnd(), and
 * before it existed they reached for Math.random - which is not seeded, so a
 * pinned entry drew different rocks every time and disagreed with the C
 * build. */
function rndF() {
  if (!rngState) return Math.random();
  return (rngNext() >>> 8) / 16777216;
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
  /* Exported so the generator can be seeded without starting a game, which is
   * what lets a test check that a pinned seed produces the same sequence here
   * as it does in the C build. A "daily" entry is only meaningful if the two
   * agree. */
  rngSeed: rngSeed, rndF: rndF,
  Scores: Scores
};

}(window));
