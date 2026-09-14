/* puzzle2.js - ports of the newer grid engines: maze and friends.
 *
 * Kept separate from puzzle.js so the original ten families stay readable.
 */
(function () {
'use strict';
var G = window.GIC, C = G.COL, reg = G.register, rnd = G.rnd, shuffle = G.shuffle;

/* ------------------------------------------------------------------- maze */
reg('maze', {
  title: 'Maze', help: 'Arrows move · H hints · Q quits',
  start: function (host, p) {
    var ALGO = ['Recursive Backtracker','Prim','Kruskal','Eller','Wilson',
                'Aldous-Broder','Hunt and Kill','Binary Tree','Sidewinder','Growing Tree'];
    var VAR = (p.variant >= 0 && p.variant <= 9) ? p.variant : 0;
    var N = Math.max(7, Math.min(41, p.size > 0 ? p.size : 21));
    if (!(N & 1)) N++;
    var CW = (N - 1) / 2;
    var DR = [-1,1,0,0], DC = [0,0,-1,1];
    var wall, pr, pc, steps, hint, won, par, path;

    function g(c) { return c * 2 + 1; }
    function inCells(r, c) { return r >= 0 && r < CW && c >= 0 && c < CW; }
    function open2(r1, c1, r2, c2) {
      wall[(g(r1) + g(r2)) / 2][(g(c1) + g(c2)) / 2] = 0;
      wall[g(r1)][g(c1)] = 0;
      wall[g(r2)][g(c2)] = 0;
    }
    function blankSeen() {
      var s = [];
      for (var r = 0; r < CW; r++) s.push(new Array(CW).fill(0));
      return s;
    }

    function genBacktracker() {
      var seen = blankSeen(), stack = [], r = rnd(CW), c = rnd(CW);
      seen[r][c] = 1; wall[g(r)][g(c)] = 0; stack.push([r, c]);
      while (stack.length) {
        var t = stack[stack.length - 1], moved = false;
        r = t[0]; c = t[1];
        var order = shuffle([0,1,2,3]);
        for (var i = 0; i < 4; i++) {
          var nr = r + DR[order[i]], nc = c + DC[order[i]];
          if (!inCells(nr, nc) || seen[nr][nc]) continue;
          seen[nr][nc] = 1; open2(r, c, nr, nc); stack.push([nr, nc]); moved = true; break;
        }
        if (!moved) stack.pop();
      }
    }
    function genPrim() {
      var seen = blankSeen(), fr = [], r = rnd(CW), c = rnd(CW), d;
      seen[r][c] = 1; wall[g(r)][g(c)] = 0;
      for (d = 0; d < 4; d++) if (inCells(r + DR[d], c + DC[d])) fr.push([r + DR[d], c + DC[d], r, c]);
      while (fr.length) {
        var i = rnd(fr.length), e = fr[i];
        fr[i] = fr[fr.length - 1]; fr.pop();
        if (seen[e[0]][e[1]]) continue;
        seen[e[0]][e[1]] = 1;
        open2(e[2], e[3], e[0], e[1]);
        for (d = 0; d < 4; d++) {
          var ar = e[0] + DR[d], ac = e[1] + DC[d];
          if (inCells(ar, ac) && !seen[ar][ac]) fr.push([ar, ac, e[0], e[1]]);
        }
      }
    }
    function genKruskal() {
      var edges = [], uf = [], r, c, i;
      for (r = 0; r < CW; r++) for (c = 0; c < CW; c++) {
        if (c + 1 < CW) edges.push([r, c, r, c + 1]);
        if (r + 1 < CW) edges.push([r, c, r + 1, c]);
      }
      for (i = 0; i < CW * CW; i++) uf.push(i);
      function find(a) { while (uf[a] !== a) { uf[a] = uf[uf[a]]; a = uf[a]; } return a; }
      edges = shuffle(edges);
      for (i = 0; i < edges.length; i++) {
        var a = find(edges[i][0] * CW + edges[i][1]), b = find(edges[i][2] * CW + edges[i][3]);
        if (a === b) continue;
        uf[a] = b;
        open2(edges[i][0], edges[i][1], edges[i][2], edges[i][3]);
      }
    }
    function genEller() {
      var set = [], next = 1, r, c, k;
      for (c = 0; c < CW; c++) set.push(next++);
      for (r = 0; r < CW; r++) {
        var last = (r === CW - 1);
        for (c = 0; c + 1 < CW; c++) {
          if (set[c] === set[c + 1]) continue;
          if (!last && rnd(2)) continue;
          var old = set[c + 1];
          open2(r, c, r, c + 1);
          for (k = 0; k < CW; k++) if (set[k] === old) set[k] = set[c];
        }
        if (last) break;
        var dropped = new Array(CW).fill(0);
        for (c = 0; c < CW; c++) {
          var end = c;
          while (end + 1 < CW && set[end + 1] === set[c]) end++;
          var chosen = c + rnd(end - c + 1);
          for (k = c; k <= end; k++) if (k === chosen || rnd(3) === 0) { open2(r, k, r + 1, k); dropped[k] = 1; }
          c = end;
        }
        for (c = 0; c < CW; c++) if (!dropped[c]) set[c] = next++;
      }
    }
    function genWilson() {
      var inTree = blankSeen(), dir = [], r, c, added = 1, total = CW * CW, guard = 0;
      for (r = 0; r < CW; r++) dir.push(new Array(CW).fill(0));
      r = rnd(CW); c = rnd(CW);
      inTree[r][c] = 1; wall[g(r)][g(c)] = 0;
      while (added < total && guard++ < 400000) {
        var sr = rnd(CW), sc = rnd(CW);
        if (inTree[sr][sc]) continue;
        var wr = sr, wc = sc;
        while (!inTree[wr][wc]) {
          var d = rnd(4), nr = wr + DR[d], nc = wc + DC[d];
          if (!inCells(nr, nc)) continue;
          dir[wr][wc] = d; wr = nr; wc = nc;
        }
        wr = sr; wc = sc;
        while (!inTree[wr][wc]) {
          var d2 = dir[wr][wc], nr2 = wr + DR[d2], nc2 = wc + DC[d2];
          inTree[wr][wc] = 1; added++;
          open2(wr, wc, nr2, nc2);
          wr = nr2; wc = nc2;
        }
      }
    }
    function genAldous() {
      var seen = blankSeen(), r = rnd(CW), c = rnd(CW), added = 1, total = CW * CW, guard = 0;
      seen[r][c] = 1; wall[g(r)][g(c)] = 0;
      while (added < total && guard++ < 2000000) {
        var d = rnd(4), nr = r + DR[d], nc = c + DC[d];
        if (!inCells(nr, nc)) continue;
        if (!seen[nr][nc]) { seen[nr][nc] = 1; added++; open2(r, c, nr, nc); }
        r = nr; c = nc;
      }
    }
    function genHuntKill() {
      var seen = blankSeen(), r = rnd(CW), c = rnd(CW);
      seen[r][c] = 1; wall[g(r)][g(c)] = 0;
      for (;;) {
        var order = shuffle([0,1,2,3]), moved = false, i;
        for (i = 0; i < 4; i++) {
          var nr = r + DR[order[i]], nc = c + DC[order[i]];
          if (!inCells(nr, nc) || seen[nr][nc]) continue;
          seen[nr][nc] = 1; open2(r, c, nr, nc); r = nr; c = nc; moved = true; break;
        }
        if (moved) continue;
        var found = false;
        for (var hr = 0; hr < CW && !found; hr++) for (var hc = 0; hc < CW && !found; hc++) {
          if (seen[hr][hc]) continue;
          for (i = 0; i < 4; i++) {
            var ar = hr + DR[i], ac = hc + DC[i];
            if (!inCells(ar, ac) || !seen[ar][ac]) continue;
            seen[hr][hc] = 1; open2(hr, hc, ar, ac); r = hr; c = hc; found = true; break;
          }
        }
        if (!found) return;
      }
    }
    function genBinary() {
      for (var r = 0; r < CW; r++) for (var c = 0; c < CW; c++) {
        var north = r > 0, east = c + 1 < CW;
        wall[g(r)][g(c)] = 0;
        if (north && east) { if (rnd(2)) open2(r, c, r - 1, c); else open2(r, c, r, c + 1); }
        else if (north) open2(r, c, r - 1, c);
        else if (east) open2(r, c, r, c + 1);
      }
    }
    function genSidewinder() {
      var r, c;
      for (c = 0; c < CW; c++) { wall[g(0)][g(c)] = 0; if (c + 1 < CW) open2(0, c, 0, c + 1); }
      for (r = 1; r < CW; r++) {
        var runStart = 0;
        for (c = 0; c < CW; c++) {
          wall[g(r)][g(c)] = 0;
          if (c + 1 < CW && rnd(2)) { open2(r, c, r, c + 1); continue; }
          var pick = runStart + rnd(c - runStart + 1);
          open2(r, pick, r - 1, pick);
          runStart = c + 1;
        }
      }
    }
    function genGrowing() {
      var seen = blankSeen(), list = [], r = rnd(CW), c = rnd(CW);
      seen[r][c] = 1; wall[g(r)][g(c)] = 0; list.push([r, c]);
      while (list.length) {
        var i = (rnd(100) < 60) ? list.length - 1 : rnd(list.length);
        r = list[i][0]; c = list[i][1];
        var order = shuffle([0,1,2,3]), moved = false;
        for (var k = 0; k < 4; k++) {
          var nr = r + DR[order[k]], nc = c + DC[order[k]];
          if (!inCells(nr, nc) || seen[nr][nc]) continue;
          seen[nr][nc] = 1; open2(r, c, nr, nc); list.push([nr, nc]); moved = true; break;
        }
        if (!moved) { list[i] = list[list.length - 1]; list.pop(); }
      }
    }

    function generate() {
      wall = [];
      for (var r = 0; r < N; r++) wall.push(new Array(N).fill(1));
      [genBacktracker, genPrim, genKruskal, genEller, genWilson,
       genAldous, genHuntKill, genBinary, genSidewinder, genGrowing][VAR]();
      wall[1][1] = 0;
      wall[N - 2][N - 2] = 0;
    }
    /* Breadth-first shortest path: gives both the hint and the par count. */
    function solve() {
      var dist = [], q = [[1, 1]], head = 0, r, c, d;
      for (r = 0; r < N; r++) dist.push(new Array(N).fill(-1));
      dist[1][1] = 0;
      while (head < q.length) {
        r = q[head][0]; c = q[head][1]; head++;
        for (d = 0; d < 4; d++) {
          var nr = r + DR[d], nc = c + DC[d];
          if (nr < 0 || nr >= N || nc < 0 || nc >= N) continue;
          if (wall[nr][nc] || dist[nr][nc] >= 0) continue;
          dist[nr][nc] = dist[r][c] + 1;
          q.push([nr, nc]);
        }
      }
      path = [];
      for (r = 0; r < N; r++) path.push(new Array(N).fill(0));
      if (dist[N - 2][N - 2] < 0) return -1;
      r = N - 2; c = N - 2;
      while (!(r === 1 && c === 1)) {
        path[r][c] = 1;
        var found = false;
        for (d = 0; d < 4; d++) {
          var pr2 = r + DR[d], pc2 = c + DC[d];
          if (pr2 < 0 || pr2 >= N || pc2 < 0 || pc2 >= N) continue;
          if (dist[pr2][pc2] === dist[r][c] - 1) { r = pr2; c = pc2; found = true; break; }
        }
        if (!found) break;
      }
      path[1][1] = 1;
      return dist[N - 2][N - 2];
    }
    function reset() {
      generate();
      par = solve();
      pr = 1; pc = 1; steps = 0; hint = false; won = false;
    }
    reset();
    return {
      key: function (k) {
        if (won) { reset(); return; }
        if (k === 'h') { hint = !hint; return; }
        var d = { up: 0, down: 1, left: 2, right: 3 }[k];
        if (d === undefined) return;
        var nr = pr + DR[d], nc = pc + DC[d];
        if (nr < 0 || nr >= N || nc < 0 || nc >= N || wall[nr][nc]) return;
        pr = nr; pc = nc; steps++;
        if (pr === N - 2 && pc === N - 2) {
          won = true;
          host.saveScore(Math.round(par * 1000 / (steps + 1)));
        }
      },
      draw: function (t) {
        t.header('MAZE', ALGO[VAR] + ', ' + N + 'x' + N + ' — arrows move, H hints');
        /* Big mazes scroll; the viewport follows the player. */
        var vh = 18, vw = 66;
        var top = Math.max(0, Math.min(N - vh, pr - (vh >> 1)));
        var left = Math.max(0, Math.min(N - vw, pc - (vw >> 1)));
        if (top < 0) top = 0;
        if (left < 0) left = 0;
        for (var r = 0; r < vh && top + r < N; r++)
          for (var c = 0; c < vw && left + c < N; c++) {
            var gr2 = top + r, gc2 = left + c, ch, fg;
            if (gr2 === pr && gc2 === pc) { ch = '@'; fg = C.cyan; }
            else if (gr2 === N - 2 && gc2 === N - 2) { ch = '$'; fg = C.yellow; }
            else if (wall[gr2][gc2]) { ch = '#'; fg = C.blue; }
            else if (hint && path[gr2][gc2]) { ch = '.'; fg = C.green; }
            else { ch = ' '; fg = C.grey; }
            t.put(7 + c, 4 + r, ch, fg, null, ch === '@' || ch === '$');
          }
        t.text(7, 5 + vh, 'Steps ' + steps + '   Shortest ' + par + '   ' + (hint ? 'hint on ' : '        '), C.fg);
        if (won) t.text(7, 6 + vh, 'Out in ' + steps + ' steps (shortest ' + par + ')! Press any key.', C.green, null, true);
      }
    };
  }
});

/* ------------------------------------------------------------ word search */
reg('wordsearch', {
  title: 'Word Search', help: 'Arrows move · Enter marks each end · Q quits',
  start: function (host, p) {
    var WS = 14, NW = 8;
    var W = window.GICWORDS || { themes: {}, themeOrder: [] };
    var order = W.themeOrder || Object.keys(W.themes);
    var theme = (p.theme && W.themes[p.theme]) ? p.theme : order[(p.variant | 0) % order.length];
    var pool = W.themes[theme] || ['ALPHA','BRAVO','CHARLIE','DELTA'];
    var DR = [0,0,1,-1,1,1,-1,-1], DC = [1,-1,0,0,1,-1,1,-1];
    var grid, words, cr, cc, sr, sc, found;

    function fits(w, r, c, dr, dc) {
      for (var i = 0; i < w.length; i++) {
        var rr = r + dr * i, cc2 = c + dc * i;
        if (rr < 0 || rr >= WS || cc2 < 0 || cc2 >= WS) return false;
        if (grid[rr][cc2] && grid[rr][cc2] !== w[i]) return false;
      }
      return true;
    }
    function build() {
      var i;
      grid = [];
      for (i = 0; i < WS; i++) grid.push(new Array(WS).fill(''));
      words = [];
      for (var tries = 0; tries < 600 && words.length < NW; tries++) {
        var w = pool[rnd(pool.length)].toUpperCase();
        if (w.length < 4 || w.length > WS) continue;
        if (words.some(function (x) { return x.w === w; })) continue;
        for (i = 0; i < 40; i++) {
          var d = rnd(8), r = rnd(WS), c = rnd(WS);
          if (!fits(w, r, c, DR[d], DC[d])) continue;
          for (var k = 0; k < w.length; k++) grid[r + DR[d]*k][c + DC[d]*k] = w[k];
          words.push({ w: w, r: r, c: c, dr: DR[d], dc: DC[d], found: false });
          break;
        }
      }
      for (i = 0; i < WS; i++) for (var j = 0; j < WS; j++)
        if (!grid[i][j]) grid[i][j] = String.fromCharCode(65 + rnd(26));
      cr = 0; cc = 0; sr = -1; sc = -1; found = 0;
    }
    build();
    return {
      key: function (k) {
        if (found >= words.length) { build(); return; }
        if (k === 'up'    && cr > 0)      cr--;
        if (k === 'down'  && cr < WS - 1) cr++;
        if (k === 'left'  && cc > 0)      cc--;
        if (k === 'right' && cc < WS - 1) cc++;
        if (k !== 'enter' && k !== 'space') return;
        if (sr < 0) { sr = cr; sc = cc; return; }
        for (var i = 0; i < words.length; i++) {
          var x = words[i];
          if (x.found) continue;
          var er = x.r + x.dr * (x.w.length - 1), ec = x.c + x.dc * (x.w.length - 1);
          if ((sr === x.r && sc === x.c && cr === er && cc === ec) ||
              (cr === x.r && cc === x.c && sr === er && sc === ec)) {
            x.found = true; found++;
            host.saveScore(found * 100);
            break;
          }
        }
        sr = sc = -1;
      },
      draw: function (t) {
        t.header('WORD SEARCH', theme + ' — arrows move, Enter marks each end of a word');
        var lit = [];
        for (var i = 0; i < WS; i++) lit.push(new Array(WS).fill(false));
        words.forEach(function (x) {
          if (!x.found) return;
          for (var k = 0; k < x.w.length; k++) lit[x.r + x.dr*k][x.c + x.dc*k] = true;
        });
        for (var r = 0; r < WS; r++) for (var c = 0; c < WS; c++)
          t.put(12 + c * 2, 4 + r, grid[r][c], lit[r][c] ? C.green : C.white,
                (r === cr && c === cc) ? '#2b4a6b' : (r === sr && c === sc) ? '#2f6b2f' : null,
                lit[r][c]);
        for (i = 0; i < words.length; i++)
          t.text(46, 4 + i, (words[i].w + '              ').slice(0, 14),
                 words[i].found ? C.green : C.grey, null, words[i].found);
        t.text(12, 5 + WS, 'Found ' + found + ' of ' + words.length + '    ', C.fg);
        if (found >= words.length) t.center(7 + WS, 'All words found! Press any key.', C.green, true);
      }
    };
  }
});

/* ------------------------------------------------------------ match three */
reg('matchthree', {
  title: 'Match Three', help: 'Arrows move · Enter picks two neighbours · Q quits',
  start: function (host, p) {
    var M = 8;
    var TNAME = ['Gems','Fruit','Runes','Candy','Stars','Blocks'];
    var TSYM = [['@','#','$','%','&','*'], ['a','b','c','d','e','f'],
                ['R','U','N','E','S','X'], ['o','O','0','Q','q','8'],
                ['*','+','x','.','^','~'], ['A','B','C','D','E','F']];
    var COL = [C.red, C.green, C.yellow, C.cyan, C.magenta, C.white];
    var ti = TNAME.indexOf(p.theme || '');
    if (ti < 0) ti = (p.variant | 0) % 6;
    var grid, score, moves, cr, cc, sr, sc;

    /* Collapse runs of three or more, refill from the top, repeat. */
    function settle() {
      for (var guard = 0; guard < 40; guard++) {
        var cleared = 0, r, c, j;
        for (r = 0; r < M; r++) for (c = 0; c < M; c++) {
          var n = 1;
          while (c + n < M && grid[r][c+n] === grid[r][c] && grid[r][c] >= 0) n++;
          if (n >= 3 && grid[r][c] >= 0) { for (j = 0; j < n; j++) grid[r][c+j] = -1; cleared += n; }
        }
        for (c = 0; c < M; c++) for (r = 0; r < M; r++) {
          var n2 = 1;
          while (r + n2 < M && grid[r+n2][c] === grid[r][c] && grid[r][c] >= 0) n2++;
          if (n2 >= 3 && grid[r][c] >= 0) { for (j = 0; j < n2; j++) grid[r+j][c] = -1; cleared += n2; }
        }
        if (!cleared) return;
        score += cleared * 10;
        for (c = 0; c < M; c++) {
          var w = M - 1;
          for (r = M - 1; r >= 0; r--) if (grid[r][c] >= 0) grid[w--][c] = grid[r][c];
          while (w >= 0) grid[w--][c] = rnd(6);
        }
      }
    }
    function reset() {
      grid = [];
      for (var r = 0; r < M; r++) { grid.push([]); for (var c = 0; c < M; c++) grid[r].push(rnd(6)); }
      score = 0; moves = 30; cr = 0; cc = 0; sr = -1; sc = -1;
      settle();
      score = 0;
    }
    reset();
    return {
      key: function (k) {
        if (moves <= 0) { reset(); return; }
        if (k === 'up'    && cr > 0)     cr--;
        if (k === 'down'  && cr < M - 1) cr++;
        if (k === 'left'  && cc > 0)     cc--;
        if (k === 'right' && cc < M - 1) cc++;
        if (k !== 'enter' && k !== 'space') return;
        if (sr < 0) { sr = cr; sc = cc; return; }
        if (Math.abs(sr - cr) + Math.abs(sc - cc) === 1) {
          var t = grid[sr][sc];
          grid[sr][sc] = grid[cr][cc];
          grid[cr][cc] = t;
          moves--;
          settle();
          host.saveScore(score);
        }
        sr = sc = -1;
      },
      draw: function (t) {
        t.header('MATCH THREE', TNAME[ti] + ' — arrows move, Enter picks two neighbours to swap');
        for (var r = 0; r < M; r++) for (var c = 0; c < M; c++)
          t.text(26 + c * 3, 5 + r, ' ' + TSYM[ti][grid[r][c]] + ' ', COL[grid[r][c]],
                 (r === cr && c === cc) ? '#2b4a6b' : (r === sr && c === sc) ? '#2f6b2f' : null, true);
        t.text(26, 6 + M, 'Score ' + score + '   Moves left ' + moves + '    ', C.fg);
        if (moves <= 0) t.center(8 + M, 'Out of moves — press any key.', C.white, true);
      }
    };
  }
});

/* ---------------------------------------------------------- peg solitaire */
reg('pegsolitaire', {
  title: 'Peg Solitaire', help: 'Arrows move · Enter picks a peg then its landing hole · Q quits',
  start: function (host, p) {
    var NAMES = ['English','European','Triangular','Diamond','Square'];
    var SHAPES = [
      ['  ooo  ','  ooo  ','ooooooo','ooo.ooo','ooooooo','  ooo  ','  ooo  '],
      ['  ooo  ',' ooooo ','ooooooo','ooo.ooo','ooooooo',' ooooo ','  ooo  '],
      ['    .    ','   o o   ','  o o o  ',' o o o o ','o o o o o'],
      ['   o   ','  ooo  ',' ooooo ','ooo.ooo',' ooooo ','  ooo  ','   o   '],
      ['ooooo','ooooo','oo.oo','ooooo','ooooo']];
    var v = (p.variant >= 0 && p.variant <= 4) ? p.variant : 0;
    var bd, rows, cols, cr, cc, sr, sc, msg;
    var DR = [-2,2,0,0], DC = [0,0,-2,2];

    function reset() {
      bd = SHAPES[v].map(function (r) { return r.split(''); });
      rows = bd.length; cols = bd[0].length;
      cr = 0; cc = 0; sr = -1; sc = -1; msg = null;
    }
    function counts() {
      var pegs = 0, avail = 0, r, c, d;
      for (r = 0; r < rows; r++) for (c = 0; c < bd[r].length; c++) {
        if (bd[r][c] !== 'o') continue;
        pegs++;
        for (d = 0; d < 4; d++) {
          var mr = r + DR[d]/2, mc = c + DC[d]/2, tr = r + DR[d], tc = c + DC[d];
          if (tr < 0 || tr >= rows || tc < 0 || tc >= bd[tr].length) continue;
          if (bd[mr] && bd[mr][mc] === 'o' && bd[tr][tc] === '.') avail++;
        }
      }
      return [pegs, avail];
    }
    reset();
    return {
      key: function (k) {
        if (msg) { reset(); return; }
        if (k === 'up'    && cr > 0)        cr--;
        if (k === 'down'  && cr < rows - 1) cr++;
        if (k === 'left'  && cc > 0)        cc--;
        if (k === 'right' && cc < cols - 1) cc++;
        if (k !== 'enter' && k !== 'space') return;
        if (sr < 0) {
          if (cc < bd[cr].length && bd[cr][cc] === 'o') { sr = cr; sc = cc; }
          return;
        }
        var dr = cr - sr, dc = cc - sc;
        if ((Math.abs(dr) === 2 && dc === 0) || (Math.abs(dc) === 2 && dr === 0)) {
          var mr = sr + dr / 2, mc = sc + dc / 2;
          if (bd[cr][cc] === '.' && bd[mr][mc] === 'o') {
            bd[sr][sc] = '.'; bd[mr][mc] = '.'; bd[cr][cc] = 'o';
          }
        }
        sr = sc = -1;
        var n = counts();
        if (n[0] === 1) { msg = 'One peg left — perfect!'; host.saveScore(1000); }
        else if (n[1] === 0) { msg = 'No moves left — ' + n[0] + ' pegs remain.'; host.saveScore(Math.round(1000 / n[0])); }
      },
      draw: function (t) {
        var n = counts();
        t.header('PEG SOLITAIRE', NAMES[v] + ' board — Enter picks a peg then its landing hole');
        for (var r = 0; r < rows; r++) for (var c = 0; c < bd[r].length; c++) {
          var ch = bd[r][c];
          t.text(32 + c * 2, 5 + r, ch === ' ' ? '  ' : ch + ' ',
                 ch === 'o' ? C.yellow : C.grey,
                 (r === cr && c === cc) ? '#2b4a6b' : (r === sr && c === sc) ? '#2f6b2f' : null,
                 ch === 'o');
        }
        t.text(32, 6 + rows, 'Pegs left: ' + n[0] + '   Moves available: ' + n[1] + '   ', C.fg);
        if (msg) t.center(8 + rows, msg + ' Press any key.', C.white, true);
      }
    };
  }
});


/* ------------------------------------------------------------------- quiz */
reg('quiz', {
  title: 'Quiz', help: 'Press 1-4 to answer · Q quits',
  start: function (host, p) {
    var Q = window.GICQUIZ || { topics: [], items: [] };
    var BAND = ['easy', 'medium', 'hard', 'expert'];
    var ROUND = 6;
    var topic = Q.topics.indexOf(p.theme || '');
    if (topic < 0) topic = (p.variant | 0) % Math.max(1, Q.topics.length);
    /* Catalogue difficulty runs 1..5; the bank has four graded bands. */
    var level = Math.max(0, Math.min(3, (p.difficulty || 1) - 1));

    var pool = Q.items.filter(function (i) { return i.t === topic && i.l === level; });
    if (!pool.length) pool = Q.items.filter(function (i) { return i.t === topic; });
    if (!pool.length) pool = Q.items.slice(0, 6);

    var order, idx, shown, pick, asked, right, done;
    function nextQuestion() {
      shown = shuffle([0, 1, 2, 3]);
      pick = -1;
    }
    function reset() {
      order = shuffle(pool.slice());
      idx = 0; asked = 0; right = 0; done = false;
      nextQuestion();
    }
    reset();
    return {
      key: function (k) {
        if (done) { reset(); return; }
        if (pick >= 0) {
          idx++;
          if (asked >= ROUND || idx >= order.length) {
            done = true;
            host.saveScore(Math.round(right * 100 / Math.max(1, asked)));
            return;
          }
          nextQuestion();
          return;
        }
        if (k < '1' || k > '4') return;
        pick = (+k) - 1;
        asked++;
        if (shown[pick] === order[idx].c) right++;
      },
      draw: function (t) {
        t.header('QUIZ', (Q.topics[topic] || 'Quiz') + ', ' + BAND[level] +
                 ' — press 1-4 to answer');
        if (done) {
          t.center(10, 'You scored ' + right + ' out of ' + asked + '.', C.white, true);
          t.center(12, 'Press any key to play again.', C.dim);
          return;
        }
        var it = order[idx];
        t.text(10, 5, 'Question ' + (asked + (pick < 0 ? 1 : 0)) + ' of ' + ROUND + '    ', C.dim);
        t.text(10, 7, it.q, C.white, null, true);
        for (var j = 0; j < 4; j++) {
          var fg = C.white;
          if (pick >= 0) fg = (shown[j] === it.c) ? C.green : (j === pick ? C.red : C.grey);
          t.text(12, 10 + j * 2, (j + 1) + ') ' + it.a[shown[j]] + '                    ', fg);
        }
        t.text(10, 19, 'Score ' + right + ' of ' + asked + '     ', C.fg);
        if (pick >= 0) t.text(10, 21, 'Press any key for the next question.', C.dim);
      }
    };
  }
});


})();
