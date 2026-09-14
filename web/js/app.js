/* app.js - the catalogue listing, filters, and the game overlay. */
(function () {
'use strict';

var CATALOG = window.CATALOG || [];
var GAMES = window.GIC.GAMES;

var rowsEl   = document.getElementById('rows');
var moreEl   = document.getElementById('more');
var qEl      = document.getElementById('q');
var genreEl  = document.getElementById('genre');
var onlyEl   = document.getElementById('only');
var countEl  = document.getElementById('count');
var overlay  = document.getElementById('overlay');
var panelC   = document.getElementById('panel-content');
var titleEl  = document.getElementById('panel-title');
var slugEl   = document.getElementById('panel-slug');

var PAGE = 100;
var shown = PAGE;
var host = null;

/* ------------------------------------------------------------- filtering */

function matches(g) {
  if (onlyEl.getAttribute('aria-pressed') === 'true' && !g.implemented) return false;
  if (genreEl.value && g.genre !== genreEl.value) return false;
  var q = qEl.value.trim().toLowerCase();
  if (!q) return true;
  return (g.title + ' ' + g.genre + ' ' + g.mechanic + ' ' + g.blurb).toLowerCase().indexOf(q) >= 0;
}

function render() {
  var list = CATALOG.filter(matches);
  var slice = list.slice(0, shown);
  var frag = document.createDocumentFragment();

  rowsEl.textContent = '';

  slice.forEach(function (g, i) {
    var b = document.createElement('button');
    b.className = 'row ' + (g.implemented ? 'playable' : 'spec');
    b.type = 'button';
    b.dataset.slug = g.slug;
    /* Line numbers are the listing's own structure, so they carry the index. */
    b.innerHTML =
      '<span class="ln">' + String(i + 1).padStart(4, '0') + '</span>' +
      '<span class="title"></span>' +
      '<span class="genre"></span>' +
      '<span class="mech"></span>' +
      '<span class="tag">' + (g.implemented ? 'play' : 'spec') + '</span>';
    b.querySelector('.title').textContent = g.title;
    b.querySelector('.genre').textContent = g.genre;
    b.querySelector('.mech').textContent = g.mechanic;
    frag.appendChild(b);
  });

  if (!slice.length) {
    var p = document.createElement('p');
    p.className = 'empty';
    p.textContent = 'No games match those filters. Clear the search, or switch off “Playable only”.';
    frag.appendChild(p);
  }

  rowsEl.appendChild(frag);
  moreEl.hidden = list.length <= shown;
  moreEl.textContent = 'Print the next ' + Math.min(PAGE, list.length - shown) + ' lines';
  countEl.textContent = list.length + ' of ' + CATALOG.length + ' shown';
}

/* --------------------------------------------------------------- overlay */

function openGame(slug) {
  var g = CATALOG.filter(function (x) { return x.slug === slug; })[0];
  if (!g) return;

  titleEl.textContent = g.title;
  slugEl.textContent = g.slug;
  panelC.textContent = '';

  if (g.implemented && GAMES[g.family]) {
    var wrap = document.createElement('div');
    wrap.className = 'screen-wrap';
    var canvas = document.createElement('canvas');
    wrap.appendChild(canvas);

    var help = document.createElement('p');
    help.className = 'helpline';

    var pad = document.createElement('div');
    pad.className = 'pad';
    [['↑', 'up'], ['↓', 'down'], ['←', 'left'], ['→', 'right'],
     ['Enter', 'enter'], ['Space', 'space']].forEach(function (spec) {
      var btn = document.createElement('button');
      btn.type = 'button';
      btn.textContent = spec[0];
      if (spec[1] === 'enter' || spec[1] === 'space') btn.className = 'wide';
      btn.addEventListener('click', function () { if (host) host.key(spec[1]); });
      pad.appendChild(btn);
    });

    panelC.appendChild(wrap);
    panelC.appendChild(help);
    panelC.appendChild(pad);

    overlay.classList.add('open');
    host = new window.GIC.Host(canvas, pad, help);
    host.start(g.family, g.params, g.slug);
  } else {
    panelC.appendChild(specSheet(g));
    overlay.classList.add('open');
  }
  document.getElementById('close').focus();
}

function specSheet(g) {
  var d = document.createElement('div');
  d.className = 'spec-body';
  var pips = '';
  for (var i = 0; i < 5; i++) pips += '<i class="' + (i < g.difficulty ? 'on' : '') + '">●</i>';

  d.innerHTML =
    '<dl>' +
      '<dt>Genre</dt><dd>' + esc(g.genre) + '</dd>' +
      '<dt>Mechanic</dt><dd>' + esc(g.mechanic) + '</dd>' +
      '<dt>Players</dt><dd>' + esc(g.players) + '</dd>' +
      '<dt>Difficulty</dt><dd class="pips">' + pips + '</dd>' +
      '<dt>Slug</dt><dd>' + esc(g.slug) + '</dd>' +
      '<dt>Engine</dt><dd>' + esc(g.family) + '</dd>' +
    '</dl>' +
    '<p>' + esc(g.blurb) + '</p>' +
    '<p style="color:#7b8189">Specified and already bound to its engine and parameters. ' +
    'It becomes playable as soon as the <code>' + esc(g.family) + '</code> engine lands.</p>';
  return d;
}

function esc(s) {
  return String(s).replace(/[&<>"]/g, function (c) {
    return { '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;' }[c];
  });
}

function closeOverlay() {
  if (host) { host.stop(); host = null; }
  overlay.classList.remove('open');
  panelC.textContent = '';
}
window.GICApp = { closeOverlay: closeOverlay };

/* ------------------------------------------------------------------ wire */

rowsEl.addEventListener('click', function (e) {
  var row = e.target.closest('.row');
  if (row) openGame(row.dataset.slug);
});
moreEl.addEventListener('click', function () { shown += PAGE; render(); });
qEl.addEventListener('input', function () { shown = PAGE; render(); });
genreEl.addEventListener('change', function () { shown = PAGE; render(); });
onlyEl.addEventListener('click', function () {
  var on = onlyEl.getAttribute('aria-pressed') === 'true';
  onlyEl.setAttribute('aria-pressed', String(!on));
  shown = PAGE;
  render();
});
document.getElementById('close').addEventListener('click', closeOverlay);
overlay.addEventListener('click', function (e) { if (e.target === overlay) closeOverlay(); });
document.addEventListener('keydown', function (e) {
  if (e.key === 'Escape' && overlay.classList.contains('open')) closeOverlay();
});

/* --------------------------------------------------------------- startup */

(function init() {
  var genres = {}, playable = 0;
  CATALOG.forEach(function (g) {
    genres[g.genre] = (genres[g.genre] || 0) + 1;
    if (g.implemented) playable++;
  });

  Object.keys(genres).sort().forEach(function (name) {
    var o = document.createElement('option');
    o.value = name;
    o.textContent = name + ' (' + genres[name] + ')';
    genreEl.appendChild(o);
  });

  document.getElementById('stat-total').textContent = CATALOG.length;
  document.getElementById('stat-play').textContent = playable;
  document.getElementById('stat-genre').textContent = Object.keys(genres).length;

  var wired = CATALOG.filter(function (g) { return g.implemented && GAMES[g.family]; }).length;
  document.getElementById('foot-note').textContent =
    wired + ' games wired to this page';

  render();
}());

}());
