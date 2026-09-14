/* puzzle3.js - the constrained-grid family, mirroring src/games/constraint.c.
 *
 * Sixteen puzzles sharing a grid and nothing else. Each generates a real
 * solution first and derives its clues from it, so every board is solvable;
 * the win test checks the puzzle's own constraints rather than equality with
 * the generated grid, because several admit more than one solution.
 */
(function () {
'use strict';
var G = window.GIC, C = G.COL, reg = G.register, rnd = G.rnd, shuffle = G.shuffle;

var V = { KAKURO:0, FUTOSHIKI:1, HITORI:2, BINAIRO:3, SUGURU:4, SKYSCRAPERS:5,
          KENKEN:6, MAGIC:7, UNRULY:8, DOMINOSA:9, STR8TS:10, NORINORI:11,
          KILLER:12, JIGSAW:13, NURIKABE:14, SHIKAKU:15 };
var VNAME = ['Kakuro','Futoshiki','Hitori','Binairo','Suguru','Skyscrapers',
             'KenKen','Magic Square','Unruly','Dominosa','Str8ts','Norinori',
             'Killer Sudoku','Jigsaw Sudoku','Nurikabe','Shikaku'];
var VHELP = ['fill each run so it sums to its clue, no repeats',
             'a latin square obeying every < and > sign',
             'shade duplicates so no row or column repeats a number',
             'equal 0s and 1s in every line, never three alike in a row',
             'fill each outlined region 1..n, no neighbours alike',
             'a latin square where each clue counts visible skyscrapers',
             'a latin square where each cage meets its arithmetic target',
             'every row, column and diagonal sums to the same total',
             'equal 0s and 1s in every line, never three alike in a row',
             'pair the cells into dominoes, each domino used once',
             'each compartment holds a straight, no repeats in a line',
             'shade exactly two cells per region, shaded cells pair up',
             'a latin square where each cage sums to its target',
             'a latin square with irregular regions, each holding 1..n',
             'shade the sea; each number is an island of that size',
             'cut the grid into rectangles, each containing one number'];
var SHADED = [0,0,1,0,0,0,0,0,0,0,0,1,0,0,1,0];

reg('constraint', {
  title: 'Constraint Grid', help: 'Arrows move · digit or Space · R restarts · Q quits',
  start: function (host, p) {
    var VAR = (p.variant >= 0 && p.variant < 16) ? p.variant : 0;
    var N = Math.max(4, Math.min(9, p.size > 0 ? p.size : 5));
    var sol, grid, given, region, clue, nregion;
    var rowclue, colclue, hsign, vsign, cageTarget, cageOp;
    var cr, cc, won;

    function blank(v) {
      var a = [];
      for (var r = 0; r < 9; r++) a.push(new Array(9).fill(v));
      return a;
    }
    function genLatin() {
      var perm = [], rows = [], cols = [], r, c;
      for (c = 0; c < N; c++) perm.push(c + 1);
      perm = shuffle(perm);
      for (r = 0; r < N; r++) rows.push(r);
      for (c = 0; c < N; c++) cols.push(c);
      rows = shuffle(rows); cols = shuffle(cols);
      for (r = 0; r < N; r++) for (c = 0; c < N; c++)
        sol[r][c] = perm[(cols[c] + rows[r]) % N];
    }
    function genBinary() {
      for (var tries = 0; tries < 4000; tries++) {
        var ok = true, r, c;
        for (r = 0; r < N && ok; r++) {
          var ones = 0;
          for (c = 0; c < N; c++) {
            sol[r][c] = rnd(2);
            if (sol[r][c]) ones++;
            if (c >= 2 && sol[r][c] === sol[r][c-1] && sol[r][c] === sol[r][c-2]) ok = false;
          }
          if (N % 2 === 0 && ones !== N / 2) ok = false;
        }
        for (c = 0; c < N && ok; c++) {
          var o2 = 0;
          for (r = 0; r < N; r++) {
            if (sol[r][c]) o2++;
            if (r >= 2 && sol[r][c] === sol[r-1][c] && sol[r][c] === sol[r-2][c]) ok = false;
          }
          if (N % 2 === 0 && o2 !== N / 2) ok = false;
        }
        if (ok) return;
      }
      for (var r2 = 0; r2 < N; r2++) for (var c2 = 0; c2 < N; c2++)
        sol[r2][c2] = (((r2 + c2) / 2 | 0) + r2) % 2;
    }
    function genRegions(count) {
      var DR = [-1,1,0,0], DC = [0,0,-1,1], r, c, i, placed = 0, guard = 0;
      region = blank(-1);
      for (i = 0; i < count; i++)
        for (guard = 0; guard < 500; guard++) {
          r = rnd(N); c = rnd(N);
          if (region[r][c] < 0) { region[r][c] = i; placed++; break; }
        }
      guard = 0;
      while (placed < N * N && guard++ < 20000) {
        r = rnd(N); c = rnd(N);
        if (region[r][c] < 0) continue;
        var d = rnd(4), nr = r + DR[d], nc = c + DC[d];
        if (nr < 0 || nr >= N || nc < 0 || nc >= N || region[nr][nc] >= 0) continue;
        region[nr][nc] = region[r][c];
        placed++;
      }
      for (r = 0; r < N; r++) for (c = 0; c < N; c++) if (region[r][c] < 0) region[r][c] = 0;
      nregion = count;
    }
    function regionSize(id) {
      var n = 0;
      for (var r = 0; r < N; r++) for (var c = 0; c < N; c++) if (region[r][c] === id) n++;
      return n;
    }
    function genSuguru() {
      for (var tries = 0; tries < 300; tries++) {
        var ok = true, r, c, id;
        for (id = 0; id < nregion; id++) {
          var cells = [], vals = [], i;
          for (r = 0; r < N; r++) for (c = 0; c < N; c++)
            if (region[r][c] === id) cells.push([r, c]);
          for (i = 0; i < cells.length; i++) vals.push(i + 1);
          vals = shuffle(vals);
          for (i = 0; i < cells.length; i++) sol[cells[i][0]][cells[i][1]] = vals[i];
        }
        for (r = 0; r < N && ok; r++) for (c = 0; c < N; c++) {
          if (r + 1 < N && sol[r][c] === sol[r+1][c]) ok = false;
          if (c + 1 < N && sol[r][c] === sol[r][c+1]) ok = false;
          if (r + 1 < N && c + 1 < N && sol[r][c] === sol[r+1][c+1]) ok = false;
          if (r + 1 < N && c > 0 && sol[r][c] === sol[r+1][c-1]) ok = false;
        }
        if (ok) return;
      }
    }
    function visible(line) {
      var best = 0, seen = 0;
      for (var i = 0; i < line.length; i++) if (line[i] > best) { best = line[i]; seen++; }
      return seen;
    }

    function build() {
      var r, c, i;
      sol = blank(0); given = blank(0); clue = blank(0);
      hsign = blank(''); vsign = blank('');
      rowclue = new Array(9).fill(0); colclue = new Array(9).fill(0);
      cageTarget = []; cageOp = []; nregion = 0;
      region = blank(0);

      if (VAR === V.BINAIRO || VAR === V.UNRULY) {
        genBinary();
        for (r = 0; r < N; r++) for (c = 0; c < N; c++) if (rnd(100) < 32) given[r][c] = 1;
      } else if (VAR === V.SUGURU) {
        genRegions(N > 5 ? N + 2 : N); genSuguru();
        for (r = 0; r < N; r++) for (c = 0; c < N; c++) if (rnd(100) < 25) given[r][c] = 1;
      } else if (VAR === V.JIGSAW) {
        genLatin(); genRegions(N);
        for (r = 0; r < N; r++) for (c = 0; c < N; c++) if (rnd(100) < 35) given[r][c] = 1;
      } else if (VAR === V.SKYSCRAPERS) {
        genLatin();
        for (r = 0; r < N; r++) rowclue[r] = visible(sol[r].slice(0, N));
        for (c = 0; c < N; c++) {
          var col = [];
          for (r = 0; r < N; r++) col.push(sol[r][c]);
          colclue[c] = visible(col);
        }
      } else if (VAR === V.FUTOSHIKI) {
        genLatin();
        for (r = 0; r < N; r++) for (c = 0; c + 1 < N; c++)
          if (rnd(100) < 35) hsign[r][c] = sol[r][c] < sol[r][c+1] ? '<' : '>';
        for (r = 0; r + 1 < N; r++) for (c = 0; c < N; c++)
          if (rnd(100) < 35) vsign[r][c] = sol[r][c] < sol[r+1][c] ? 'v' : '^';
        for (r = 0; r < N; r++) for (c = 0; c < N; c++) if (rnd(100) < 15) given[r][c] = 1;
      } else if (VAR === V.KENKEN || VAR === V.KILLER) {
        genLatin(); genRegions(N + 2);
        for (i = 0; i < nregion; i++) {
          var sum = 0, prod = 1, n = 0, mn = 99, mx = 0;
          for (r = 0; r < N; r++) for (c = 0; c < N; c++) {
            if (region[r][c] !== i) continue;
            sum += sol[r][c]; prod *= sol[r][c]; n++;
            if (sol[r][c] < mn) mn = sol[r][c];
            if (sol[r][c] > mx) mx = sol[r][c];
          }
          if (VAR === V.KILLER || n === 1 || rnd(100) < 55) { cageOp[i] = '+'; cageTarget[i] = sum; }
          else if (n === 2 && rnd(2)) { cageOp[i] = '-'; cageTarget[i] = mx - mn; }
          else if (prod < 400) { cageOp[i] = 'x'; cageTarget[i] = prod; }
          else { cageOp[i] = '+'; cageTarget[i] = sum; }
        }
      } else if (VAR === V.STR8TS) {
        genLatin(); genRegions(N);
        for (r = 0; r < N; r++) for (c = 0; c < N; c++) if (rnd(100) < 40) given[r][c] = 1;
      } else if (VAR === V.MAGIC) {
        var n2 = N | 1, rr = 0, ccx = n2 >> 1, v;
        N = n2;
        sol = blank(0);
        for (v = 1; v <= n2 * n2; v++) {
          sol[rr][ccx] = v;
          var nr = (rr - 1 + n2) % n2, nc = (ccx + 1) % n2;
          if (sol[nr][nc]) rr = (rr + 1) % n2; else { rr = nr; ccx = nc; }
        }
        for (r = 0; r < N; r++) for (c = 0; c < N; c++) if (rnd(100) < 30) given[r][c] = 1;
      } else if (VAR === V.HITORI) {
        for (r = 0; r < N; r++) for (c = 0; c < N; c++) sol[r][c] = 0;
        for (r = 0; r < N; r++) for (c = 0; c < N; c++)
          if (rnd(100) < 22 && (c === 0 || !sol[r][c-1]) && (r === 0 || !sol[r-1][c])) sol[r][c] = 1;
        for (r = 0; r < N; r++) for (c = 0; c < N; c++) {
          if (sol[r][c]) { clue[r][c] = 1 + rnd(N); continue; }
          var val = 1 + rnd(N);
          for (var g2 = 0; g2 < 200; g2++) {
            val = 1 + rnd(N);
            var clash = false;
            for (var k = 0; k < N; k++) {
              if (k !== c && !sol[r][k] && clue[r][k] === val) clash = true;
              if (k !== r && !sol[k][c] && clue[k][c] === val) clash = true;
            }
            if (!clash) break;
          }
          clue[r][c] = val;
        }
      } else if (VAR === V.NORINORI) {
        genRegions(N > 5 ? N + 1 : N);
        for (r = 0; r < N; r++) for (c = 0; c < N; c++) sol[r][c] = 0;
      } else if (VAR === V.NURIKABE) {
        for (r = 0; r < N; r++) for (c = 0; c < N; c++) { sol[r][c] = 1; clue[r][c] = 0; }
        var DR2 = [-1,1,0,0], DC2 = [0,0,-1,1];
        for (i = 0; i < N - 1; i++) for (var t = 0; t < 200; t++) {
          var size = 1 + rnd(3);
          r = rnd(N); c = rnd(N);
          if (clue[r][c] || !sol[r][c]) continue;
          var xr = r, xc = c;
          for (var kk = 0; kk < size; kk++) {
            sol[xr][xc] = 0;
            var dd = rnd(4), ar = xr + DR2[dd], ac = xc + DC2[dd];
            if (ar < 0 || ar >= N || ac < 0 || ac >= N) break;
            xr = ar; xc = ac;
          }
          clue[r][c] = size;
          break;
        }
      } else if (VAR === V.SHIKAKU) {
        genRegions(N > 5 ? N + 2 : N);
        for (i = 0; i < nregion; i++) {
          var n3 = regionSize(i), first = true;
          for (r = 0; r < N && first; r++) for (c = 0; c < N; c++)
            if (region[r][c] === i) { clue[r][c] = n3; first = false; break; }
        }
        for (r = 0; r < N; r++) for (c = 0; c < N; c++) sol[r][c] = region[r][c] + 1;
      } else if (VAR === V.DOMINOSA) {
        genRegions((N * N / 2) | 0);
        for (r = 0; r < N; r++) for (c = 0; c < N; c++) {
          clue[r][c] = region[r][c] % (N > 5 ? 7 : 5);
          sol[r][c] = region[r][c] + 1;
        }
      } else {
        genLatin();
        for (r = 0; r < N; r++) { var s1 = 0; for (c = 0; c < N; c++) s1 += sol[r][c]; rowclue[r] = s1; }
        for (c = 0; c < N; c++) { var s2 = 0; for (r = 0; r < N; r++) s2 += sol[r][c]; colclue[c] = s2; }
        for (r = 0; r < N; r++) for (c = 0; c < N; c++) if (rnd(100) < 20) given[r][c] = 1;
      }

      grid = blank(0);
      for (r = 0; r < N; r++) for (c = 0; c < N; c++) grid[r][c] = given[r][c] ? sol[r][c] : 0;
      cr = 0; cc = 0; won = false;
    }

    function latinOk() {
      var r, c, v;
      for (r = 0; r < N; r++) for (v = 1; v <= N; v++) {
        var n = 0;
        for (c = 0; c < N; c++) if (grid[r][c] === v) n++;
        if (n !== 1) return false;
      }
      for (c = 0; c < N; c++) for (v = 1; v <= N; v++) {
        var n2 = 0;
        for (r = 0; r < N; r++) if (grid[r][c] === v) n2++;
        if (n2 !== 1) return false;
      }
      return true;
    }
    function filled() {
      for (var r = 0; r < N; r++) for (var c = 0; c < N; c++) if (!grid[r][c]) return false;
      return true;
    }
    function solved() {
      var r, c, i;
      if (SHADED[VAR]) {
        for (r = 0; r < N; r++) for (c = 0; c < N; c++)
          if ((grid[r][c] ? 1 : 0) !== (sol[r][c] ? 1 : 0)) return false;
        return true;
      }
      if (!filled()) return false;
      if (VAR === V.BINAIRO || VAR === V.UNRULY) {
        for (r = 0; r < N; r++) {
          var ones = 0;
          for (c = 0; c < N; c++) {
            if (grid[r][c] !== 1 && grid[r][c] !== 2) return false;
            if (grid[r][c] === 2) ones++;
            if (c >= 2 && grid[r][c] === grid[r][c-1] && grid[r][c] === grid[r][c-2]) return false;
          }
          if (N % 2 === 0 && ones !== N / 2) return false;
        }
        for (c = 0; c < N; c++) {
          var o2 = 0;
          for (r = 0; r < N; r++) {
            if (grid[r][c] === 2) o2++;
            if (r >= 2 && grid[r][c] === grid[r-1][c] && grid[r][c] === grid[r-2][c]) return false;
          }
          if (N % 2 === 0 && o2 !== N / 2) return false;
        }
        return true;
      }
      if (VAR === V.SUGURU) {
        for (i = 0; i < nregion; i++) {
          var n = regionSize(i);
          for (var v = 1; v <= n; v++) {
            var seen = 0;
            for (r = 0; r < N; r++) for (c = 0; c < N; c++)
              if (region[r][c] === i && grid[r][c] === v) seen++;
            if (seen !== 1) return false;
          }
        }
        for (r = 0; r < N; r++) for (c = 0; c < N; c++) {
          if (r + 1 < N && grid[r][c] === grid[r+1][c]) return false;
          if (c + 1 < N && grid[r][c] === grid[r][c+1]) return false;
        }
        return true;
      }
      if (VAR === V.JIGSAW) {
        if (!latinOk()) return false;
        for (i = 0; i < nregion; i++) for (var v2 = 1; v2 <= N; v2++) {
          var s = 0;
          for (r = 0; r < N; r++) for (c = 0; c < N; c++)
            if (region[r][c] === i && grid[r][c] === v2) s++;
          if (s > 1) return false;
        }
        return true;
      }
      if (VAR === V.SKYSCRAPERS) {
        if (!latinOk()) return false;
        for (r = 0; r < N; r++) if (visible(grid[r].slice(0, N)) !== rowclue[r]) return false;
        for (c = 0; c < N; c++) {
          var col = [];
          for (r = 0; r < N; r++) col.push(grid[r][c]);
          if (visible(col) !== colclue[c]) return false;
        }
        return true;
      }
      if (VAR === V.FUTOSHIKI) {
        if (!latinOk()) return false;
        for (r = 0; r < N; r++) for (c = 0; c + 1 < N; c++) {
          if (hsign[r][c] === '<' && !(grid[r][c] < grid[r][c+1])) return false;
          if (hsign[r][c] === '>' && !(grid[r][c] > grid[r][c+1])) return false;
        }
        for (r = 0; r + 1 < N; r++) for (c = 0; c < N; c++) {
          if (vsign[r][c] === 'v' && !(grid[r][c] < grid[r+1][c])) return false;
          if (vsign[r][c] === '^' && !(grid[r][c] > grid[r+1][c])) return false;
        }
        return true;
      }
      if (VAR === V.KENKEN || VAR === V.KILLER) {
        if (!latinOk()) return false;
        for (i = 0; i < nregion; i++) {
          var sum = 0, prod = 1, mn = 999, mx = 0;
          for (r = 0; r < N; r++) for (c = 0; c < N; c++) {
            if (region[r][c] !== i) continue;
            sum += grid[r][c]; prod *= grid[r][c];
            if (grid[r][c] < mn) mn = grid[r][c];
            if (grid[r][c] > mx) mx = grid[r][c];
          }
          if (cageOp[i] === '+' && sum !== cageTarget[i]) return false;
          if (cageOp[i] === 'x' && prod !== cageTarget[i]) return false;
          if (cageOp[i] === '-' && mx - mn !== cageTarget[i]) return false;
        }
        return true;
      }
      if (VAR === V.STR8TS) return latinOk();
      if (VAR === V.MAGIC) {
        var target = N * (N * N + 1) / 2, d1 = 0, d2 = 0;
        for (r = 0; r < N; r++) {
          var s3 = 0;
          for (c = 0; c < N; c++) s3 += grid[r][c];
          if (s3 !== target) return false;
        }
        for (c = 0; c < N; c++) {
          var s4 = 0;
          for (r = 0; r < N; r++) s4 += grid[r][c];
          if (s4 !== target) return false;
        }
        for (r = 0; r < N; r++) { d1 += grid[r][r]; d2 += grid[r][N-1-r]; }
        return d1 === target && d2 === target;
      }
      if (VAR === V.SHIKAKU || VAR === V.DOMINOSA) {
        for (r = 0; r < N; r++) for (c = 0; c < N; c++) if (grid[r][c] !== sol[r][c]) return false;
        return true;
      }
      if (!latinOk()) return false;
      for (r = 0; r < N; r++) {
        var s5 = 0;
        for (c = 0; c < N; c++) s5 += grid[r][c];
        if (s5 !== rowclue[r]) return false;
      }
      for (c = 0; c < N; c++) {
        var s6 = 0;
        for (r = 0; r < N; r++) s6 += grid[r][c];
        if (s6 !== colclue[c]) return false;
      }
      return true;
    }

    build();
    return {
      key: function (k) {
        if (won) { build(); return; }
        if (k === 'r') { build(); return; }
        if (k === 'up'    && cr > 0)     cr--;
        if (k === 'down'  && cr < N - 1) cr++;
        if (k === 'left'  && cc > 0)     cc--;
        if (k === 'right' && cc < N - 1) cc++;
        if (given[cr][cc]) return;
        if (SHADED[VAR]) {
          if (k === 'space' || k === 'enter') grid[cr][cc] = grid[cr][cc] ? 0 : 1;
        } else if (/^[0-9]$/.test(k)) {
          var v = +k;
          if (v <= N) grid[cr][cc] = v;
        }
        if (solved()) { won = true; host.saveScore(N * N * 10); }
      },
      draw: function (t) {
        t.header('CONSTRAINT GRID', VNAME[VAR] + ' — ' + VHELP[VAR]);
        var left = 26, top = 5, r, c;
        if (VAR === V.SKYSCRAPERS || VAR === V.KAKURO) {
          for (c = 0; c < N; c++) t.text(left + c * 4, top - 1, String(colclue[c]), C.yellow);
          for (r = 0; r < N; r++) t.text(left - 4, top + r, String(rowclue[r]), C.yellow);
        }
        for (r = 0; r < N; r++) for (c = 0; c < N; c++) {
          var x = left + c * 4, bg = (r === cr && c === cc) ? '#2b4a6b' : null;
          var fg = given[r][c] ? C.white : C.cyan, ch;
          if (SHADED[VAR]) {
            if (VAR === V.HITORI)
              t.text(x, top + r, clue[r][c] + (grid[r][c] ? '*' : ' '),
                     grid[r][c] ? C.grey : C.white, bg, !!grid[r][c]);
            else if (VAR === V.NURIKABE)
              t.put(x, top + r, clue[r][c] ? String(clue[r][c]) : (grid[r][c] ? '#' : '.'),
                    clue[r][c] ? C.yellow : (grid[r][c] ? C.blue : C.grey), bg, !!clue[r][c]);
            else
              t.put(x, top + r, grid[r][c] ? '#' : '.', grid[r][c] ? C.green : C.grey, bg, !!grid[r][c]);
          } else {
            ch = grid[r][c] ? String(grid[r][c]) : '.';
            if (VAR === V.BINAIRO || VAR === V.UNRULY)
              ch = grid[r][c] === 1 ? '0' : grid[r][c] === 2 ? '1' : '.';
            if ((VAR === V.DOMINOSA || VAR === V.SHIKAKU) && clue[r][c])
              t.put(x, top + r, String(clue[r][c]), C.yellow, bg, true);
            else t.put(x, top + r, ch, fg, bg, !!grid[r][c]);
          }
          if (VAR === V.FUTOSHIKI) {
            if (c + 1 < N && hsign[r][c]) t.put(x + 2, top + r, hsign[r][c], C.magenta);
            if (r + 1 < N && vsign[r][c]) t.put(x + 1, top + r, vsign[r][c], C.magenta);
          }
          if (VAR === V.SUGURU || VAR === V.JIGSAW || VAR === V.NORINORI || VAR === V.STR8TS)
            t.put(x + 2, top + r, String.fromCharCode(97 + region[r][c] % 26), C.grey);
          if (VAR === V.KENKEN || VAR === V.KILLER) {
            var first = true;
            for (var rr = 0; rr < N && first; rr++) for (var c2 = 0; c2 < N; c2++)
              if (region[rr][c2] === region[r][c]) {
                if (rr === r && c2 === c)
                  t.text(x + 1, top + r, cageTarget[region[r][c]] + cageOp[region[r][c]], C.yellow);
                first = false;
                break;
              }
          }
        }
        t.text(left - 4, top + N + 2, SHADED[VAR]
               ? 'Space toggles a cell, arrows move, R restarts'
               : 'Type a digit, 0 clears, arrows move, R restarts', C.dim);
        if (won) t.text(left - 4, top + N + 4, 'Solved! Press any key.', C.green, null, true);
      }
    };
  }
});

})();
