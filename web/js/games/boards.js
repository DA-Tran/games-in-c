/* boards.js - the board families: connection games, go, morris, tafl,
 * abstract strategy, backgammon and the tile puzzles. Mirrors
 * src/games/hexconnect.c, classic.c and abstract.c.
 */
(function () {
'use strict';
var G = window.GIC, C = G.COL, reg = G.register, rnd = G.rnd, shuffle = G.shuffle;

/* ====================================================== connection games */
var V_HEX = 0, V_Y = 1, V_HAVANNAH = 2, V_TWIXT = 3;
var DR6 = [-1,-1, 0, 0, 1, 1], DC6 = [0, 1,-1, 1,-1, 0];

reg('hexconnect', {
  title: 'Connection', help: 'Arrows move · Enter places',
  start: function (host, p) {
    var VAR = p.variant | 0, N = p.size | 0;
    if (VAR < 0 || VAR > 3) VAR = 0;
    if (N < 5) N = 11;
    if (N > 19) N = 19;
    var board = [], cr = N >> 1, cc = N >> 1, moves = 0, msg = '', over = 0;
    var NAME = ['Hex', 'Y', 'Havannah', 'TwixT'];
    var parent = [];

    function on(r, c) {
      if (r < 0 || r >= N || c < 0) return false;
      if (VAR === V_Y) return c <= r;
      return c < N;
    }
    function idx(r, c) { return r * N + c; }
    function find(a) { while (parent[a] !== a) { parent[a] = parent[parent[a]]; a = parent[a]; } return a; }
    function unite(a, b) { a = find(a); b = find(b); if (a !== b) parent[a] = b; }
    var CELLS = 19 * 19, E_TOP = CELLS, E_BOT = CELLS + 1, E_L = CELLS + 2, E_R = CELLS + 3;
    function reset() {
      board = [];
      for (var i = 0; i < CELLS; i++) board.push(0);
      cr = N >> 1; cc = N >> 1; moves = 0; over = 0; msg = '';
    }
    function rebuild(who) {
      var r, c, d, i;
      parent = [];
      for (i = 0; i < CELLS + 8; i++) parent.push(i);
      for (r = 0; r < N; r++) for (c = 0; c < N; c++) {
        if (!on(r, c) || board[idx(r, c)] !== who) continue;
        for (d = 0; d < 6; d++) {
          var nr = r + DR6[d], nc = c + DC6[d];
          if (on(nr, nc) && board[idx(nr, nc)] === who) unite(idx(r, c), idx(nr, nc));
        }
      }
    }
    function hexWin(who) {
      rebuild(who);
      for (var r = 0; r < N; r++) for (var c = 0; c < N; c++) {
        if (board[idx(r, c)] !== who) continue;
        if (who === 1) { if (r === 0) unite(idx(r, c), E_TOP); if (r === N - 1) unite(idx(r, c), E_BOT); }
        else { if (c === 0) unite(idx(r, c), E_L); if (c === N - 1) unite(idx(r, c), E_R); }
      }
      return who === 1 ? find(E_TOP) === find(E_BOT) : find(E_L) === find(E_R);
    }
    function yWin(who) {
      rebuild(who);
      for (var r = 0; r < N; r++) for (var c = 0; c <= r; c++) {
        if (board[idx(r, c)] !== who) continue;
        if (r === 0) unite(idx(r, c), E_TOP);
        if (c === 0) unite(idx(r, c), E_L);
        if (c === r) unite(idx(r, c), E_R);
      }
      return find(E_TOP) === find(E_L) && find(E_L) === find(E_R);
    }
    function havWin(who, out) {
      rebuild(who);
      var i, r, c, seen = {}, root;
      for (i = 0; i < CELLS; i++) {
        if (board[i] !== who) continue;
        root = find(i);
        if (seen[root]) continue;
        seen[root] = 1;
        var corners = 0, edges = 0;
        for (r = 0; r < N; r++) for (c = 0; c < N; c++) {
          if (board[idx(r, c)] !== who || find(idx(r, c)) !== root) continue;
          if ((r === 0 || r === N - 1) && (c === 0 || c === N - 1)) corners++;
          else if (r === 0 || r === N - 1 || c === 0 || c === N - 1) edges++;
        }
        if (corners >= 2) { out.how = 'a bridge between two corners'; return true; }
        if (edges >= 3)   { out.how = 'a fork touching three edges';  return true; }
      }
      /* A ring: an unowned cell that cannot reach the border. */
      var vis = {};
      for (r = 1; r < N - 1; r++) for (c = 1; c < N - 1; c++) {
        if (board[idx(r, c)] === who || vis[idx(r, c)]) continue;
        var stack = [idx(r, c)], esc = false;
        vis[idx(r, c)] = 1;
        while (stack.length) {
          var cell = stack.pop(), q = cell / N | 0, w = cell % N, d;
          if (q === 0 || q === N - 1 || w === 0 || w === N - 1) esc = true;
          for (d = 0; d < 6; d++) {
            var nr = q + DR6[d], nc = w + DC6[d];
            if (!on(nr, nc) || vis[idx(nr, nc)] || board[idx(nr, nc)] === who) continue;
            vis[idx(nr, nc)] = 1;
            stack.push(idx(nr, nc));
          }
        }
        if (!esc) { out.how = 'a ring'; return true; }
      }
      return false;
    }
    var KR = [-2,-2,-1,-1,1,1,2,2], KC = [-1,1,-2,2,-2,2,-1,1];
    function twixtWin(who) {
      var i, r, c, d;
      parent = [];
      for (i = 0; i < CELLS + 8; i++) parent.push(i);
      for (r = 0; r < N; r++) for (c = 0; c < N; c++) {
        if (board[idx(r, c)] !== who) continue;
        for (d = 0; d < 8; d++) {
          var nr = r + KR[d], nc = c + KC[d];
          if (nr >= 0 && nr < N && nc >= 0 && nc < N && board[idx(nr, nc)] === who) unite(idx(r, c), idx(nr, nc));
        }
        if (who === 1) { if (r === 0) unite(idx(r, c), E_TOP); if (r === N - 1) unite(idx(r, c), E_BOT); }
        else { if (c === 0) unite(idx(r, c), E_L); if (c === N - 1) unite(idx(r, c), E_R); }
      }
      return who === 1 ? find(E_TOP) === find(E_BOT) : find(E_L) === find(E_R);
    }
    function wins(who, out) {
      out.how = 'a connection';
      if (VAR === V_HEX) return hexWin(who);
      if (VAR === V_Y) return yWin(who);
      if (VAR === V_HAVANNAH) return havWin(who, out);
      return twixtWin(who);
    }
    /* 0-1 shortest path, with a bounded queue and an already-queued flag. */
    function pathCost(who) {
      var dist = [], inq = [], q = [], head = 0, tail = 0, count = 0, r, c, best = 9999;
      for (r = 0; r < CELLS; r++) { dist.push(9999); inq.push(0); }
      for (r = 0; r <= CELLS; r++) q.push(0);
      for (r = 0; r < N; r++) for (c = 0; c < N; c++) {
        var start = (who === 1) ? (r === 0) : (c === 0);
        if (!on(r, c) || !start || board[idx(r, c)] === 3 - who) continue;
        dist[idx(r, c)] = board[idx(r, c)] === who ? 0 : 1;
        if (!inq[idx(r, c)]) { inq[idx(r, c)] = 1; q[tail] = idx(r, c); tail = (tail + 1) % (CELLS + 1); count++; }
      }
      while (count > 0) {
        var cell = q[head];
        head = (head + 1) % (CELLS + 1);
        count--;
        inq[cell] = 0;
        var cq = cell / N | 0, cw = cell % N, d;
        for (d = 0; d < 6; d++) {
          var nr = cq + DR6[d], nc = cw + DC6[d];
          if (!on(nr, nc) || board[idx(nr, nc)] === 3 - who) continue;
          var ni = idx(nr, nc), wt = board[ni] === who ? 0 : 1;
          if (dist[cell] + wt < dist[ni]) {
            dist[ni] = dist[cell] + wt;
            if (!inq[ni]) { inq[ni] = 1; q[tail] = ni; tail = (tail + 1) % (CELLS + 1); count++; }
          }
        }
      }
      for (r = 0; r < N; r++) for (c = 0; c < N; c++) {
        var goal = (who === 1) ? (r === N - 1) : (c === N - 1);
        if (on(r, c) && goal && dist[idx(r, c)] < best) best = dist[idx(r, c)];
      }
      return best;
    }
    function aiMove() {
      var r, c, best = -1, bestscore = -99999;
      for (r = 0; r < N; r++) for (c = 0; c < N; c++) {
        if (!on(r, c) || board[idx(r, c)]) continue;
        board[idx(r, c)] = 2;
        var s = -pathCost(2) * 3 + pathCost(1);
        board[idx(r, c)] = 0;
        if (s > bestscore) { bestscore = s; best = idx(r, c); }
      }
      return best;
    }
    reset();
    return {
      key: function (k) {
        if (over) { host.saveScore(over === 1 ? Math.max(0, 1000 - moves * 5) : moves); reset(); return; }
        if (k === 'up')    { if (cr > 0) cr--; if (!on(cr, cc)) cc = cr; return; }
        if (k === 'down')  { if (cr < N - 1) cr++; return; }
        if (k === 'left')  { if (cc > 0) cc--; return; }
        if (k === 'right') { if (on(cr, cc + 1)) cc++; return; }
        if (k !== 'enter' && k !== 'space') return;
        if (!on(cr, cc) || board[idx(cr, cc)]) return;
        board[idx(cr, cc)] = 1;
        moves++;
        var out = {};
        if (wins(1, out)) { over = 1; msg = 'You win with ' + out.how + '.'; return; }
        var m = aiMove();
        if (m < 0) { over = 1; msg = 'The board is full.'; return; }
        board[m] = 2;
        moves++;
        if (wins(2, out)) { over = 2; msg = 'The opponent wins with ' + out.how + '.'; }
      },
      draw: function (t) {
        var r, c;
        t.header('CONNECTION', NAME[VAR] + ' size ' + N +
                 ' — you are O joining top to bottom, arrows move, Enter places');
        for (r = 0; r < N; r++) {
          var indent = (VAR === V_TWIXT) ? 0 : r;
          for (c = 0; c < N; c++) {
            if (!on(r, c)) continue;
            var v = board[idx(r, c)];
            t.text(4 + indent + c * 2, 3 + r, v === 1 ? 'O' : v === 2 ? 'X' : '.',
                   v === 1 ? C.green : v === 2 ? C.red : C.grey,
                   (r === cr && c === cc) ? '#2b4a6b' : null);
          }
        }
        t.text(4, 4 + N, 'moves ' + moves, C.grey);
        if (VAR === V_HAVANNAH)
          t.text(4, 5 + N, 'win with a ring, a bridge between corners, or a fork on three edges', C.grey);
        t.text(4, 6 + N, msg, over === 1 ? C.green : C.red);
      }
    };
  }
});

/* ==================================================================== GO */
reg('go', {
  title: 'Go', help: 'Arrows move · Enter places · P passes',
  start: function (host, p) {
    var atari = (p.variant | 0) === 1, N = p.size | 0;
    if (N < 5) N = 9;
    if (N > 19) N = 19;
    var b = [], prev = [], cr = N >> 1, cc = N >> 1, cap = [0, 0, 0], passes = 0, over = 0, msg = '';
    var DR = [-1,1,0,0], DC = [0,0,-1,1];
    function idx(r, c) { return r * N + c; }
    function on(r, c) { return r >= 0 && r < N && c >= 0 && c < N; }
    function reset() {
      b = []; prev = [];
      for (var i = 0; i < N * N; i++) { b.push(0); prev.push(0); }
      cap = [0, 0, 0]; passes = 0; over = 0; msg = ''; cr = N >> 1; cc = N >> 1;
    }
    function group(r, c) {
      var who = b[idx(r, c)], stack = [idx(r, c)], seen = {}, cells = [], libs = 0;
      if (!who) return { cells: [], libs: 0 };
      seen[idx(r, c)] = 1;
      while (stack.length) {
        var cell = stack.pop(), q = cell / N | 0, w = cell % N, d;
        cells.push(cell);
        for (d = 0; d < 4; d++) {
          var nr = q + DR[d], nc = w + DC[d];
          if (!on(nr, nc)) continue;
          var ni = idx(nr, nc);
          if (b[ni] === 0) { if (!seen[ni]) { seen[ni] = 1; libs++; } continue; }
          if (b[ni] === who && !seen[ni]) { seen[ni] = 1; stack.push(ni); }
        }
      }
      return { cells: cells, libs: libs };
    }
    function captureAround(r, c, who) {
      var taken = 0, d;
      for (d = 0; d < 4; d++) {
        var nr = r + DR[d], nc = c + DC[d];
        if (!on(nr, nc) || b[idx(nr, nc)] !== 3 - who) continue;
        var g = group(nr, nc);
        if (g.libs === 0) { g.cells.forEach(function (x) { b[x] = 0; }); taken += g.cells.length; }
      }
      return taken;
    }
    function score() {
      var seen = {}, black = 0, white = 0, r, c, i;
      for (i = 0; i < N * N; i++) { if (b[i] === 1) black++; if (b[i] === 2) white++; }
      for (r = 0; r < N; r++) for (c = 0; c < N; c++) {
        if (b[idx(r, c)] || seen[idx(r, c)]) continue;
        var stack = [idx(r, c)], n = 0, touch = 0;
        seen[idx(r, c)] = 1;
        while (stack.length) {
          var cell = stack.pop(), q = cell / N | 0, w = cell % N, d;
          n++;
          for (d = 0; d < 4; d++) {
            var nr = q + DR[d], nc = w + DC[d];
            if (!on(nr, nc)) continue;
            var ni = idx(nr, nc);
            if (b[ni]) { touch |= b[ni]; continue; }
            if (!seen[ni]) { seen[ni] = 1; stack.push(ni); }
          }
        }
        if (touch === 1) black += n; else if (touch === 2) white += n;
      }
      return { black: black, white: white };
    }
    function ai() {
      var r, c, best = -1, bestscore = -9999;
      for (r = 0; r < N; r++) for (c = 0; c < N; c++) {
        if (b[idx(r, c)]) continue;
        var save = b.slice(), s = 0, d;
        b[idx(r, c)] = 2;
        s += captureAround(r, c, 2) * 12;
        if (group(r, c).libs === 0) s -= 50;
        for (d = 0; d < 4; d++) {
          var nr = r + DR[d], nc = c + DC[d];
          if (!on(nr, nc)) { s += 1; continue; }
          if (b[idx(nr, nc)] === 2) s += 2;
          if (b[idx(nr, nc)] === 1) s += 3;
        }
        s += rnd(3);
        b = save;
        if (s > bestscore) { bestscore = s; best = idx(r, c); }
      }
      return best;
    }
    reset();
    return {
      key: function (k) {
        if (over) { var sc = score(); host.saveScore(atari ? (cap[1] ? 500 : 100) : sc.black * 10); reset(); return; }
        if (k === 'up')    { if (cr > 0) cr--; return; }
        if (k === 'down')  { if (cr < N - 1) cr++; return; }
        if (k === 'left')  { if (cc > 0) cc--; return; }
        if (k === 'right') { if (cc < N - 1) cc++; return; }
        if (k === 'p') { passes++; if (passes >= 2) { over = 1; msg = 'Two passes — the game ends.'; } return; }
        if (k !== 'enter' && k !== 'space') return;
        if (b[idx(cr, cc)]) return;
        var save = b.slice();
        b[idx(cr, cc)] = 1;
        var taken = captureAround(cr, cc, 1);
        if (group(cr, cc).libs === 0 && !taken) { b = save; msg = 'Suicide is not allowed.'; return; }
        /* Simple ko: the position may not repeat the previous one. */
        if (b.every(function (x, i) { return x === prev[i]; })) { b = save; msg = 'Ko — try elsewhere.'; return; }
        prev = save;
        cap[1] += taken;
        passes = 0;
        msg = '';
        if (atari && taken) { over = 1; msg = 'You made the first capture.'; return; }
        var m = ai();
        if (m < 0) { over = 1; return; }
        b[m] = 2;
        var t2 = captureAround(m / N | 0, m % N, 2);
        cap[2] += t2;
        if (atari && t2) { over = 2; msg = 'They made the first capture.'; }
      },
      draw: function (t) {
        var r, c, s = score();
        t.header('GO', (atari ? 'Atari Go (first capture wins)' : 'Go') + ' ' + N + 'x' + N +
                 ' — arrows move, Enter places, P passes');
        for (r = 0; r < N; r++) for (c = 0; c < N; c++) {
          var v = b[idx(r, c)];
          t.text(4 + c * 2, 3 + r, v === 1 ? 'O' : v === 2 ? 'X' : '.',
                 v === 1 ? C.green : v === 2 ? C.red : C.grey,
                 (r === cr && c === cc) ? '#2b4a6b' : null);
        }
        t.text(4, 4 + N, 'captures: you ' + cap[1] + ', them ' + cap[2], C.grey);
        if (!atari) t.text(4, 5 + N, 'area: you ' + s.black + ', them ' + s.white, C.grey);
        t.text(4, 7 + N, msg, over === 1 ? C.green : over === 2 ? C.red : C.yellow);
      }
    };
  }
});

/* ================================================================ MORRIS */
var M3 = [[0,1,2],[3,4,5],[6,7,8],[0,3,6],[1,4,7],[2,5,8],[0,4,8],[2,4,6]];
var M9 = [[0,1,2],[3,4,5],[6,7,8],[9,10,11],[12,13,14],[15,16,17],[18,19,20],[21,22,23],
          [0,9,21],[3,10,18],[6,11,15],[1,4,7],[16,19,22],[8,12,17],[5,13,20],[2,14,23]];
var M12 = M9.concat([[0,3,6],[2,5,8],[15,18,21],[17,20,23]]);
var MX = [0,3,6,1,3,5,2,3,4,0,1,2,4,5,6,2,3,4,1,3,5,0,3,6];
var MY = [0,0,0,1,1,1,2,2,2,3,3,3,3,3,3,4,4,4,5,5,5,6,6,6];

reg('morris', {
  title: 'Morris', help: 'Arrows move · Enter places or moves',
  start: function (host, p) {
    var men = p.count | 0;
    if ([3,6,9,12].indexOf(men) < 0) men = 9;
    var mills = men === 3 ? M3 : men === 12 ? M12 : M9;
    var npts = men === 3 ? 9 : 24;
    var mb = [], placed = [0,0,0], onb = [0,0,0], turn = 1, cur = 0, sel = -1, over = 0, msg = '';
    function reset() {
      mb = [];
      for (var i = 0; i < 24; i++) mb.push(0);
      placed = [0,0,0]; onb = [0,0,0]; turn = 1; cur = 0; sel = -1; over = 0; msg = '';
    }
    function inMill(pt, who) {
      return mills.some(function (m) {
        return m.indexOf(pt) >= 0 && mb[m[0]] === who && mb[m[1]] === who && mb[m[2]] === who;
      });
    }
    function adj(a, b) {
      return mills.some(function (m) {
        for (var j = 0; j < 2; j++)
          if ((m[j] === a && m[j+1] === b) || (m[j] === b && m[j+1] === a)) return true;
        return false;
      });
    }
    function takeOne(from) {
      for (var i = 0; i < npts; i++)
        if (mb[i] === from && !inMill(i, from)) { mb[i] = 0; onb[from]--; return; }
    }
    function aiTurn() {
      var i, done = false;
      if (placed[2] < men) {
        for (i = 0; i < npts && !done; i++) {
          if (mb[i]) continue;
          mb[i] = 2;
          if (inMill(i, 2)) done = true;
          else { mb[i] = 1; if (inMill(i, 1)) { mb[i] = 2; done = true; } else mb[i] = 0; }
        }
        if (!done) for (i = 0; i < npts; i++) if (!mb[i]) { mb[i] = 2; break; }
        placed[2]++; onb[2]++;
      } else {
        var from, to, moved = false;
        for (from = 0; from < npts && !moved; from++) {
          if (mb[from] !== 2) continue;
          for (to = 0; to < npts && !moved; to++) {
            if (mb[to]) continue;
            if (onb[2] > 3 && !adj(from, to)) continue;
            mb[from] = 0; mb[to] = 2; moved = true;
          }
        }
        if (!moved) { over = 1; msg = 'They have no move — you win.'; return; }
      }
      for (i = 0; i < npts; i++) if (mb[i] === 2 && inMill(i, 2)) { takeOne(1); break; }
      if (onb[1] < 3 && placed[1] >= men) { over = 2; msg = 'They reduced you below three.'; }
      turn = 1;
    }
    reset();
    return {
      key: function (k) {
        if (over) { host.saveScore(over === 1 ? 500 : 100); reset(); return; }
        if (k === 'left')  { cur = (cur + npts - 1) % npts; return; }
        if (k === 'right') { cur = (cur + 1) % npts; return; }
        if (k === 'up')    { cur = (cur + npts - 3) % npts; return; }
        if (k === 'down')  { cur = (cur + 3) % npts; return; }
        if (k !== 'enter' && k !== 'space') return;
        var phase = placed[1] < men ? 0 : 1;
        var flying = onb[1] === 3 && placed[1] >= men;
        if (phase === 0) {
          if (mb[cur]) return;
          mb[cur] = 1; placed[1]++; onb[1]++;
        } else if (sel < 0) {
          if (mb[cur] !== 1) return;
          sel = cur;
          return;
        } else {
          if (mb[cur]) { sel = -1; return; }
          if (!flying && !adj(sel, cur)) { sel = -1; msg = 'Not adjacent.'; return; }
          mb[sel] = 0; mb[cur] = 1; sel = -1;
        }
        msg = '';
        if (inMill(cur, 1)) { takeOne(2); msg = 'Mill! You take one of theirs.'; }
        if (onb[2] < 3 && placed[2] >= men) { over = 1; msg = 'You reduced them below three.'; return; }
        turn = 2;
        aiTurn();
      },
      draw: function (t) {
        var i;
        t.header('MORRIS', men + " men's morris — " +
                 (placed[1] < men ? 'placing' : (onb[1] === 3 ? 'flying' : 'moving')) +
                 ', arrows move, Enter places or moves');
        for (i = 0; i < npts; i++) {
          var v = mb[i];
          t.text(8 + MX[i] * 5, 3 + MY[i] * 2, v === 1 ? 'O' : v === 2 ? 'X' : '+',
                 v === 1 ? C.green : v === 2 ? C.red : C.grey,
                 i === cur ? '#2b4a6b' : i === sel ? '#2d5a3d' : null);
        }
        t.text(5, 17, 'you: ' + placed[1] + ' placed, ' + onb[1] + ' on board   them: ' +
               placed[2] + ' placed, ' + onb[2] + ' on board', C.grey);
        t.text(5, 19, msg, over === 1 ? C.green : over === 2 ? C.red : C.yellow);
      }
    };
  }
});

/* ================================================================== TAFL */
reg('tafl', {
  title: 'Tafl', help: 'Arrows move · Enter picks then puts',
  start: function (host, p) {
    var variant = p.variant | 0, N = p.size | 0;
    if (N < 7) N = 11;
    if (N > 19) N = 19;
    var tb = [], cr = N >> 1, cc = N >> 1, sel = -1, moves = 0, over = 0, msg = '';
    var DR = [-1,1,0,0], DC = [0,0,-1,1];
    function idx(r, c) { return r * N + c; }
    function on(r, c) { return r >= 0 && r < N && c >= 0 && c < N; }
    function corner(r, c) { return (r === 0 || r === N - 1) && (c === 0 || c === N - 1); }
    function setup() {
      var mid = N >> 1, i;
      tb = [];
      for (i = 0; i < N * N; i++) tb.push(0);
      tb[idx(mid, mid)] = 3;
      var arm = N <= 7 ? 1 : N <= 11 ? 2 : 3, att = N <= 7 ? 2 : N <= 9 ? 3 : N <= 11 ? 4 : 6;
      for (i = 1; i <= arm; i++) {
        tb[idx(mid - i, mid)] = 1; tb[idx(mid + i, mid)] = 1;
        tb[idx(mid, mid - i)] = 1; tb[idx(mid, mid + i)] = 1;
      }
      if (N >= 11) { tb[idx(mid-1,mid-1)] = 1; tb[idx(mid+1,mid+1)] = 1;
                     tb[idx(mid-1,mid+1)] = 1; tb[idx(mid+1,mid-1)] = 1; }
      for (i = 0; i < att; i++) {
        var off = i - (att >> 1);
        if (on(0, mid + off))      tb[idx(0, mid + off)] = 2;
        if (on(N - 1, mid + off))  tb[idx(N - 1, mid + off)] = 2;
        if (on(mid + off, 0))      tb[idx(mid + off, 0)] = 2;
        if (on(mid + off, N - 1))  tb[idx(mid + off, N - 1)] = 2;
      }
      if (variant !== 1) {
        if (on(1, mid))     tb[idx(1, mid)] = 2;
        if (on(N - 2, mid)) tb[idx(N - 2, mid)] = 2;
        if (on(mid, 1))     tb[idx(mid, 1)] = 2;
        if (on(mid, N - 2)) tb[idx(mid, N - 2)] = 2;
      }
      cr = N >> 1; cc = N >> 1; sel = -1; moves = 0; over = 0; msg = '';
    }
    function isAtt(v) { return v === 2; }
    function isDef(v) { return v === 1 || v === 3; }
    function captures(r, c, mover) {
      for (var d = 0; d < 4; d++) {
        var mr = r + DR[d], mc = c + DC[d], fr = r + DR[d] * 2, fc = c + DC[d] * 2;
        if (!on(mr, mc) || !on(fr, fc)) continue;
        var victim = tb[idx(mr, mc)], beyond = tb[idx(fr, fc)];
        if (!victim) continue;
        if (mover === 2 && isDef(victim)) {
          if (victim === 3) continue;                 /* the king needs four */
          if (isAtt(beyond) || corner(fr, fc)) tb[idx(mr, mc)] = 0;
        } else if (mover === 1 && isAtt(victim)) {
          if (isDef(beyond) || corner(fr, fc)) tb[idx(mr, mc)] = 0;
        }
      }
    }
    function kingTaken() {
      for (var r = 0; r < N; r++) for (var c = 0; c < N; c++) {
        if (tb[idx(r, c)] !== 3) continue;
        var around = 0;
        for (var d = 0; d < 4; d++) {
          var nr = r + DR[d], nc = c + DC[d];
          if (!on(nr, nc)) { around++; continue; }
          if (tb[idx(nr, nc)] === 2) around++;
        }
        return around >= 4;
      }
      return true;
    }
    function kingOut() {
      for (var r = 0; r < N; r++) for (var c = 0; c < N; c++)
        if (tb[idx(r, c)] === 3 && corner(r, c)) return true;
      return false;
    }
    function attackers() {
      var save = tb.slice(), r, c, kr = -1, kc = -1, bestFrom = -1, bestTo = -1, bestScore = -99999;
      for (r = 0; r < N; r++) for (c = 0; c < N; c++) if (tb[idx(r, c)] === 3) { kr = r; kc = c; }
      if (kr < 0) { over = 2; return; }
      for (r = 0; r < N; r++) for (c = 0; c < N; c++) {
        if (tb[idx(r, c)] !== 2) continue;
        for (var dir = 0; dir < 4; dir++) {
          var nr = r, nc = c;
          for (var dist = 1; dist < N; dist++) {
            nr += DR[dir]; nc += DC[dir];
            if (!on(nr, nc) || tb[idx(nr, nc)]) break;
            if (corner(nr, nc)) continue;
            var before = tb.filter(function (x) { return x === 1; }).length;
            tb[idx(r, c)] = 0; tb[idx(nr, nc)] = 2;
            captures(nr, nc, 2);
            var after = tb.filter(function (x) { return x === 1; }).length;
            var s = (before - after) * 20 - (Math.abs(nr - kr) + Math.abs(nc - kc));
            if (kingTaken()) s += 1000;
            tb = save.slice();
            if (s > bestScore) { bestScore = s; bestFrom = idx(r, c); bestTo = idx(nr, nc); }
          }
        }
      }
      if (bestFrom < 0) { over = 1; msg = 'They have no move — the king is safe.'; return; }
      tb[bestTo] = tb[bestFrom];
      tb[bestFrom] = 0;
      captures(bestTo / N | 0, bestTo % N, 2);
      moves++;
      if (kingTaken()) { over = 2; msg = 'The king was taken.'; }
    }
    setup();
    return {
      key: function (k) {
        if (over) { host.saveScore(over === 1 ? 500 : moves * 5); setup(); return; }
        if (k === 'up')    { if (cr > 0) cr--; return; }
        if (k === 'down')  { if (cr < N - 1) cr++; return; }
        if (k === 'left')  { if (cc > 0) cc--; return; }
        if (k === 'right') { if (cc < N - 1) cc++; return; }
        if (k !== 'enter' && k !== 'space') return;
        if (sel < 0) { if (isDef(tb[idx(cr, cc)])) sel = idx(cr, cc); return; }
        var sr = sel / N | 0, sc = sel % N, step, ok = true;
        if (tb[idx(cr, cc)]) { sel = -1; return; }
        if (sr !== cr && sc !== cc) { sel = -1; msg = 'Pieces move like rooks.'; return; }
        if (corner(cr, cc) && tb[sel] !== 3) { sel = -1; msg = 'Only the king may enter a corner.'; return; }
        if (sr === cr) { var lo = Math.min(sc, cc) + 1, hi = Math.max(sc, cc);
                         for (step = lo; step < hi; step++) if (tb[idx(sr, step)]) ok = false; }
        else { var lo2 = Math.min(sr, cr) + 1, hi2 = Math.max(sr, cr);
               for (step = lo2; step < hi2; step++) if (tb[idx(step, sc)]) ok = false; }
        if (!ok) { sel = -1; msg = 'The way is blocked.'; return; }
        tb[idx(cr, cc)] = tb[sel];
        tb[sel] = 0;
        sel = -1;
        moves++;
        msg = '';
        captures(cr, cc, 1);
        if (kingOut()) { over = 1; msg = 'The king reached a corner.'; return; }
        attackers();
      },
      draw: function (t) {
        var r, c;
        t.header('TAFL', N + 'x' + N + ' — you defend: get the king (K) to a corner. ' +
                 'Arrows move, Enter picks then puts');
        for (r = 0; r < N; r++) for (c = 0; c < N; c++) {
          var v = tb[idx(r, c)];
          var bg = (r === cr && c === cc) ? '#2b4a6b'
                 : (sel === idx(r, c)) ? '#2d5a3d'
                 : corner(r, c) ? '#3a3a20' : null;
          t.text(4 + c * 2, 3 + r, v === 3 ? 'K' : v === 1 ? 'O' : v === 2 ? 'X' : '.',
                 v === 3 ? C.yellow : v === 1 ? C.green : v === 2 ? C.red : C.grey, bg);
        }
        t.text(4, 4 + N, 'moves ' + moves + ' — corners are highlighted', C.grey);
        t.text(4, 6 + N, msg, over === 1 ? C.green : over === 2 ? C.red : C.yellow);
      }
    };
  }
});

/* ============================================================== ABSTRACT */
var CAP_REPLACE = 0, CAP_JUMP = 1, CAP_NONE = 2;
var AM = [
  ['Breakthrough',     8, 8, 2, 0, CAP_REPLACE, 0],
  ['Clobber',          8, 5, 5, 1, CAP_REPLACE, 1],
  ['Konane',          10,10, 5, 1, CAP_JUMP,    1],
  ['Surakarta',        6, 6, 2, 1, CAP_REPLACE, 1],
  ['Game of Amazons', 10,10, 1, 2, CAP_NONE,    1],
  ['Lines of Action',  8, 8, 1, 2, CAP_REPLACE, 2],
  ['Alquerque',        5, 5, 2, 2, CAP_JUMP,    1],
  ['Fanorona',         9, 5, 2, 2, CAP_JUMP,    1]
];
var D8R = [-1,-1,-1,0,0,1,1,1], D8C = [-1,0,1,-1,1,-1,0,1];

function mover(host, ri) {
  var A = AM[ri], W = A[1], H = A[2], ab = [], cr = H - 1, cc = 0, sr = -1, sc = -1;
  var moves = 0, over = 0, msg = '';
  function on(r, c) { return r >= 0 && r < H && c >= 0 && c < W; }
  function setup() {
    var r, c;
    ab = [];
    for (r = 0; r < H; r++) { ab.push([]); for (c = 0; c < W; c++) ab[r].push(0); }
    if (ri === 1 || ri === 2) {
      for (r = 0; r < H; r++) for (c = 0; c < W; c++) ab[r][c] = ((r + c) % 2) ? 1 : 2;
      if (ri === 2) { ab[H>>1][W>>1] = 0; ab[(H>>1)-1][(W>>1)-1] = 0; }
    } else if (ri === 4) {
      ab[0][3] = 2; ab[0][6] = 2; ab[3][0] = 2; ab[3][9] = 2;
      ab[6][0] = 1; ab[6][9] = 1; ab[9][3] = 1; ab[9][6] = 1;
    } else if (ri === 5) {
      for (c = 1; c < W - 1; c++) { ab[0][c] = 2; ab[H-1][c] = 2; }
      for (r = 1; r < H - 1; r++) { ab[r][0] = 1; ab[r][W-1] = 1; }
    } else {
      for (r = 0; r < A[3]; r++) for (c = 0; c < W; c++) ab[r][c] = 2;
      for (r = H - A[3]; r < H; r++) for (c = 0; c < W; c++) ab[r][c] = 1;
      if (ri >= 6) ab[H>>1][W>>1] = 0;
    }
    cr = H - 1; cc = 0; sr = -1; moves = 0; over = 0; msg = '';
  }
  function legal(fr, fc, tr, tc, who) {
    if (!on(fr, fc) || !on(tr, tc) || ab[fr][fc] !== who) return false;
    var vr = tr - fr, vc = tc - fc, dir = -1, i;
    if (!vr && !vc) return false;
    var n = vr ? Math.abs(vr) : Math.abs(vc);
    for (i = 0; i < 8; i++) if (D8R[i] * n === vr && D8C[i] * n === vc) { dir = i; break; }
    if (dir < 0) return false;
    if (A[4] === 0 && D8R[dir] !== (who === 1 ? -1 : 1)) return false;
    if (A[4] === 1 && D8R[dir] && D8C[dir]) return false;
    if (A[5] === CAP_JUMP) {
      if (n !== 2) return false;
      return ab[fr + D8R[dir]][fc + D8C[dir]] === 3 - who && !ab[tr][tc];
    }
    if (ri === 5) {
      /* Lines of Action: move exactly as many squares as there are men on the line. */
      var count = 0, s;
      for (s = 0; ; s++) { var q = fr + D8R[dir]*s, w = fc + D8C[dir]*s;
                           if (!on(q, w)) break; if (ab[q][w]) count++; }
      for (s = 1; ; s++) { var q2 = fr - D8R[dir]*s, w2 = fc - D8C[dir]*s;
                           if (!on(q2, w2)) break; if (ab[q2][w2]) count++; }
      if (n !== count) return false;
    } else if (A[5] !== CAP_NONE && n !== 1 && ri !== 3) return false;
    for (i = 1; i < n; i++) {
      var q3 = fr + D8R[dir]*i, w3 = fc + D8C[dir]*i;
      if (ab[q3][w3] === 3 - who || ab[q3][w3] === 3) return false;
      if (ab[q3][w3] === who && ri !== 5) return false;
    }
    if (ab[tr][tc] === who || ab[tr][tc] === 3) return false;
    if (ab[tr][tc] === 3 - who) {
      if (A[5] === CAP_NONE) return false;
      if (A[4] === 0 && !D8C[dir]) return false;    /* Breakthrough takes diagonally */
    }
    return true;
  }
  function count(who) { var n = 0, r, c; for (r = 0; r < H; r++) for (c = 0; c < W; c++) if (ab[r][c] === who) n++; return n; }
  function connected(who) {
    var total = count(who), seen = {}, stack = [], found = 0, r, c;
    for (r = 0; r < H && !stack.length; r++) for (c = 0; c < W && !stack.length; c++)
      if (ab[r][c] === who) { stack.push(r * W + c); seen[r * W + c] = 1; }
    while (stack.length) {
      var cell = stack.pop(), q = cell / W | 0, w = cell % W, d;
      found++;
      for (d = 0; d < 8; d++) {
        var nr = q + D8R[d], nc = w + D8C[d];
        if (!on(nr, nc) || seen[nr * W + nc] || ab[nr][nc] !== who) continue;
        seen[nr * W + nc] = 1;
        stack.push(nr * W + nc);
      }
    }
    return total > 0 && found === total;
  }
  function fireArrow(r, c) {
    for (var d = 0; d < 8; d++) {
      var ar = r + D8R[d], ac = c + D8C[d];
      if (on(ar, ac) && !ab[ar][ac]) { ab[ar][ac] = 3; return; }
    }
  }
  setup();
  return {
    key: function (k) {
      if (over) { host.saveScore(over === 1 ? 500 : moves * 5); setup(); return; }
      if (k === 'up')    { if (cr > 0) cr--; return; }
      if (k === 'down')  { if (cr < H - 1) cr++; return; }
      if (k === 'left')  { if (cc > 0) cc--; return; }
      if (k === 'right') { if (cc < W - 1) cc++; return; }
      if (k !== 'enter' && k !== 'space') return;
      if (sr < 0) { if (ab[cr][cc] === 1) { sr = cr; sc = cc; } return; }
      if (!legal(sr, sc, cr, cc, 1)) { sr = -1; msg = 'Not a legal move.'; return; }
      if (A[5] === CAP_JUMP) ab[(sr + cr) >> 1][(sc + cc) >> 1] = 0;
      ab[cr][cc] = 1; ab[sr][sc] = 0;
      if (A[5] === CAP_NONE) fireArrow(cr, cc);
      sr = -1; moves++; msg = '';
      if (A[6] === 0 && cr === 0) { over = 1; msg = 'You reached the far row.'; return; }
      if (A[6] === 2 && connected(1)) { over = 1; msg = 'All your men are joined.'; return; }
      if (!count(2)) { over = 1; msg = 'You took every man.'; return; }
      /* The opponent takes a capture if one exists, else any move. */
      var br = -1, bc = -1, bsr = -1, bsc = -1, bestscore = -9999, r, c, dr, dc;
      for (r = 0; r < H; r++) for (c = 0; c < W; c++) {
        if (ab[r][c] !== 2) continue;
        for (dr = 0; dr < H; dr++) for (dc = 0; dc < W; dc++) {
          if (!legal(r, c, dr, dc, 2)) continue;
          var s = ab[dr][dc] === 1 ? 50 : 0;
          if (A[5] === CAP_JUMP && ab[(r + dr) >> 1][(c + dc) >> 1] === 1) s += 50;
          if (A[6] === 0) s += dr * 2;
          s += rnd(4);
          if (s > bestscore) { bestscore = s; bsr = r; bsc = c; br = dr; bc = dc; }
        }
      }
      if (bsr < 0) { over = 1; msg = 'They have no move.'; return; }
      if (A[5] === CAP_JUMP) ab[(bsr + br) >> 1][(bsc + bc) >> 1] = 0;
      ab[br][bc] = 2; ab[bsr][bsc] = 0;
      if (A[5] === CAP_NONE) fireArrow(br, bc);
      if (A[6] === 0 && br === H - 1) { over = 2; msg = 'They reached your row.'; }
      if (A[6] === 2 && connected(2)) { over = 2; msg = 'They joined all their men.'; }
      if (!count(1)) { over = 2; msg = 'You lost every man.'; }
    },
    draw: function (t) {
      var r, c;
      t.header('ABSTRACT', A[0] + ' — arrows move, Enter picks then puts');
      for (r = 0; r < H; r++) for (c = 0; c < W; c++) {
        var v = ab[r][c];
        t.text(6 + c * 3, 3 + r, v === 1 ? 'O' : v === 2 ? 'X' : v === 3 ? '#' : '.',
               v === 1 ? C.green : v === 2 ? C.red : v === 3 ? C.yellow : C.grey,
               (r === cr && c === cc) ? '#2b4a6b' : (sr === r && sc === c) ? '#2d5a3d' : null);
      }
      t.text(6, 4 + H, 'you ' + count(1) + ', them ' + count(2) + '   moves ' + moves, C.grey);
      if (A[6] === 0) t.text(6, 5 + H, 'reach the far row to win', C.grey);
      if (A[6] === 2) t.text(6, 5 + H, 'gather every man into one group', C.grey);
      t.text(6, 7 + H, msg, over === 1 ? C.green : over === 2 ? C.red : C.yellow);
    }
  };
}

function pentago(host) {
  var b = [], cr = 0, cc = 0, moves = 0, over = 0, msg = '';
  function reset() { b = []; for (var i = 0; i < 6; i++) { b.push([0,0,0,0,0,0]); } cr = 0; cc = 0; moves = 0; over = 0; msg = ''; }
  function twist(q) {
    var br = (q >> 1) * 3, bc = (q % 2) * 3, tmp = [[0,0,0],[0,0,0],[0,0,0]], i, j;
    for (i = 0; i < 3; i++) for (j = 0; j < 3; j++) tmp[j][2 - i] = b[br + i][bc + j];
    for (i = 0; i < 3; i++) for (j = 0; j < 3; j++) b[br + i][bc + j] = tmp[i][j];
  }
  function five() {
    var WR = [0,1,1,1], WC = [1,0,1,-1], who, r, c, d, s;
    for (who = 1; who <= 2; who++)
      for (r = 0; r < 6; r++) for (c = 0; c < 6; c++) for (d = 0; d < 4; d++) {
        var n = 0;
        for (s = 0; s < 5; s++) {
          var nr = r + WR[d] * s, nc = c + WC[d] * s;
          if (nr < 0 || nr > 5 || nc < 0 || nc > 5 || b[nr][nc] !== who) break;
          n++;
        }
        if (n === 5) return who;
      }
    return 0;
  }
  reset();
  return {
    key: function (k) {
      if (over) { host.saveScore(over === 1 ? 500 : moves * 5); reset(); return; }
      if (k === 'up')    { if (cr > 0) cr--; return; }
      if (k === 'down')  { if (cr < 5) cr++; return; }
      if (k === 'left')  { if (cc > 0) cc--; return; }
      if (k === 'right') { if (cc < 5) cc++; return; }
      var acted = false;
      if ((k === 'enter' || k === 'space') && !b[cr][cc]) { b[cr][cc] = 1; moves++; acted = true; }
      if (/^[1-4]$/.test(k)) { twist(+k - 1); acted = true; }
      if (!acted) return;
      var w = five();
      if (w) { over = w === 1 ? 1 : 2; msg = w === 1 ? 'Five in a row — you win.' : 'They lined up five.'; return; }
      var best = -1, bestscore = -1, r, c, d;
      for (r = 0; r < 6; r++) for (c = 0; c < 6; c++) {
        if (b[r][c]) continue;
        var s = 0;
        for (d = 0; d < 8; d++) {
          var nr = r + D8R[d], nc = c + D8C[d];
          if (nr < 0 || nr > 5 || nc < 0 || nc > 5) continue;
          if (b[nr][nc] === 2) s += 3;
          if (b[nr][nc] === 1) s += 2;
        }
        s += rnd(3);
        if (s > bestscore) { bestscore = s; best = r * 6 + c; }
      }
      if (best < 0) { over = 3; msg = 'The board is full.'; return; }
      b[best / 6 | 0][best % 6] = 2;
      moves++;
      twist(rnd(4));
      w = five();
      if (w) { over = w === 1 ? 1 : 2; msg = w === 1 ? 'Five in a row — you win.' : 'They lined up five.'; }
    },
    draw: function (t) {
      var i, j;
      t.header('PENTAGO', 'Place a marble then twist a quadrant — Enter places, 1-4 twists');
      for (i = 0; i < 6; i++) for (j = 0; j < 6; j++)
        t.text(10 + j * 3 + ((j / 3 | 0) * 2), 3 + i + (i / 3 | 0),
               b[i][j] === 1 ? 'O' : b[i][j] === 2 ? 'X' : '.',
               b[i][j] === 1 ? C.green : b[i][j] === 2 ? C.red : C.grey,
               (i === cr && j === cc) ? '#2b4a6b' : null);
      t.text(10, 12, 'moves ' + moves + ' — five in a row wins', C.grey);
      t.text(10, 14, msg, over === 1 ? C.green : over === 2 ? C.red : C.yellow);
    }
  };
}

function quarto(host, gobblet) {
  var b = [], used = [], give = 0, cr = 0, cc = 0, moves = 0, over = 0, msg = '';
  function label(v) {
    return ((v & 1) ? 'T' : 't') + ((v & 2) ? 'R' : 'r') + ((v & 4) ? 'S' : 's') + ((v & 8) ? 'H' : 'h');
  }
  function reset() {
    b = []; used = [];
    for (var i = 0; i < 4; i++) b.push([-1,-1,-1,-1]);
    for (i = 0; i < 16; i++) used.push(false);
    give = rnd(16); cr = 0; cc = 0; moves = 0; over = 0; msg = '';
  }
  function lineWin() {
    var L = [[[0,0],[0,1],[0,2],[0,3]],[[1,0],[1,1],[1,2],[1,3]],[[2,0],[2,1],[2,2],[2,3]],
             [[3,0],[3,1],[3,2],[3,3]],[[0,0],[1,0],[2,0],[3,0]],[[0,1],[1,1],[2,1],[3,1]],
             [[0,2],[1,2],[2,2],[3,2]],[[0,3],[1,3],[2,3],[3,3]],
             [[0,0],[1,1],[2,2],[3,3]],[[0,3],[1,2],[2,1],[3,0]]];
    for (var d = 0; d < L.length; d++) {
      var line = L[d].map(function (q) { return b[q[0]][q[1]]; });
      if (line.some(function (x) { return x < 0; })) continue;
      for (var a = 0; a < 4; a++) {
        var bit = 1 << a, same = true;
        for (var i = 1; i < 4; i++) if ((line[i] & bit) !== (line[0] & bit)) same = false;
        if (same) return true;
      }
    }
    return false;
  }
  reset();
  return {
    key: function (k) {
      if (over) { host.saveScore(over === 1 ? 500 : moves * 10); reset(); return; }
      if (k === 'up')    { if (cr > 0) cr--; return; }
      if (k === 'down')  { if (cr < 3) cr++; return; }
      if (k === 'left')  { if (cc > 0) cc--; return; }
      if (k === 'right') { if (cc < 3) cc++; return; }
      if (k !== 'enter' && k !== 'space') return;
      if (b[cr][cc] >= 0 && !gobblet) { msg = 'That square is taken.'; return; }
      b[cr][cc] = give;
      used[give] = true;
      moves++;
      msg = '';
      if (lineWin()) { over = 1; msg = 'You completed a line.'; return; }
      if (moves >= 16) { over = 3; msg = 'The board filled with no line.'; return; }
      do { give = rnd(16); } while (used[give]);
    },
    draw: function (t) {
      var i, j;
      t.header(gobblet ? 'GOBBLET' : 'QUARTO',
               gobblet ? 'Bigger pieces cover smaller ones — arrows move, Enter places'
                       : 'You must place the piece you were handed — arrows move, Enter places');
      for (i = 0; i < 4; i++) for (j = 0; j < 4; j++)
        t.text(10 + j * 7, 3 + i * 2, b[i][j] < 0 ? '....' : label(b[i][j]),
               b[i][j] < 0 ? C.grey : C.white, (i === cr && j === cc) ? '#2b4a6b' : null);
      t.text(10, 12, 'you must place: ' + label(give), C.yellow);
      t.text(10, 14, 'four sharing any one attribute in a line wins', C.grey);
      t.text(10, 16, msg, over === 1 ? C.green : C.yellow);
    }
  };
}

function quoridor(host) {
  var wallh = [], wallv = [], myr = 8, myc = 4, thr = 0, thc = 4, mw = 10, tw = 10, moves = 0, over = 0, msg = '';
  function reset() {
    wallh = []; wallv = [];
    for (var i = 0; i < 9; i++) { wallh.push([0,0,0,0,0,0,0,0,0]); wallv.push([0,0,0,0,0,0,0,0,0]); }
    myr = 8; myc = 4; thr = 0; thc = 4; mw = 10; tw = 10; moves = 0; over = 0; msg = '';
  }
  reset();
  return {
    key: function (k) {
      if (over) { host.saveScore(over === 1 ? 500 : moves * 5); reset(); return; }
      var acted = false;
      if (k === 'up' && myr > 0 && !wallh[myr - 1][myc]) { myr--; acted = true; }
      else if (k === 'down' && myr < 8 && !wallh[myr][myc]) { myr++; acted = true; }
      else if (k === 'left' && myc > 0 && !wallv[myr][myc - 1]) { myc--; acted = true; }
      else if (k === 'right' && myc < 8 && !wallv[myr][myc]) { myc++; acted = true; }
      else if (k === 'h' && mw && myr < 8) { wallh[myr][myc] = 1; mw--; acted = true; }
      else if (k === 'v' && mw && myc < 8) { wallv[myr][myc] = 1; mw--; acted = true; }
      if (!acted) return;
      moves++;
      if (myr === 0) { over = 1; msg = 'You crossed first.'; return; }
      if (tw && myr < 4 && rnd(100) < 35 && myr > 0) { wallh[myr - 1][myc] = 1; tw--; }
      else if (thr < 8 && !wallh[thr][thc]) thr++;
      else if (thc < 8 && !wallv[thr][thc]) thc++;
      else if (thc > 0 && !wallv[thr][thc - 1]) thc--;
      if (thr === 8) { over = 2; msg = 'They crossed first.'; }
    },
    draw: function (t) {
      var i, j;
      t.header('QUORIDOR', 'Reach the far row — arrows move, H/V drop a fence');
      for (i = 0; i < 9; i++) for (j = 0; j < 9; j++) {
        t.text(8 + j * 4, 3 + i * 2, (i === myr && j === myc) ? 'O' : (i === thr && j === thc) ? 'X' : '.',
               (i === myr && j === myc) ? C.green : (i === thr && j === thc) ? C.red : C.grey);
        if (i < 8 && wallh[i][j]) t.text(8 + j * 4, 4 + i * 2, '===', C.yellow);
        if (j < 8 && wallv[i][j]) t.text(10 + j * 4, 3 + i * 2, '|', C.yellow);
      }
      t.text(8, 22, 'your fences ' + mw + ', theirs ' + tw + '   moves ' + moves, C.grey);
      t.text(8, 24, msg, over === 1 ? C.green : C.red);
    }
  };
}

function graphGame(host, col) {
  var edge = [], region = [], cur = 0, sel = -1, moves = 0, over = 0, msg = '';
  function reset() {
    edge = []; region = [0,0,0,0,0,0,0,0,0];
    for (var i = 0; i < 6; i++) edge.push([0,0,0,0,0,0]);
    cur = 0; sel = -1; moves = 0; over = 0; msg = '';
  }
  function triangle(who) {
    for (var i = 0; i < 6; i++) for (var j = i + 1; j < 6; j++) {
      if (edge[i][j] !== who) continue;
      for (var m = j + 1; m < 6; m++) if (edge[i][m] === who && edge[j][m] === who) return true;
    }
    return false;
  }
  function neighbours(a, b) {
    var dr = ((a / 3) | 0) - ((b / 3) | 0), dc = (a % 3) - (b % 3);
    return dr * dr + dc * dc === 1;
  }
  reset();
  return {
    key: function (k) {
      if (over) { host.saveScore(over === 2 ? 100 : 500); reset(); return; }
      var n = col ? 9 : 6;
      if (k === 'up' || k === 'left')    { cur = (cur + n - 1) % n; return; }
      if (k === 'down' || k === 'right') { cur = (cur + 1) % n; return; }
      if (k !== 'enter' && k !== 'space') return;
      if (col) {
        if (region[cur]) { msg = 'Already coloured.'; return; }
        for (var i = 0; i < 9; i++) if (neighbours(i, cur) && region[i] === 1) { msg = 'That touches your own colour.'; return; }
        region[cur] = 1;
      } else {
        if (sel < 0) { sel = cur; return; }
        var a = Math.min(sel, cur), b2 = Math.max(sel, cur);
        if (a === b2 || edge[a][b2]) { sel = -1; return; }
        edge[a][b2] = 1;
        sel = -1;
        if (triangle(1)) { over = 2; msg = 'You made a triangle — you lose.'; return; }
      }
      moves++;
      msg = '';
      /* The opponent plays by the same rule. */
      var done = false, i2, j2;
      if (col) {
        for (i2 = 0; i2 < 9 && !done; i2++) {
          if (region[i2]) continue;
          var ok = true;
          for (j2 = 0; j2 < 9; j2++) if (neighbours(i2, j2) && region[j2] === 2) ok = false;
          if (ok) { region[i2] = 2; done = true; }
        }
        if (!done) { over = 1; msg = 'They have no safe region left.'; }
      } else {
        var bi = -1, bj = -1;
        for (i2 = 0; i2 < 6 && !done; i2++) for (j2 = i2 + 1; j2 < 6 && !done; j2++) {
          if (edge[i2][j2]) continue;
          edge[i2][j2] = 2;
          var safe = !triangle(2);
          edge[i2][j2] = 0;
          if (safe) { bi = i2; bj = j2; done = true; }
          else if (bi < 0) { bi = i2; bj = j2; }
        }
        if (bi < 0) { over = 1; msg = 'No edges left.'; }
        else { edge[bi][bj] = 2; if (triangle(2)) { over = 1; msg = 'They made a triangle — you win.'; } }
      }
    },
    draw: function (t) {
      var i, j;
      t.header(col ? 'COL' : 'SIM',
               col ? 'Colour a region — you may not touch your own colour'
                   : 'Join two dots — making a triangle in your own colour loses');
      if (col) {
        for (i = 0; i < 9; i++)
          t.text(14 + (i % 3) * 8, 4 + ((i / 3) | 0) * 3, '  ' + (i + 1) + '  ',
                 region[i] === 1 ? C.green : region[i] === 2 ? C.red : C.grey,
                 i === cur ? '#2b4a6b' : null);
      } else {
        for (i = 0; i < 6; i++)
          t.text(12, 3 + i, ' dot ' + (i + 1) + ' ', i === sel ? C.yellow : C.white,
                 i === cur ? '#2b4a6b' : null);
        for (i = 0; i < 6; i++) for (j = i + 1; j < 6; j++)
          if (edge[i][j]) t.text(24 + j * 4, 3 + i, (i + 1) + '-' + (j + 1),
                                 edge[i][j] === 1 ? C.green : C.red);
      }
      t.text(10, 15, 'moves ' + moves, C.grey);
      t.text(10, 17, msg, over === 2 ? C.red : C.green);
    }
  };
}

function halma(host, chinese) {
  var N = chinese ? 9 : 8, sz = 3, b = [], cr = N - 1, cc = N - 1, sr = -1, sc = -1, moves = 0, over = 0;
  function reset() {
    var i, j;
    b = [];
    for (i = 0; i < N; i++) { b.push([]); for (j = 0; j < N; j++) b[i].push(0); }
    for (i = 0; i < sz; i++) for (j = 0; j < sz; j++) { b[N-1-i][N-1-j] = 1; b[i][j] = 2; }
    cr = N - 1; cc = N - 1; sr = -1; moves = 0; over = 0;
  }
  function home() { var n = 0, i, j; for (i = 0; i < sz; i++) for (j = 0; j < sz; j++) if (b[i][j] === 1) n++; return n; }
  reset();
  return {
    key: function (k) {
      if (over) { host.saveScore(Math.max(0, 2000 - moves * 5)); reset(); return; }
      if (k === 'up')    { if (cr > 0) cr--; return; }
      if (k === 'down')  { if (cr < N - 1) cr++; return; }
      if (k === 'left')  { if (cc > 0) cc--; return; }
      if (k === 'right') { if (cc < N - 1) cc++; return; }
      if (k !== 'enter' && k !== 'space') return;
      if (sr < 0) { if (b[cr][cc] === 1) { sr = cr; sc = cc; } return; }
      var dr = cr - sr, dc = cc - sc;
      if (b[cr][cc]) { sr = -1; return; }
      if (Math.abs(dr) > 1 || Math.abs(dc) > 1) {
        if (!(Math.abs(dr) === 2 || dr === 0) || !(Math.abs(dc) === 2 || dc === 0)) { sr = -1; return; }
        if (!b[sr + dr / 2][sc + dc / 2]) { sr = -1; return; }
      }
      b[cr][cc] = 1; b[sr][sc] = 0; sr = -1; moves++;
      if (home() === sz * sz) { over = 1; return; }
      var bi = -1, bj = -1, bsi = -1, bsj = -1, bestscore = -9999, i, j, di, dj;
      for (i = 0; i < N; i++) for (j = 0; j < N; j++) {
        if (b[i][j] !== 2) continue;
        for (di = -2; di <= 2; di++) for (dj = -2; dj <= 2; dj++) {
          var ni = i + di, nj = j + dj;
          if ((!di && !dj) || ni < 0 || ni >= N || nj < 0 || nj >= N || b[ni][nj]) continue;
          if ((Math.abs(di) === 2 || Math.abs(dj) === 2) && !b[i + (di >> 1)][j + (dj >> 1)]) continue;
          var s = ni + nj + rnd(2);
          if (s > bestscore) { bestscore = s; bsi = i; bsj = j; bi = ni; bj = nj; }
        }
      }
      if (bsi >= 0) { b[bi][bj] = 2; b[bsi][bsj] = 0; }
    },
    draw: function (t) {
      var i, j;
      t.header(chinese ? 'CHINESE CHECKERS' : 'HALMA',
               'Move every man into the far corner — arrows move, Enter picks then puts');
      for (i = 0; i < N; i++) for (j = 0; j < N; j++)
        t.text(8 + j * 3, 3 + i, b[i][j] === 1 ? 'O' : b[i][j] === 2 ? 'X' : '.',
               b[i][j] === 1 ? C.green : b[i][j] === 2 ? C.red : C.grey,
               (i === cr && j === cc) ? '#2b4a6b' : (sr === i && sc === j) ? '#2d5a3d' : null);
      t.text(8, 4 + N, home() + ' of ' + (sz * sz) + ' men home   moves ' + moves, C.grey);
      if (over) t.text(8, 6 + N, 'Every man is home — press any key.', C.green);
    }
  };
}

reg('abstract', {
  title: 'Abstract strategy', help: 'Arrows move · Enter picks then puts',
  start: function (host, p) {
    var v = p.variant | 0;
    if (v < 0 || v > 15) v = 0;
    if (v <= 7) return mover(host, v);
    if (v === 8)  return halma(host, 0);
    if (v === 9)  return halma(host, 1);
    if (v === 10) return pentago(host);
    if (v === 11) return quarto(host, 0);
    if (v === 12) return quarto(host, 1);
    if (v === 13) return quoridor(host);
    if (v === 14) return graphGame(host, 0);
    return graphGame(host, 1);
  }
});

/* ============================================================ BACKGAMMON */
var BR = [
  ['Backgammon',  0,0,0,0,0], ['Hypergammon', 1,0,0,0,0], ['Nackgammon', 0,1,0,0,0],
  ['Acey Deucey', 0,0,1,0,0],  ['Plakoto',     0,0,0,1,0], ['Fevga',      0,0,0,0,1],
  ['Nardi',       0,0,0,0,1]
];

reg('backgammon', {
  title: 'Backgammon', help: 'Arrows pick a point · Enter moves',
  start: function (host, p) {
    var v = p.variant | 0;
    if (v < 0 || v > 6) v = 0;
    var B = BR[v], pt = [], dice = [], cur = 1, borne = 0, theirs = 0, over = 0, msg = '';
    function setup() {
      var i;
      pt = [];
      for (i = 0; i < 26; i++) pt.push(0);
      if (B[1]) { pt[1] = 1; pt[2] = 1; pt[3] = 1; pt[24] = -1; pt[23] = -1; pt[22] = -1; }
      else if (B[5]) { pt[24] = 15; pt[12] = -15; }
      else {
        pt[24] = 2; pt[13] = 5; pt[8] = 3; pt[6] = 5;
        pt[1] = -2; pt[12] = -5; pt[17] = -3; pt[19] = -5;
        if (B[2]) { pt[24] = 2; pt[23] = 2; pt[13] = 4; pt[1] = -2; pt[2] = -2; pt[12] = -4; }
      }
      cur = 24; borne = 0; theirs = 0; over = 0; msg = '';
      roll();
    }
    function roll() {
      var a = 1 + rnd(6), b = 1 + rnd(6);
      dice = a === b ? [a, a, a, a] : [a, b];
    }
    function homeCount(who) {
      var i, n = 0;
      for (i = 1; i <= 6; i++) { var val = who > 0 ? pt[i] : -pt[25 - i]; if (val > 0) n += val; }
      return n;
    }
    function totalMen(who) {
      var i, n = 0;
      for (i = 0; i < 26; i++) { var val = pt[i]; if (who > 0 ? val > 0 : val < 0) n += Math.abs(val); }
      return n;
    }
    function opponentTurn() {
      var rolls, i, j;
      var a = 1 + rnd(6), b = 1 + rnd(6);
      rolls = a === b ? [a, a, a, a] : [a, b];
      for (j = 0; j < rolls.length; j++) {
        var moved = false;
        for (i = 1; i <= 24 && !moved; i++) {
          if (pt[i] >= 0) continue;
          var to = i + rolls[j];
          if (to > 24) { if (homeCount(-1) === totalMen(-1)) { pt[i]++; theirs++; moved = true; } continue; }
          if (pt[to] > 1) continue;
          if (pt[to] === 1) { if (B[5] || B[4]) continue; pt[to] = 0; pt[0]++; }
          pt[i]++; pt[to]--;
          moved = true;
        }
      }
      if (theirs >= 15) { over = 2; msg = 'They bore off first.'; }
      roll();
    }
    setup();
    return {
      key: function (k) {
        if (over) { host.saveScore(over === 1 ? 1000 : borne * 60); setup(); return; }
        if (k === 'left')  { cur = cur > 1 ? cur - 1 : 24; return; }
        if (k === 'right') { cur = cur < 24 ? cur + 1 : 1; return; }
        if (k !== 'enter' && k !== 'space') return;
        if (!dice.length) { opponentTurn(); return; }
        var die = dice[0], to;
        if (pt[0] > 0) {
          to = 25 - die;
          if (pt[to] < -1) { dice.shift(); msg = 'That entry point is blocked.'; return; }
          if (pt[to] === -1 && !B[4] && !B[5]) { pt[to] = 0; pt[25]--; }
          pt[to]++; pt[0]--;
        } else {
          if (pt[cur] <= 0) { msg = 'No man of yours there.'; return; }
          to = cur - die;
          if (to <= 0) {
            if (homeCount(1) < totalMen(1)) { msg = 'All fifteen must be home to bear off.'; return; }
            pt[cur]--; borne++;
          } else {
            if (pt[to] < -1) { msg = 'That point is blocked.'; return; }
            if (pt[to] === -1) {
              if (B[5] || B[4]) { msg = 'No hitting in this variant.'; return; }
              pt[to] = 0; pt[25]--;
            }
            pt[cur]--; pt[to]++;
          }
        }
        msg = '';
        dice.shift();
        if (borne >= 15) { over = 1; msg = 'You bore off first.'; return; }
        if (!dice.length) opponentTurn();
      },
      draw: function (t) {
        var i;
        t.header('BACKGAMMON', B[0] + ' — dice ' + dice.join(' and ') +
                 ' — arrows pick a point, Enter moves');
        for (i = 1; i <= 12; i++) {
          t.text(4 + (12 - i) * 5, 3, ('  ' + i).slice(-2), C.grey);
          t.text(4 + (12 - i) * 5, 4, (pt[i] > 0 ? '+' : '') + pt[i],
                 pt[i] > 0 ? C.green : pt[i] < 0 ? C.red : C.grey, i === cur ? '#2b4a6b' : null);
        }
        for (i = 13; i <= 24; i++) {
          t.text(4 + (i - 13) * 5, 8, ('  ' + i).slice(-2), C.grey);
          t.text(4 + (i - 13) * 5, 7, (pt[i] > 0 ? '+' : '') + pt[i],
                 pt[i] > 0 ? C.green : pt[i] < 0 ? C.red : C.grey, i === cur ? '#2b4a6b' : null);
        }
        t.text(4, 10, 'bar: you ' + pt[0] + ', them ' + (-pt[25]) +
               '   borne off: you ' + borne + ', them ' + theirs, C.grey);
        t.text(4, 11, 'you have ' + homeCount(1) + ' men home of ' + totalMen(1) +
               ' — all fifteen must be home to bear off', C.grey);
        if (B[4]) t.text(4, 12, 'Plakoto: a lone man is pinned, not sent to the bar', C.grey);
        if (B[5]) t.text(4, 12, 'both sides run the same way, and no man is ever hit', C.grey);
        t.text(4, 14, msg, over === 1 ? C.green : over === 2 ? C.red : C.yellow);
      }
    };
  }
});

/* ============================================================ TILEPUZZLE */
reg('tilepuzzle', {
  title: 'Tile puzzle', help: 'Arrows move · Enter acts',
  start: function (host, p) {
    var v = p.variant | 0;
    if (v < 0 || v > 5) v = 0;
    var NAME = ['TILE ROTATE','LOOP CLOSER','FLIP GRID','BALL SORT','COLOUR SORT','RUSH HOUR'];
    var grid = [], tube = [], ntube = 6, depth = 4, cr = 0, cc = 0, sel = -1, moves = 0, over = 0;
    function reset() {
      var i, j;
      moves = 0; cr = 0; cc = 0; sel = -1; over = 0;
      if (v === 3 || v === 4) {
        var colours = v === 3 ? 4 : 5, pool = [];
        ntube = colours + 2;
        for (i = 0; i < colours; i++) for (j = 0; j < depth; j++) pool.push(i + 1);
        shuffle(pool);
        tube = [];
        for (i = 0; i < ntube; i++) tube.push([0,0,0,0]);
        var n = 0;
        for (i = 0; i < colours; i++) for (j = 0; j < depth; j++) tube[i][j] = pool[n++];
      } else {
        grid = [];
        for (i = 0; i < 6; i++) { grid.push([]); for (j = 0; j < 6; j++) grid[i].push(v === 5 ? 0 : rnd(v === 2 ? 2 : 4)); }
        if (v === 5) {
          grid[2][0] = 1; grid[2][1] = 1;
          for (i = 0; i < 5; i++) {
            var r = rnd(6), c = rnd(5);
            if (r === 2 && c < 2) continue;
            grid[r][c] = 2; grid[r][c + 1] = 2;
          }
        }
      }
    }
    function sorted() {
      for (var i = 0; i < ntube; i++) {
        var first = 0;
        for (var j = 0; j < depth; j++) {
          if (!tube[i][j]) continue;
          if (!first) first = tube[i][j];
          else if (tube[i][j] !== first) return false;
        }
      }
      return true;
    }
    function allZero() {
      for (var i = 0; i < 6; i++) for (var j = 0; j < 6; j++) if (grid[i][j]) return false;
      return true;
    }
    reset();
    return {
      key: function (k) {
        if (over) { host.saveScore(Math.max(0, 2000 - moves * 5)); reset(); return; }
        if (v === 3 || v === 4) {
          if (k === 'left')  { cc = (cc + ntube - 1) % ntube; return; }
          if (k === 'right') { cc = (cc + 1) % ntube; return; }
          if (k !== 'enter' && k !== 'space') return;
          if (sel < 0) { sel = cc; return; }
          var from = sel, to = cc, fi = -1, ti = -1, j;
          sel = -1;
          if (from === to) return;
          for (j = depth - 1; j >= 0; j--) if (tube[from][j]) { fi = j; break; }
          for (j = 0; j < depth; j++) if (!tube[to][j]) { ti = j; break; }
          if (fi < 0 || ti < 0) return;
          if (ti > 0 && tube[to][ti - 1] !== tube[from][fi]) return;
          tube[to][ti] = tube[from][fi];
          tube[from][fi] = 0;
          moves++;
          if (sorted()) over = 1;
          return;
        }
        if (v === 5) {
          if (k !== 'right') return;
          var rr = -1, c2 = -1, i2, j2;
          for (i2 = 0; i2 < 6; i2++) for (j2 = 0; j2 < 6; j2++)
            if (grid[i2][j2] === 1 && j2 > c2) { rr = i2; c2 = j2; }
          if (rr >= 0 && c2 < 5 && !grid[rr][c2 + 1]) {
            grid[rr][c2 + 1] = 1;
            grid[rr][c2 - 1] = 0;
            moves++;
            if (c2 + 1 === 5) over = 1;
          }
          return;
        }
        if (k === 'up')    { if (cr > 0) cr--; return; }
        if (k === 'down')  { if (cr < 5) cr++; return; }
        if (k === 'left')  { if (cc > 0) cc--; return; }
        if (k === 'right') { if (cc < 5) cc++; return; }
        if (k !== 'enter' && k !== 'space') return;
        if (v === 2) {
          grid[cr][cc] ^= 1;
          if (cr > 0) grid[cr-1][cc] ^= 1;
          if (cr < 5) grid[cr+1][cc] ^= 1;
          if (cc > 0) grid[cr][cc-1] ^= 1;
          if (cc < 5) grid[cr][cc+1] ^= 1;
        } else grid[cr][cc] = (grid[cr][cc] + 1) % 4;
        moves++;
        if (allZero()) over = 1;
      },
      draw: function (t) {
        var i, j;
        t.header('TILE PUZZLE', NAME[v] + ' — ' +
                 ((v === 3 || v === 4) ? 'arrows pick a tube, Enter lifts then pours'
                : v === 5 ? 'right arrow moves the red car to the right edge'
                          : 'arrows move, Enter turns the tile'));
        if (v === 3 || v === 4) {
          var COLS = [C.grey, C.red, C.green, C.yellow, C.blue, C.magenta];
          for (i = 0; i < ntube; i++) {
            t.text(8 + i * 6, 3, ' tube ' + (i + 1), C.white, i === cc ? '#2b4a6b' : null);
            for (j = depth - 1; j >= 0; j--)
              t.text(8 + i * 6, 5 + (depth - 1 - j), tube[i][j] ? ' ## ' : ' .. ',
                     COLS[tube[i][j]], i === sel ? '#2d5a3d' : null);
          }
        } else {
          var FACE = ['\\', '|', '/', '-'];
          for (i = 0; i < 6; i++) for (j = 0; j < 6; j++)
            t.text(10 + j * 4, 4 + i,
                   v === 2 ? (grid[i][j] ? ' ## ' : ' .. ')
                 : v === 5 ? (grid[i][j] === 1 ? ' RR ' : grid[i][j] ? ' ## ' : ' .. ')
                           : FACE[grid[i][j] % 4],
                   v === 5 ? (grid[i][j] === 1 ? C.red : grid[i][j] ? C.yellow : C.grey)
                           : (grid[i][j] ? C.green : C.grey),
                   (i === cr && j === cc) ? '#2b4a6b' : null);
        }
        t.text(8, 13, 'moves ' + moves, C.grey);
        if (over) t.text(8, 15, 'Solved — press any key.', C.green);
      }
    };
  }
});

})();
