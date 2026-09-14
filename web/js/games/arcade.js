/* arcade.js - real-time ports of src/games/{snake,tetris,pong,breakout,
 * invaders,dino,flappy,frogger,pacman,asteroids}.c
 *
 * Real-time games declare realtime:true and expose tick(); the host drives
 * them on a fixed timestep, exactly as the C versions use now_ms() deltas.
 */
(function () {
'use strict';
var G = window.GIC, C = G.COL, reg = G.register, rnd = G.rnd;

/* ------------------------------------------------------------------ snake */
reg('snake', {
  title: 'Snake', help: 'Arrows steer · Q quits',
  start: function (host, p) {
    var W = 40, H = 20;
    var VAR = (p.variant >= 0 && p.variant <= 7) ? p.variant : 0;
    var SUB = ['Arrows steer',
               'WRAP: walls are open — you reappear on the far side',
               'MAZE: obstacles block the arena',
               'SPEED: starts fast and keeps accelerating',
               'PORTAL: the two rings are linked',
               'SHRINKING: the arena closes in as you eat',
               'POISON: red food kills — eat only the yellow',
               'NIBBLES: clear the target, then the maze grows'][VAR];
    var sx, sy, dx, dy, score, wall, food, portals, margin, dead, level, eaten, need, speed, acc;

    function blocked(x, y) {
      if (VAR === 5 && (x < margin || x >= W - margin || y < margin || y >= H - margin)) return true;
      return !!wall[y][x];
    }
    function onSnake(x, y) {
      for (var i = 0; i < sx.length; i++) if (sx[i] === x && sy[i] === y) return true;
      return false;
    }
    function placeFood(i) {
      var x, y, guard = 0;
      do { x = rnd(W); y = rnd(H); } while ((onSnake(x, y) || blocked(x, y)) && ++guard < 500);
      food[i] = { x: x, y: y, bad: (VAR === 6 && i > 0 && rnd(100) < 55) };
    }
    function buildWalls(lv) {
      var x, y, i;
      wall = [];
      for (y = 0; y < H; y++) wall.push(new Array(W).fill(0));
      portals = [];
      if (VAR === 2 || VAR === 7) {
        var blocks = (VAR === 7) ? 3 + lv * 2 : 8;
        for (i = 0; i < blocks; i++) {
          var bx = 3 + rnd(W - 10), by = 2 + rnd(H - 6), len = 3 + rnd(6), vert = rnd(2);
          for (x = 0; x < len; x++) {
            var wx = vert ? bx : bx + x, wy = vert ? by + x : by;
            if (wx > 0 && wx < W - 1 && wy > 0 && wy < H - 1) wall[wy][wx] = 1;
          }
        }
        for (x = W / 2 - 6; x <= W / 2 + 2; x++) if (x > 0 && x < W) wall[H >> 1][x] = 0;
      }
      if (VAR === 4) { portals = [{x:5,y:3},{x:W-6,y:H-4}]; }
    }
    function reset() {
      score = 0; margin = 0; level = 1; eaten = 0; need = 5; dead = false;
      speed = (VAR === 3) ? 4 : 7; acc = 0;
      buildWalls(level);
      sx = []; sy = [];
      for (var i = 0; i < 4; i++) { sx.push((W >> 1) - i); sy.push(H >> 1); }
      dx = 1; dy = 0;
      food = [];
      var n = (VAR === 6) ? 4 : 1;
      for (i = 0; i < n; i++) placeFood(i);
      if (VAR === 6) food[0].bad = false;
    }
    reset();
    return {
      tick: function () {
        if (dead) return;
        if (++acc < speed) return;
        acc = 0;
        var nx = sx[0] + dx, ny = sy[0] + dy, i;
        if (VAR === 1) { nx = (nx + W) % W; ny = (ny + H) % H; }
        else if (nx < 0 || nx >= W || ny < 0 || ny >= H) { dead = true; host.saveScore(score); return; }
        if (blocked(nx, ny)) { dead = true; host.saveScore(score); return; }
        for (i = 0; i < sx.length - 1; i++)
          if (sx[i] === nx && sy[i] === ny) { dead = true; host.saveScore(score); return; }
        for (i = 0; i < portals.length; i++)
          if (nx === portals[i].x && ny === portals[i].y) {
            nx = portals[1 - i].x; ny = portals[1 - i].y; break;
          }
        sx.unshift(nx); sy.unshift(ny);
        var ate = false;
        for (i = 0; i < food.length; i++) {
          if (nx !== food[i].x || ny !== food[i].y) continue;
          if (food[i].bad) { dead = true; host.saveScore(score); return; }
          score += 10; eaten++; ate = true;
          if (speed > 2) speed -= 0.08;
          if (VAR === 5 && eaten % 4 === 0 && margin < 6) margin++;
          placeFood(i);
          if (VAR === 7 && eaten >= need) {
            level++; need += 5; buildWalls(level);
            sx = [W >> 1]; sy = [H >> 1]; dx = 1; dy = 0;
            placeFood(0);
          }
          break;
        }
        if (!ate) { sx.pop(); sy.pop(); }
      },
      key: function (k) {
        if (dead) { reset(); return; }
        if (k === 'up'    && dy === 0) { dx = 0; dy = -1; }
        if (k === 'down'  && dy === 0) { dx = 0; dy = 1; }
        if (k === 'left'  && dx === 0) { dx = -1; dy = 0; }
        if (k === 'right' && dx === 0) { dx = 1; dy = 0; }
      },
      draw: function (t) {
        t.header('SNAKE', SUB);
        t.box(18, 3, W + 2, H + 2, C.blue);
        for (var y = 0; y < H; y++) for (var x = 0; x < W; x++) {
          if (wall[y][x]) t.put(19 + x, 4 + y, '#', C.blue);
          else if (VAR === 5 && blocked(x, y)) t.put(19 + x, 4 + y, '%', C.grey);
          else t.put(19 + x, 4 + y, ' ', C.grey);
        }
        for (var i = 0; i < portals.length; i++)
          t.put(19 + portals[i].x, 4 + portals[i].y, 'O', C.magenta, null, true);
        for (i = 0; i < sx.length; i++)
          t.put(19 + sx[i], 4 + sy[i], i ? 'o' : '@', C.green, null, i === 0);
        for (i = 0; i < food.length; i++)
          t.put(19 + food[i].x, 4 + food[i].y, food[i].bad ? 'x' : '*',
                food[i].bad ? C.red : C.yellow, null, true);
        t.text(18, H + 6, 'Score ' + score + '   Best ' + host.best() + '   ', C.fg);
        if (dead) t.center(H + 8, 'Game over — press any key.', C.red, true);
      }
    };
  }
});

/* ----------------------------------------------------------------- tetris */
reg('tetris', {
  title: 'Tetris', help: 'Arrows move/rotate · Space hard-drops · Q quits',
  start: function (host, p) {
    var VAR = (p.variant >= 0 && p.variant <= 8) ? p.variant : 0;
    var W = VAR === 4 ? 8 : 10, H = VAR === 4 ? 16 : 20, CW = VAR === 4 ? 3 : 2;
    var TETRO = [
      [[0,1],[1,1],[2,1],[3,1]], [[0,0],[0,1],[1,1],[2,1]], [[2,0],[0,1],[1,1],[2,1]],
      [[1,0],[2,0],[1,1],[2,1]], [[1,0],[2,0],[0,1],[1,1]], [[1,0],[0,1],[1,1],[2,1]],
      [[0,0],[1,0],[1,1],[2,1]]];
    var PENTO = [
      [[0,0],[0,1],[0,2],[0,3],[0,4]], [[0,0],[0,1],[0,2],[0,3],[1,3]],
      [[1,0],[1,1],[1,2],[0,3],[1,3]], [[0,0],[0,1],[1,1],[1,2],[1,3]],
      [[0,0],[1,0],[0,1],[1,1],[0,2]], [[0,0],[0,1],[1,1],[0,2],[1,2]],
      [[1,0],[0,1],[1,1],[0,2],[1,2]], [[0,0],[1,0],[1,1],[1,2],[2,2]],
      [[0,0],[1,0],[2,0],[1,1],[1,2]], [[0,0],[2,0],[0,1],[1,1],[2,1]],
      [[0,0],[0,1],[0,2],[1,2],[2,2]], [[1,0],[0,1],[1,1],[2,1],[1,2]]];
    var COL = [C.cyan,C.blue,C.yellow,C.yellow,C.green,C.magenta,C.red,
               C.cyan,C.green,C.blue,C.magenta,C.white];
    var SUB = ['Arrows move/rotate, Space hard-drops',
               'SPRINT: clear 40 lines as fast as you can',
               'ULTRA: score as much as possible in two minutes',
               'ZEN: no game over, no speed-up',
               'BIG MODE: wide board, chunky blocks',
               'MASTER: fast from the first piece',
               'INVISIBLE: locked blocks fade — F flashes them',
               'CASCADE: loose blocks fall again after a clear',
               'PENTIX: twelve pentominoes'][VAR];

    /* Rotations are derived, not tabulated — the pentominoes would otherwise
     * need 240 literals that could drift out of step with the C table. */
    function rotate(cells) {
      var maxy = 0, i, out = [];
      for (i = 0; i < cells.length; i++) if (cells[i][1] > maxy) maxy = cells[i][1];
      for (i = 0; i < cells.length; i++) out.push([maxy - cells[i][1], cells[i][0]]);
      var minx = 99, miny = 99;
      for (i = 0; i < out.length; i++) { if (out[i][0] < minx) minx = out[i][0]; if (out[i][1] < miny) miny = out[i][1]; }
      for (i = 0; i < out.length; i++) { out[i][0] -= minx; out[i][1] -= miny; }
      return out;
    }
    var base = VAR === 8 ? PENTO : TETRO, NP = base.length;
    var rots = base.map(function (b) {
      var r = [b], i;
      for (i = 1; i < 4; i++) r.push(rotate(r[i - 1]));
      return r;
    });

    var board, lockAt, piece, rot, px, py, score, lines, level, bag, nextp, dead, flash, started, acc, msg;

    function nextPiece() {
      if (!bag || !bag.length) {
        bag = [];
        for (var i = 0; i < NP; i++) bag.push(i);
        for (i = bag.length - 1; i > 0; i--) { var j = rnd(i + 1), t = bag[i]; bag[i] = bag[j]; bag[j] = t; }
      }
      return bag.pop();
    }
    function collides(pc, r, x, y) {
      var s = rots[pc][r], i;
      for (i = 0; i < s.length; i++) {
        var cx = x + s[i][0], cy = y + s[i][1];
        if (cx < 0 || cx >= W || cy >= H) return true;
        if (cy >= 0 && board[cy][cx]) return true;
      }
      return false;
    }
    function lockPiece() {
      var s = rots[piece][rot], i;
      for (i = 0; i < s.length; i++) {
        var cx = px + s[i][0], cy = py + s[i][1];
        if (cy >= 0 && cy < H && cx >= 0 && cx < W) { board[cy][cx] = piece + 1; lockAt[cy][cx] = Date.now(); }
      }
    }
    function cascade() {
      var moved = true, guard = 0;
      while (moved && guard++ < 40) {
        moved = false;
        for (var r = H - 2; r >= 0; r--) for (var c = 0; c < W; c++)
          if (board[r][c] && !board[r + 1][c]) {
            board[r + 1][c] = board[r][c]; lockAt[r + 1][c] = lockAt[r][c];
            board[r][c] = 0; moved = true;
          }
      }
    }
    function clearLines() {
      var PTS = [0,100,300,500,800,1200], cleared = 0, r, c;
      for (r = H - 1; r >= 0; r--) {
        var full = true;
        for (c = 0; c < W; c++) if (!board[r][c]) { full = false; break; }
        if (!full) continue;
        board.splice(r, 1); lockAt.splice(r, 1);
        board.unshift(new Array(W).fill(0));
        lockAt.unshift(new Array(W).fill(0));
        cleared++; r++;
      }
      if (cleared) {
        lines += cleared;
        score += PTS[Math.min(5, cleared)] * (level + 1);
        if (VAR !== 3) level = Math.floor(lines / 10);
        if (VAR === 7) cascade();
      }
    }
    function spawn() {
      piece = nextp; rot = 0; px = (W >> 1) - 2; py = -1;
      nextp = nextPiece();
      if (collides(piece, rot, px, py)) {
        if (VAR === 3) {                          /* zen just clears the stack */
          board = []; lockAt = [];
          for (var r = 0; r < H; r++) { board.push(new Array(W).fill(0)); lockAt.push(new Array(W).fill(0)); }
        } else { dead = true; host.saveScore(score); }
      }
    }
    function reset() {
      board = []; lockAt = [];
      for (var r = 0; r < H; r++) { board.push(new Array(W).fill(0)); lockAt.push(new Array(W).fill(0)); }
      score = 0; lines = 0; level = (VAR === 5) ? 9 : 0;
      bag = null; dead = false; flash = false; msg = null;
      started = Date.now(); acc = 0;
      nextp = nextPiece();
      spawn();
    }
    reset();
    return {
      tick: function () {
        if (dead) return;
        var speed = Math.max(2, 10 - level);
        if (VAR === 3) speed = 9;
        if (++acc < speed) return;
        acc = 0;
        if (!collides(piece, rot, px, py + 1)) py++;
        else { lockPiece(); clearLines(); spawn(); }
        if (VAR === 1 && lines >= 40) { dead = true; msg = '40 lines in ' + Math.round((Date.now() - started) / 1000) + 's!'; host.saveScore(score); }
        if (VAR === 2 && Date.now() - started > 120000) { dead = true; msg = 'Time!'; host.saveScore(score); }
      },
      key: function (k) {
        if (dead) { reset(); return; }
        if (k === 'f') flash = !flash;
        if (k === 'left'  && !collides(piece, rot, px - 1, py)) px--;
        if (k === 'right' && !collides(piece, rot, px + 1, py)) px++;
        if (k === 'down'  && !collides(piece, rot, px, py + 1)) py++;
        if (k === 'up') {
          var nr = (rot + 1) % 4;
          if (!collides(piece, nr, px, py)) rot = nr;
          else if (!collides(piece, nr, px - 1, py)) { rot = nr; px--; }
          else if (!collides(piece, nr, px + 1, py)) { rot = nr; px++; }
          else if (!collides(piece, nr, px + 2, py)) { rot = nr; px += 2; }
        }
        if (k === 'space') {
          while (!collides(piece, rot, px, py + 1)) { py++; score += 2; }
          lockPiece(); clearLines(); spawn();
        }
      },
      draw: function (t) {
        t.header('TETRIS', SUB);
        var bx = 40 - (W * CW) / 2, now = Date.now();
        t.box(bx - 1, 3, W * CW + 2, H + 2, C.blue);
        for (var r = 0; r < H; r++) for (var c = 0; c < W; c++) {
          var v = board[r][c];
          var hidden = (VAR === 6 && v && !flash && now - lockAt[r][c] > 1000);
          t.text(bx + c * CW, 4 + r, (v && !hidden) ? '##'.slice(0, CW) : ' .'.slice(0, CW),
                 (v && !hidden) ? COL[v - 1] : C.grey, null, !!v && !hidden);
        }
        var s = rots[piece][rot];
        for (var i = 0; i < s.length; i++) {
          var cy = py + s[i][1];
          if (cy >= 0 && cy < H) t.text(bx + (px + s[i][0]) * CW, 4 + cy, '##'.slice(0, CW), COL[piece], null, true);
        }
        var ix = bx + W * CW + 4;
        t.text(ix, 5, 'Score ' + score + '    ', C.fg);
        t.text(ix, 6, 'Lines ' + lines + '    ', C.fg);
        t.text(ix, 7, 'Level ' + level + '    ', C.fg);
        if (VAR === 1) t.text(ix, 8, 'Left  ' + Math.max(0, 40 - lines) + '    ', C.cyan);
        if (VAR === 2) t.text(ix, 8, 'Time  ' + Math.max(0, 120 - Math.round((Date.now() - started) / 1000)) + '   ', C.cyan);
        t.text(ix, 10, 'Next:', C.dim);
        for (r = 0; r < 5; r++) t.text(ix, 11 + r, '          ', C.grey);
        var n = rots[nextp][0];
        for (i = 0; i < n.length; i++) t.text(ix + n[i][0] * 2, 11 + n[i][1], '##', COL[nextp], null, true);
        if (dead) t.center(H + 6, (msg || 'Game over') + ' — press any key.', C.red, true);
      }
    };
  }
});

/* ------------------------------------------------------------------- pong */
reg('pong', {
  title: 'Pong', help: 'Up/Down move · first to 7 · Q quits',
  start: function (host, p) {
    var W = 60, H = 20;
    var VAR = (p.variant >= 0 && p.variant <= 5) ? p.variant : 0;
    var SUB = ['CLASSIC', 'CURVE: the ball carries spin', 'OBSTACLES in the court',
               'SHRINKING: your paddle shortens each rally',
               'FOUR-PLAYER: defend two walls', 'AIR HOCKEY: free-moving striker'][VAR];
    var bx, by, vx, vy, spin, pyv, ay, ps, as, paddle, blocks, tx, bxp, phx, phy, msg;

    function serve(dir) { bx = W / 2; by = H / 2; vx = dir * 0.9; vy = rnd(2) ? 0.4 : -0.4; spin = 0; }
    function reset() {
      paddle = 4; pyv = ay = (H >> 1) - 2; ps = as = 0; msg = null;
      phx = 4; phy = H / 2; tx = bxp = (W >> 1) - 3;
      blocks = [];
      if (VAR === 2) for (var i = 0; i < 5; i++) blocks.push({x:(W>>1)-8+rnd(16), y:2+rnd(H-4)});
      serve(1);
    }
    reset();
    return {
      tick: function () {
        if (msg) return;
        var i;
        if (VAR === 1) vy += spin * 0.06;
        bx += vx; by += vy;

        if (VAR === 4) {
          if (by <= 0) { if (bx >= tx && bx <= tx + 6) { by = 0; vy = -vy; } else { as++; serve(1); } }
          if (by >= H - 1) { if (bx >= bxp && bx <= bxp + 6) { by = H - 1; vy = -vy; } else { ps++; serve(-1); } }
        } else {
          if (by <= 0) { by = 0; vy = -vy; }
          if (by >= H - 1) { by = H - 1; vy = -vy; }
        }
        for (i = 0; i < blocks.length; i++)
          if ((bx | 0) === blocks[i].x && (by | 0) === blocks[i].y) { vx = -vx; bx += vx; }

        if (VAR === 5) {
          var ddx = bx - phx, ddy = by - phy;
          if (ddx * ddx + ddy * ddy < 2.2 && vx < 0) { vx = -vx; vy += ddy * 0.35; }
          vx *= 0.999; vy *= 0.999;
          if (bx <= 1) { as++; serve(1); }
        } else if (bx <= 2 && vx < 0) {
          if (by >= pyv - 0.5 && by <= pyv + paddle) {
            var off = (by - (pyv + paddle / 2)) / (paddle / 2);
            vx = -vx; vy += off * 0.45;
            if (VAR === 1) spin = off;
            vy = Math.max(-0.9, Math.min(0.9, vy));
            if (VAR === 3 && paddle > 2) paddle--;
          } else { as++; serve(1); if (VAR === 3) paddle = 4; }
        }
        if (bx >= W - 2 && vx > 0) {
          if (by >= ay - 0.5 && by <= ay + paddle) {
            vx = -vx; vy += ((by - (ay + paddle / 2)) / (paddle / 2)) * 0.45;
          } else { ps++; serve(-1); }
        }
        if (vx > 0) {
          var target = by - paddle / 2;
          if (ay < target - 0.5 && ay < H - paddle) ay++;
          else if (ay > target + 0.5 && ay > 0) ay--;
        }
        if (VAR === 4) {
          if (vy < 0) { if (tx < bx - 3) tx++; else if (tx > bx - 3) tx--; }
          else { if (bxp < bx - 3) bxp++; else if (bxp > bx - 3) bxp--; }
          tx = Math.max(0, Math.min(W - 7, tx));
          bxp = Math.max(0, Math.min(W - 7, bxp));
        }
        if (ps >= 7 || as >= 7) { msg = ps > as ? 'You win!' : 'Computer wins.'; host.saveScore(ps * 100 - as * 50); }
      },
      key: function (k) {
        if (msg) { reset(); return; }
        if (VAR === 5) {
          if (k === 'up' && phy > 1) phy--;
          if (k === 'down' && phy < H - 2) phy++;
          if (k === 'left' && phx > 1) phx--;
          if (k === 'right' && phx < W / 2 - 2) phx++;
        } else {
          if (k === 'up' && pyv > 0) pyv--;
          if (k === 'down' && pyv < H - paddle) pyv++;
        }
      },
      draw: function (t) {
        t.header('PONG', SUB + ' — first to 7');
        t.box(9, 3, W + 2, H + 2, C.blue);
        for (var i = 0; i < H; i++) t.put(10 + (W >> 1), 4 + i, '|', C.grey);
        for (i = 0; i < blocks.length; i++) t.put(10 + blocks[i].x, 4 + blocks[i].y, '#', C.magenta, null, true);
        if (VAR === 5) {
          t.put(10 + (phx | 0), 4 + (phy | 0), 'U', C.cyan, null, true);
          for (i = 0; i < paddle; i++) t.put(9 + W, 4 + ay + i, '#', C.red, null, true);
        } else {
          for (i = 0; i < paddle; i++) {
            t.put(11, 4 + pyv + i, '#', C.cyan, null, true);
            t.put(9 + W, 4 + ay + i, '#', C.red, null, true);
          }
        }
        if (VAR === 4) for (i = 0; i < 6; i++) {
          t.put(10 + tx + i, 4, '=', C.yellow, null, true);
          t.put(10 + bxp + i, 3 + H, '=', C.green, null, true);
        }
        t.put(10 + (bx | 0), 4 + (by | 0), 'O', C.yellow, null, true);
        t.text((W >> 1) + 4, 2, ps + '   ' + as + '  ', C.white, null, true);
        if (msg) t.center(H + 6, msg + ' Press any key.', C.white, true);
      }
    };
  }
});

/* --------------------------------------------------------------- breakout */
reg('breakout', {
  title: 'Breakout', help: 'Left/Right move · Q quits',
  start: function (host, p) {
    var W = 48, H = 22, BR = 5, BC = 12;
    var VAR = (p.variant >= 0 && p.variant <= 6) ? p.variant : 0;
    var SUB = ['Left/Right move', 'ARKANOID: catch the falling capsules',
               'MULTIBALL: three at once', 'GRAVITY: the ball is pulled down',
               'BOSS: one heavy target that moves', 'ENDLESS: the wall rebuilds, tougher',
               'ULTRA: fast ball, hard bricks'][VAR];
    var RC = [C.red, C.magenta, C.yellow, C.green, C.cyan];
    var brick, balls, paddle, PAD, score, lives, level, bossX, bossHP, drop, dead;

    function launch(x) { return { x: x, y: H - 4, vx: rnd(2) ? 0.7 : -0.7, vy: -0.7, live: true }; }
    function resetLevel() {
      var r, c;
      brick = [];
      for (r = 0; r < BR; r++) {
        brick.push([]);
        for (c = 0; c < BC; c++)
          brick[r].push((VAR === 5 || VAR === 6) ? 1 + rnd(3) : ((BR - r) > 2 ? 2 : 1));
      }
      if (VAR === 4) {
        for (r = 0; r < BR; r++) for (c = 0; c < BC; c++) brick[r][c] = 0;
        bossX = BC >> 1; bossHP = 20 + level * 6;
      } else bossHP = 0;
      PAD = 7;
      balls = [];
      var n = (VAR === 2) ? 3 : 1;
      for (var i = 0; i < n; i++) balls.push(launch(W / 2 + i * 2));
      paddle = (W >> 1) - 3;
      drop = null;
    }
    function bricksLeft() {
      var n = 0;
      for (var r = 0; r < BR; r++) for (var c = 0; c < BC; c++) if (brick[r][c]) n++;
      return n + (bossHP > 0 ? bossHP : 0);
    }
    function reset() { score = 0; lives = 3; level = 1; dead = false; resetLevel(); }
    reset();
    return {
      tick: function () {
        if (dead) return;
        var i, alive = 0;
        if (VAR === 4 && bossHP > 0) {
          bossX += rnd(100) < 50 ? 1 : -1;
          bossX = Math.max(0, Math.min(BC - 3, bossX));
        }
        for (i = 0; i < balls.length; i++) {
          var b = balls[i];
          if (!b.live) continue;
          if (VAR === 3) b.vy += 0.035;
          b.x += b.vx; b.y += b.vy;
          if (b.x <= 0) { b.x = 0; b.vx = -b.vx; }
          if (b.x >= W - 1) { b.x = W - 1; b.vx = -b.vx; }
          if (b.y <= 0) { b.y = 0; b.vy = -b.vy; }
          var r = (b.y | 0) - 2, c = Math.floor(b.x / (W / BC));
          if (VAR === 4 && r === 0 && bossHP > 0 && c >= bossX && c <= bossX + 2) {
            bossHP--; score += 25; b.vy = -b.vy;
          } else if (r >= 0 && r < BR && c >= 0 && c < BC && brick[r][c]) {
            brick[r][c]--; score += 10; b.vy = -b.vy;
            if (VAR === 1 && !drop && rnd(100) < 18) drop = { x: b.x, y: b.y, kind: rnd(2) };
          }
          if ((b.y | 0) >= H - 2 && b.vy > 0 && b.x >= paddle - 1 && b.x <= paddle + PAD) {
            var hit = (b.x - (paddle + PAD / 2)) / (PAD / 2);
            b.vy = -Math.abs(b.vy);
            b.vx += hit * 0.5;
            b.vx = Math.max(-1.1, Math.min(1.1, b.vx));
            b.y = H - 2;
          }
          if (b.y >= H - 1) b.live = false;
          if (b.live) alive++;
        }
        if (drop) {
          drop.y += 0.35;
          if (drop.y >= H - 2 && drop.x >= paddle && drop.x <= paddle + PAD) {
            if (drop.kind && PAD < 13) PAD += 2; else if (!drop.kind && PAD > 3) PAD -= 2;
            drop = null;
          } else if (drop.y >= H - 1) drop = null;
        }
        if (alive === 0) {
          lives--;
          if (lives > 0) {
            balls = [];
            var n = (VAR === 2) ? 3 : 1;
            for (i = 0; i < n; i++) balls.push(launch(W / 2 + i * 2));
          } else { dead = true; host.saveScore(score); }
        }
        if (bricksLeft() === 0) { level++; score += 200; resetLevel(); }
      },
      key: function (k) {
        if (dead) { reset(); return; }
        if (k === 'left') paddle = Math.max(0, paddle - 2);
        if (k === 'right') paddle = Math.min(W - PAD, paddle + 2);
      },
      draw: function (t) {
        t.header('BREAKOUT', SUB);
        t.box(13, 3, W + 2, H + 2, C.blue);
        for (var r = 0; r < BR; r++) for (var c = 0; c < BC; c++) {
          var v = brick[r][c];
          t.text(14 + c * 4, 6 + r, v >= 2 ? '###' : v === 1 ? ':::' : '   ', RC[r]);
        }
        if (VAR === 4 && bossHP > 0) t.text(14 + bossX * 4, 6, '[BOSS ' + bossHP + ']', C.red, null, true);
        for (var i = 0; i < PAD; i++) t.put(14 + paddle + i, 3 + H, '#', C.white, null, true);
        for (i = 0; i < balls.length; i++)
          if (balls[i].live) t.put(14 + (balls[i].x | 0), 4 + (balls[i].y | 0), 'O', C.yellow, null, true);
        if (drop) t.put(14 + (drop.x | 0), 4 + (drop.y | 0), drop.kind ? 'W' : 'S',
                        drop.kind ? C.green : C.red, null, true);
        t.text(13, H + 6, 'Score ' + score + '  Lives ' + lives + '  Level ' + level + '  Paddle ' + PAD + '   ', C.fg);
        if (dead) t.center(H + 8, 'Game over — press any key.', C.red, true);
      }
    };
  }
});

/* --------------------------------------------------------- space invaders */
reg('invaders', {
  title: 'Space Invaders', help: 'Left/Right move · Space fires · Q quits',
  realtime: true, step: 55,
  start: function (host, p) {
    var W = 48, H = 22, AR = 4, AC = 10;
    var ACOL = [C.magenta, C.cyan, C.green, C.yellow];
    /* Galaga pulls attackers out of the grid to dive at the ship; that is the
     * difference between the two entries, not a faster formation. */
    var DIVING = p.variant === 1;
    var alive, ax, ay, adir, ship, score, lives, wave, shots, bombs, phase, over, diver;
    function count() {
      var n = 0;
      alive.forEach(function (r) { r.forEach(function (v) { n += v; }); });
      return n;
    }
    function waveReset() {
      alive = [];
      for (var r = 0; r < AR; r++) alive.push(new Array(AC).fill(1));
      ax = 2; ay = 1; adir = 1; shots = []; bombs = []; diver = null;
    }
    function reset() {
      score = 0; lives = 3; wave = 1; ship = W / 2 | 0; phase = 0; over = false;
      waveReset();
    }
    reset();
    return {
      key: function (k) {
        if (over) { reset(); return; }
        if (k === 'left'  && ship > 0) ship--;
        if (k === 'right' && ship < W - 1) ship++;
        if ((k === 'space' || k === 'up') && shots.length < 4) shots.push({ x: ship, y: H - 2 });
      },
      tick: function () {
        if (over) return;
        shots = shots.filter(function (s) { s.y--; return s.y >= 0; });
        bombs = bombs.filter(function (b) {
          b.y++;
          if (b.y === H - 1 && b.x === ship) { lives--; return false; }
          return b.y < H;
        });
        if (++phase % 4 === 0) {
          var lo = W, hi = 0;
          for (var r = 0; r < AR; r++) for (var c = 0; c < AC; c++) {
            if (!alive[r][c]) continue;
            lo = Math.min(lo, ax + c * 4);
            hi = Math.max(hi, ax + c * 4);
          }
          if ((adir > 0 && hi >= W - 2) || (adir < 0 && lo <= 1)) { adir = -adir; ay++; }
          else ax += adir;
          if (ay + AR >= H - 1) { over = true; host.saveScore(score); return; }
        }
        if (rnd(100) < 8 && bombs.length < 6) {
          var c2 = rnd(AC);
          for (var r2 = AR - 1; r2 >= 0; r2--)
            if (alive[r2][c2]) { bombs.push({ x: ax + c2 * 4, y: ay + r2 + 1 }); break; }
        }
        shots = shots.filter(function (s) {
          for (var r = 0; r < AR; r++) for (var c = 0; c < AC; c++) {
            if (!alive[r][c]) continue;
            if (s.x === ax + c * 4 && s.y === ay + r) {
              alive[r][c] = 0;
              score += (AR - r) * 10;
              return false;
            }
          }
          return true;
        });
        if (DIVING) {
          if (!diver && rnd(100) < 4) {
            for (var tr = 0; tr < 20; tr++) {
              var dr = rnd(AR), dc = rnd(AC);
              if (!alive[dr][dc]) continue;
              diver = { x: ax + dc * 4, y: ay + dr, vx: 0 };
              alive[dr][dc] = 0;
              break;
            }
          }
          if (diver) {
            diver.vx += (ship > diver.x) ? 0.12 : -0.12;
            diver.vx = Math.max(-0.8, Math.min(0.8, diver.vx));
            diver.x += diver.vx;
            diver.y += 0.45;
            diver.x = Math.max(0, Math.min(W - 1, diver.x));
            if ((diver.y | 0) >= H - 1) {
              if (Math.abs((diver.x | 0) - ship) <= 1) lives--;
              diver = null;
            } else {
              shots = shots.filter(function (s2) {
                if (diver && s2.x === (diver.x | 0) && s2.y === (diver.y | 0)) {
                  diver = null; score += 150;      /* divers are worth more */
                  return false;
                }
                return true;
              });
            }
          }
        }
        if (count() === 0 && !diver) { wave++; score += 500; waveReset(); }
        if (lives <= 0) { over = true; host.saveScore(score); }
      },
      draw: function (t) {
        t.header(DIVING ? 'GALAGA' : 'SPACE INVADERS',
                 DIVING ? 'Attackers peel off and dive — Left/Right move, Space fires'
                        : 'Left/Right move · Space fires · Q quits');
        t.box(14, 3, W + 2, H + 2, C.blue);
        for (var r = 0; r < AR; r++) for (var c = 0; c < AC; c++)
          if (alive[r][c]) t.put(15 + ax + c * 4, 4 + ay + r, r === 0 ? 'Ѫ' : 'ᙢ', ACOL[r], null, true);
        shots.forEach(function (s) { t.put(15 + s.x, 4 + s.y, '|', C.white); });
        bombs.forEach(function (b) { t.put(15 + b.x, 4 + b.y, '!', C.red); });
        if (diver) t.put(15 + (diver.x | 0), 4 + (diver.y | 0), 'W', C.red, null, true);
        t.put(15 + ship, 4 + H - 1, '▲', C.cyan, null, true);
        t.text(14, H + 6, 'Score ' + score + '  Lives ' + lives + '  Wave ' + wave + '   ', C.fg);
        if (over) t.center(H + 8, 'Game over — press any key.', C.red, true);
      }
    };
  }
});

/* ---------------------------------------------------------------- dino run */
reg('dino', {
  title: 'Dino Run', help: 'Space or Up jumps · Q quits', realtime: true, step: 55,
  start: function (host, p) {
    var W = 60, y, vy, score, dead, gap, obstacle;
    function reset() {
      y = 0; vy = 0; score = 0; dead = false; gap = 0;
      obstacle = new Array(W).fill(0);
      self.step = 55;
    }
    var self = {
      key: function (k) {
        if (dead) { reset(); return; }
        if ((k === 'space' || k === 'up') && y === 0) vy = 2.4;
      },
      tick: function () {
        if (dead) return;
        y += vy; vy -= 0.42;
        if (y <= 0) { y = 0; vy = 0; }
        obstacle.shift(); obstacle.push(0);
        if (--gap <= 0) { obstacle[W - 1] = 1; gap = 12 + rnd(14); }
        if (obstacle[6] && y < 1.6) { dead = true; host.saveScore(score); return; }
        score++;
        if (self.step > 22) self.step -= 0.06;
      },
      draw: function (t) {
        var GROUND = 14;
        t.header('DINO RUN', 'Space or Up jumps · Q quits');
        for (var i = 0; i < W; i++) if (obstacle[i]) {
          t.put(12 + i, GROUND - 1, '♣', C.green);
          t.put(12 + i, GROUND, '║', C.green);
        }
        t.put(18, GROUND - Math.round(y), '&', C.yellow, null, true);
        t.hline(12, GROUND + 1, W, '#2e333a');
        t.text(12, GROUND + 3, 'Score ' + score + '   Best ' + host.best() + '   ', C.fg);
        if (dead) t.center(GROUND + 5, 'Crashed! Press any key.', C.red, true);
      }
    };
    reset();
    return self;
  }
});

/* ----------------------------------------------------------------- flappy */
reg('flappy', {
  title: 'Flappy', help: 'Space or Up flaps · Q quits', realtime: true, step: 70,
  start: function (host, p) {
    var W = 50, H = 20, PIPES = 4, y, vy, score, dead, pipes;
    function reset() {
      y = H / 2; vy = 0; score = 0; dead = false;
      pipes = [];
      for (var i = 0; i < PIPES; i++)
        pipes.push({ x: W + i * (W / PIPES), gap: 3 + rnd(H - 9) });
    }
    reset();
    return {
      key: function (k) {
        if (dead) { reset(); return; }
        if (k === 'space' || k === 'up') vy = -1.15;
      },
      tick: function () {
        if (dead) return;
        vy += 0.32; y += vy;
        if (y < 0 || y >= H) { dead = true; host.saveScore(score); return; }
        pipes.forEach(function (p) {
          p.x--;
          if (p.x < -2) { p.x = W; p.gap = 3 + rnd(H - 9); score++; }
          if (p.x === 8 && ((y | 0) < p.gap || (y | 0) > p.gap + 4)) {
            dead = true;
            host.saveScore(score);
          }
        });
      },
      draw: function (t) {
        t.header('FLAPPY', 'Space or Up flaps · Q quits');
        t.box(14, 3, W + 2, H + 2, C.blue);
        pipes.forEach(function (p) {
          if (p.x < 0 || p.x >= W) return;
          for (var r = 0; r < H; r++)
            if (r < p.gap || r > p.gap + 4) t.put(15 + p.x, 4 + r, '█', C.green);
        });
        if (y >= 0 && y < H) t.put(23, 4 + (y | 0), '●', C.yellow, null, true);
        t.text(14, H + 6, 'Score ' + score + '   Best ' + host.best() + '   ', C.fg);
        if (dead) t.center(H + 8, 'Crashed! Press any key.', C.red, true);
      }
    };
  }
});

/* ---------------------------------------------------------------- frogger */
reg('frogger', {
  title: 'Frogger', help: 'Arrows hop · reach the top · Q quits', realtime: true, step: 140,
  start: function (host, p) {
    var W = 44, LANES = 11, fx, fy, lives, score, home, offset, speed, over;
    function reset() {
      fx = W / 2 | 0; fy = LANES - 1; lives = 3; score = 0; home = 0; over = false;
      offset = []; speed = [];
      for (var i = 0; i < LANES; i++) {
        offset.push(rnd(W));
        speed.push((i >= 1 && i <= 4) ? (i % 2 ? 1 : -1) : (i >= 6 && i <= 9) ? (i % 2 ? -1 : 1) : 0);
      }
    }
    reset();
    return {
      key: function (k) {
        if (over) { reset(); return; }
        if (k === 'up'    && fy > 0) { fy--; score += 10; }
        if (k === 'down'  && fy < LANES - 1) fy++;
        if (k === 'left'  && fx > 0) fx--;
        if (k === 'right' && fx < W - 1) fx++;
      },
      tick: function () {
        if (over) return;
        for (var i = 0; i < LANES; i++)
          if (speed[i]) offset[i] = (offset[i] + speed[i] + W) % W;
        if (fy >= 1 && fy <= 4) {
          if (((fx + offset[fy]) % 8) < 4) fx = (fx + speed[fy] + W) % W;
          else { lives--; fx = W / 2 | 0; fy = LANES - 1; }
        } else if (fy >= 6 && fy <= 9) {
          if (((fx + offset[fy]) % 9) < 3) { lives--; fx = W / 2 | 0; fy = LANES - 1; }
        } else if (fy === 0) {
          home++; score += 100;
          fx = W / 2 | 0; fy = LANES - 1;
          if (home >= 5) { score += 500; home = 0; }
        }
        if (lives <= 0) { over = true; host.saveScore(score); }
      },
      draw: function (t) {
        t.header('FROGGER', 'Arrows hop · reach the top · Q quits');
        t.box(16, 4, W + 2, LANES + 2, C.blue);
        for (var i = 0; i < LANES; i++) for (var x = 0; x < W; x++) {
          var ch = ' ', fg = C.grey;
          if (i === 0) { ch = '▒'; fg = C.green; }
          else if (i >= 1 && i <= 4) {
            var log = ((x + offset[i]) % 8) < 4;
            ch = log ? '▬' : '≈'; fg = log ? C.yellow : C.blue;
          } else if (i === 5 || i === LANES - 1) { ch = '░'; fg = '#2e333a'; }
          else { var car = ((x + offset[i]) % 9) < 3; ch = car ? '▄' : ' '; fg = C.red; }
          t.put(17 + x, 5 + i, ch, fg);
        }
        t.put(17 + fx, 5 + fy, '@', C.green, null, true);
        t.text(16, LANES + 8, 'Score ' + score + '  Lives ' + lives + '  Home ' + home + '/5   ', C.fg);
        if (over) t.center(LANES + 10, 'Game over — press any key.', C.red, true);
      }
    };
  }
});

/* ----------------------------------------------------------------- pacman */
reg('pacman', {
  title: 'Pacman', help: 'Arrows steer · Q quits', realtime: true, step: 130,
  start: function (host, p) {
    var MAZE = [
      "############################",
      "#............##............#",
      "#.####.#####.##.#####.####.#",
      "#.####.#####.##.#####.####.#",
      "#..........................#",
      "#.####.##.########.##.####.#",
      "#......##....##....##......#",
      "######.##### ## #####.######",
      "     #.##          ##.#     ",
      "######.## ######## ##.######",
      "#............##............#",
      "#.####.#####.##.#####.####.#",
      "#...##................##...#",
      "###.##.##.########.##.##.###",
      "#......##....##....##......#",
      "#.##########.##.##########.#",
      "############################"
    ];
    var H = MAZE.length, W = 28, GH = 4;
    var GC = [C.red, C.magenta, C.cyan, C.green];
    var grid, pr, pc, dr, dc, ghost, score, lives, pellets, tick_, over;
    function wall(r, c) { return r < 0 || r >= H || c < 0 || c >= W || grid[r][c] === '#'; }
    function positions() {
      pr = 12; pc = 13; dr = 0; dc = 0;
      ghost = [];
      for (var i = 0; i < GH; i++)
        ghost.push({ r: 8, c: 11 + i, dr: 0, dc: i % 2 ? 1 : -1 });
    }
    function load() {
      grid = MAZE.map(function (r) { return r.split(''); });
      pellets = 0;
      grid.forEach(function (r) { r.forEach(function (ch) { if (ch === '.') pellets++; }); });
    }
    function reset() {
      score = 0; lives = 3; tick_ = 0; over = null;
      load(); positions();
    }
    function moveGhost(i) {
      var DR = [-1,1,0,0], DC = [0,0,-1,1];
      var g = ghost[i], tr = pr, tc = pc, best = -1, bestd = 1e9, d;
      if (i === 1) { tr = pr + dr * 4; tc = pc + dc * 4; }
      if (i === 2) { tr = H - pr; tc = W - pc; }
      if (i === 3 && rnd(4) === 0) {
        for (d = 0; d < 8; d++) {
          var tt = rnd(4);
          if (!wall(g.r + DR[tt], g.c + DC[tt])) { best = tt; break; }
        }
      }
      if (best < 0) for (d = 0; d < 4; d++) {
        var nr = g.r + DR[d], nc = g.c + DC[d];
        if (wall(nr, nc)) continue;
        if (DR[d] === -g.dr && DC[d] === -g.dc) continue;
        var dist = (nr - tr) * (nr - tr) + (nc - tc) * (nc - tc);
        if (dist < bestd) { bestd = dist; best = d; }
      }
      if (best < 0) { g.dr = -g.dr; g.dc = -g.dc; return; }
      g.dr = DR[best]; g.dc = DC[best];
      g.r += g.dr; g.c += g.dc;
    }
    reset();
    return {
      key: function (k) {
        if (over) { reset(); return; }
        if (k === 'up')    { dr = -1; dc = 0; }
        if (k === 'down')  { dr = 1;  dc = 0; }
        if (k === 'left')  { dr = 0;  dc = -1; }
        if (k === 'right') { dr = 0;  dc = 1; }
      },
      tick: function () {
        if (over) return;
        tick_++;
        if (!wall(pr + dr, pc + dc)) { pr += dr; pc += dc; }
        if (grid[pr][pc] === '.') { grid[pr][pc] = ' '; score += 10; pellets--; }
        if (tick_ % 2 === 0) for (var i = 0; i < GH; i++) moveGhost(i);
        for (var j = 0; j < GH; j++)
          if (ghost[j].r === pr && ghost[j].c === pc) { lives--; positions(); break; }
        if (!pellets) { over = 'Maze cleared!'; host.saveScore(score); }
        if (lives <= 0) { over = 'Game over.'; host.saveScore(score); }
      },
      draw: function (t) {
        t.header('PACMAN', 'Arrows steer · Q quits');
        for (var r = 0; r < H; r++) for (var c = 0; c < W; c++) {
          var ch = grid[r][c];
          if (ch === '#') t.put(26 + c, 4 + r, '█', C.blue);
          else if (ch === '.') t.put(26 + c, 4 + r, '·', C.yellow);
        }
        ghost.forEach(function (g, i) { t.put(26 + g.c, 4 + g.r, 'ᙢ', GC[i], null, true); });
        t.put(26 + pc, 4 + pr, '@', C.yellow, null, true);
        t.text(26, H + 5, 'Score ' + score + '  Lives ' + lives + '  Pellets ' + pellets + '   ', C.fg);
        if (over) t.center(H + 7, over + ' Press any key.', C.white, true);
      }
    };
  }
});

/* -------------------------------------------------------------- asteroids */
reg('asteroids', {
  title: 'Asteroids', help: 'Left/Right turn · Up thrusts · Space fires · Q quits',
  realtime: true, step: 55,
  start: function (host, p) {
    var W = 60, H = 22, rocks, shots, sx, sy, svx, svy, angle, score, lives, wave, over;
    /* Deluxe splits rocks one stage further and sends a saucer hunting. */
    var DELUXE = p.variant === 1;
    var saucer = null;
    function wrap(o) {
      if (o.x < 0) o.x += W;
      if (o.x >= W) o.x -= W;
      if (o.y < 0) o.y += H;
      if (o.y >= H) o.y -= H;
    }
    function spawnRocks(n) {
      rocks = [];
      for (var i = 0; i < n; i++)
        rocks.push({ x: rnd(W), y: rnd(H),
                     vx: (Math.random() - 0.5) * (DELUXE ? 0.9 : 0.6),
                     vy: (Math.random() - 0.5) * (DELUXE ? 0.6 : 0.4),
                     size: DELUXE ? 4 : 3 });
      saucer = null;
    }
    function reset() {
      sx = W / 2; sy = H / 2; svx = svy = 0; angle = 0;
      score = 0; lives = 3; wave = 4; shots = []; over = false;
      spawnRocks(wave);
    }
    reset();
    return {
      key: function (k) {
        if (over) { reset(); return; }
        if (k === 'left')  angle -= Math.PI / 8;
        if (k === 'right') angle += Math.PI / 8;
        if (k === 'up') {
          svx += Math.cos(angle) * 0.22;
          svy += Math.sin(angle) * 0.16;
          svx = Math.max(-1.2, Math.min(1.2, svx));
          svy = Math.max(-0.9, Math.min(0.9, svy));
        }
        if (k === 'space' && shots.length < 6)
          shots.push({ x: sx, y: sy, vx: svx + Math.cos(angle) * 1.5,
                       vy: svy + Math.sin(angle) * 1.1, life: 18 });
      },
      tick: function () {
        if (over) return;
        sx += svx; sy += svy; svx *= 0.985; svy *= 0.985;
        var ship = { x: sx, y: sy };
        wrap(ship); sx = ship.x; sy = ship.y;

        shots = shots.filter(function (s) {
          s.x += s.vx; s.y += s.vy; wrap(s);
          return --s.life > 0;
        });
        var next = [];
        rocks.forEach(function (r) {
          r.x += r.vx; r.y += r.vy; wrap(r);
          var hitBy = -1;
          shots.forEach(function (s, i) {
            if ((s.x | 0) === (r.x | 0) && (s.y | 0) === (r.y | 0)) hitBy = i;
          });
          if (hitBy >= 0) {
            shots.splice(hitBy, 1);
            score += r.size * 20;
            if (r.size > 1) for (var j = 0; j < (DELUXE ? 3 : 2); j++)
              next.push({ x: r.x, y: r.y, vx: (Math.random() - 0.5) * 1.2,
                          vy: (Math.random() - 0.5) * 0.8, size: r.size - 1 });
            return;
          }
          if ((r.x | 0) === (sx | 0) && (r.y | 0) === (sy | 0)) {
            lives--;
            sx = W / 2; sy = H / 2; svx = svy = 0;
          }
          next.push(r);
        });
        rocks = next;
        if (DELUXE) {
          if (!saucer && rnd(1000) < 6) saucer = { x: 0, y: rnd(H), vx: 0.5 };
          if (saucer) {
            saucer.x += saucer.vx;
            if (saucer.y < sy) saucer.y += 0.12; else if (saucer.y > sy) saucer.y -= 0.12;
            if (saucer.x > W) saucer = null;
          }
          if (saucer) {
            shots = shots.filter(function (s2) {
              if (saucer && Math.abs(s2.x - saucer.x) < 2 && Math.abs(s2.y - saucer.y) < 2) {
                saucer = null; score += 500;
                return false;
              }
              return true;
            });
          }
          if (saucer && Math.abs(sx - saucer.x) < 2 && Math.abs(sy - saucer.y) < 2) {
            lives--; saucer = null; sx = W / 2; sy = H / 2; svx = svy = 0;
          }
        }
        if (!rocks.length) { wave++; score += 300; spawnRocks(Math.min(16, wave)); }
        if (lives <= 0) { over = true; host.saveScore(score); }
      },
      draw: function (t) {
        t.header(DELUXE ? 'SPACE ROCKS DELUXE' : 'ASTEROIDS',
                 (DELUXE ? 'Rocks split further and a saucer hunts you — ' : '') +
                 'Left/Right turn · Up thrusts · Space fires');
        t.box(10, 3, W + 2, H + 2, C.blue);
        rocks.forEach(function (r) {
          t.put(11 + (r.x | 0), 4 + (r.y | 0),
                r.size >= 4 ? '@' : r.size === 3 ? 'O' : r.size === 2 ? 'o' : '.',
                C.grey, null, true);
        });
        shots.forEach(function (s) { t.put(11 + (s.x | 0), 4 + (s.y | 0), '·', C.yellow); });
        if (saucer) t.text(11 + (saucer.x | 0), 4 + (saucer.y | 0), '<o>', C.red, null, true);
        t.put(11 + (sx | 0), 4 + (sy | 0), '▲', C.cyan, null, true);
        t.text(10, H + 6, 'Score ' + score + '  Lives ' + lives + '  Wave ' + wave + '   ', C.fg);
        if (over) t.center(H + 8, 'Game over — press any key.', C.red, true);
      }
    };
  }
});

}());
