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

})();
