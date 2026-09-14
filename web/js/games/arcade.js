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
  title: 'Snake', help: 'Arrows steer · Q quits', realtime: true, step: 120,
  start: function (host, p) {
    var W = 40, H = 20, sx, sy, dx, dy, fx, fy, score, dead;
    function onSnake(x, y) {
      for (var i = 0; i < sx.length; i++) if (sx[i] === x && sy[i] === y) return true;
      return false;
    }
    function food() { do { fx = rnd(W); fy = rnd(H); } while (onSnake(fx, fy)); }
    function reset() {
      sx = [20,19,18,17]; sy = [10,10,10,10];
      dx = 1; dy = 0; score = 0; dead = false;
      self.step = 120;
      food();
    }
    var self = {
      over: false,
      key: function (k) {
        if (dead) { reset(); self.over = false; return; }
        if (k === 'up'    && dy === 0) { dx = 0; dy = -1; }
        if (k === 'down'  && dy === 0) { dx = 0; dy = 1; }
        if (k === 'left'  && dx === 0) { dx = -1; dy = 0; }
        if (k === 'right' && dx === 0) { dx = 1; dy = 0; }
      },
      tick: function () {
        if (dead) return;
        var nx = sx[0] + dx, ny = sy[0] + dy, i;
        if (nx < 0 || nx >= W || ny < 0 || ny >= H) { dead = true; host.saveScore(score); return; }
        for (i = 0; i < sx.length - 1; i++)
          if (sx[i] === nx && sy[i] === ny) { dead = true; host.saveScore(score); return; }
        sx.unshift(nx); sy.unshift(ny);
        if (nx === fx && ny === fy) {
          score += 10;
          if (self.step > 45) self.step -= 3;
          food();
        } else { sx.pop(); sy.pop(); }
      },
      draw: function (t) {
        t.header('SNAKE', 'Arrows steer · Q quits');
        t.box(18, 4, W + 2, H + 2, C.blue);
        for (var i = 0; i < sx.length; i++)
          t.put(19 + sx[i], 5 + sy[i], i ? 'o' : '@', C.green, null, i === 0);
        t.put(19 + fx, 5 + fy, '✱', C.red, null, true);
        t.text(18, H + 7, 'Score: ' + score + '   Best: ' + host.best() + '   ', C.fg);
        if (dead) t.center(H + 9, 'Game over — press any key.', C.red, true);
      }
    };
    reset();
    return self;
  }
});

/* ----------------------------------------------------------------- tetris */
reg('tetris', {
  title: 'Tetris', help: 'Arrows move/rotate · Space hard-drops · Q quits',
  realtime: true, step: 500,
  start: function (host, p) {
    var W = 10, H = 20;
    var SHAPES = [
      [[[0,1],[1,1],[2,1],[3,1]],[[2,0],[2,1],[2,2],[2,3]],[[0,2],[1,2],[2,2],[3,2]],[[1,0],[1,1],[1,2],[1,3]]],
      [[[0,0],[0,1],[1,1],[2,1]],[[1,0],[2,0],[1,1],[1,2]],[[0,1],[1,1],[2,1],[2,2]],[[1,0],[1,1],[0,2],[1,2]]],
      [[[2,0],[0,1],[1,1],[2,1]],[[1,0],[1,1],[1,2],[2,2]],[[0,1],[1,1],[2,1],[0,2]],[[0,0],[1,0],[1,1],[1,2]]],
      [[[1,0],[2,0],[1,1],[2,1]],[[1,0],[2,0],[1,1],[2,1]],[[1,0],[2,0],[1,1],[2,1]],[[1,0],[2,0],[1,1],[2,1]]],
      [[[1,0],[2,0],[0,1],[1,1]],[[1,0],[1,1],[2,1],[2,2]],[[1,1],[2,1],[0,2],[1,2]],[[0,0],[0,1],[1,1],[1,2]]],
      [[[1,0],[0,1],[1,1],[2,1]],[[1,0],[1,1],[2,1],[1,2]],[[0,1],[1,1],[2,1],[1,2]],[[1,0],[0,1],[1,1],[1,2]]],
      [[[0,0],[1,0],[1,1],[2,1]],[[2,0],[1,1],[2,1],[1,2]],[[0,1],[1,1],[1,2],[2,2]],[[1,0],[0,1],[1,1],[0,2]]]
    ];
    var PCOL = [C.cyan, C.blue, C.yellow, C.yellow, C.green, C.magenta, C.red];
    var PTS = [0, 100, 300, 500, 800];
    var board, piece, rot, px, py, score, lines, level, bag, nextp, dead;
    function refill() { bag = G.shuffle([0,1,2,3,4,5,6]); }
    function take() { if (!bag.length) refill(); return bag.pop(); }
    function collides(p, r, x, y) {
      for (var i = 0; i < 4; i++) {
        var cx = x + SHAPES[p][r][i][0], cy = y + SHAPES[p][r][i][1];
        if (cx < 0 || cx >= W || cy >= H) return true;
        if (cy >= 0 && board[cy][cx]) return true;
      }
      return false;
    }
    function lock() {
      for (var i = 0; i < 4; i++) {
        var cx = px + SHAPES[piece][rot][i][0], cy = py + SHAPES[piece][rot][i][1];
        if (cy >= 0 && cy < H && cx >= 0 && cx < W) board[cy][cx] = piece + 1;
      }
    }
    function clearLines() {
      var cleared = 0;
      for (var r = H - 1; r >= 0; r--) {
        if (board[r].some(function (v) { return !v; })) continue;
        board.splice(r, 1);
        board.unshift(new Array(W).fill(0));
        cleared++; r++;
      }
      if (cleared) {
        lines += cleared;
        score += PTS[Math.min(4, cleared)] * (level + 1);
        level = lines / 10 | 0;
        self.step = Math.max(80, 500 - level * 40);
      }
    }
    function spawn() {
      piece = nextp; nextp = take();
      rot = 0; px = (W / 2 | 0) - 2; py = -1;
      if (collides(piece, rot, px, py)) { dead = true; host.saveScore(score); }
    }
    function reset() {
      board = []; for (var r = 0; r < H; r++) board.push(new Array(W).fill(0));
      score = 0; lines = 0; level = 0; dead = false;
      refill(); nextp = take(); spawn();
      self.step = 500;
    }
    var self = {
      key: function (k) {
        if (dead) { reset(); return; }
        if (k === 'left'  && !collides(piece, rot, px - 1, py)) px--;
        if (k === 'right' && !collides(piece, rot, px + 1, py)) px++;
        if (k === 'down'  && !collides(piece, rot, px, py + 1)) py++;
        if (k === 'up') {
          var nr = (rot + 1) % 4;
          if (!collides(piece, nr, px, py)) rot = nr;
          else if (!collides(piece, nr, px - 1, py)) { rot = nr; px--; }
          else if (!collides(piece, nr, px + 1, py)) { rot = nr; px++; }
        }
        if (k === 'space') {
          while (!collides(piece, rot, px, py + 1)) { py++; score += 2; }
          lock(); clearLines(); spawn();
        }
      },
      tick: function () {
        if (dead) return;
        if (!collides(piece, rot, px, py + 1)) py++;
        else { lock(); clearLines(); spawn(); }
      },
      draw: function (t) {
        t.header('TETRIS', 'Arrows move/rotate · Space hard-drops · Q quits');
        t.box(24, 3, W * 2 + 2, H + 2, C.blue);
        for (var r = 0; r < H; r++) for (var c = 0; c < W; c++)
          t.text(25 + c * 2, 4 + r, board[r][c] ? '██' : ' ·',
                 board[r][c] ? PCOL[board[r][c] - 1] : C.grey);
        if (!dead) for (var i = 0; i < 4; i++) {
          var cx = px + SHAPES[piece][rot][i][0], cy = py + SHAPES[piece][rot][i][1];
          if (cy >= 0 && cy < H) t.text(25 + cx * 2, 4 + cy, '██', PCOL[piece]);
        }
        t.text(50, 5, 'Score ' + score, C.fg);
        t.text(50, 6, 'Lines ' + lines, C.fg);
        t.text(50, 7, 'Level ' + level, C.fg);
        t.text(50, 9, 'Next:', C.dim);
        for (var j = 0; j < 4; j++)
          t.text(50 + SHAPES[nextp][0][j][0] * 2, 10 + SHAPES[nextp][0][j][1], '██', PCOL[nextp]);
        if (dead) t.center(H + 6, 'Game over — press any key.', C.red, true);
      }
    };
    reset();
    return self;
  }
});

/* ------------------------------------------------------------------- pong */
reg('pong', {
  title: 'Pong', help: 'Up/Down move · first to 7 · Q quits', realtime: true, step: 40,
  start: function (host, p) {
    var W = 60, H = 20, P = 4;
    var bx, by, vx, vy, py_, ay, ps, as, over;
    function serve(dir) { bx = W / 2; by = H / 2; vx = dir * 0.9; vy = rnd(2) ? 0.4 : -0.4; }
    function reset() { py_ = ay = (H - P) / 2 | 0; ps = as = 0; over = null; serve(1); }
    reset();
    return {
      key: function (k) {
        if (over) { reset(); return; }
        if (k === 'up'   && py_ > 0) py_--;
        if (k === 'down' && py_ < H - P) py_++;
      },
      tick: function () {
        if (over) return;
        bx += vx; by += vy;
        if (by <= 0) { by = 0; vy = -vy; }
        if (by >= H - 1) { by = H - 1; vy = -vy; }
        if (bx <= 2 && vx < 0) {
          if (by >= py_ - 0.5 && by <= py_ + P) {
            vx = -vx;
            vy += ((by - (py_ + P / 2)) / (P / 2)) * 0.45;
            vy = Math.max(-0.9, Math.min(0.9, vy));
          } else { as++; serve(1); }
        }
        if (bx >= W - 2 && vx > 0) {
          if (by >= ay - 0.5 && by <= ay + P) {
            vx = -vx;
            vy += ((by - (ay + P / 2)) / (P / 2)) * 0.45;
          } else { ps++; serve(-1); }
        }
        if (vx > 0) {
          var target = by - P / 2;
          if (ay < target - 0.5 && ay < H - P) ay++;
          else if (ay > target + 0.5 && ay > 0) ay--;
        }
        if (ps >= 7 || as >= 7) {
          over = ps > as ? 'You win!' : 'Computer wins.';
          host.saveScore(ps * 100 - as * 50);
        }
      },
      draw: function (t) {
        t.header('PONG', 'Up/Down move · first to 7 · Q quits');
        t.box(10, 4, W + 2, H + 2, C.blue);
        for (var i = 0; i < H; i++) t.put(10 + W / 2, 5 + i, '│', '#2e333a');
        for (i = 0; i < P; i++) {
          t.put(12, 5 + py_ + i, '█', C.cyan);
          t.put(10 + W - 1, 5 + ay + i, '█', C.red);
        }
        t.put(11 + (bx | 0), 5 + (by | 0), '●', C.yellow, null, true);
        t.center(3, ps + '   ' + as, C.fg, true);
        if (over) t.center(H + 8, over, C.white, true);
      }
    };
  }
});

/* --------------------------------------------------------------- breakout */
reg('breakout', {
  title: 'Breakout', help: 'Left/Right move · Q quits', realtime: true, step: 35,
  start: function (host, p) {
    var W = 48, H = 22, BR = 5, BC = 12, P = 7;
    var RC = [C.red, C.magenta, C.yellow, C.green, C.cyan];
    var brick, bx, by, vx, vy, paddle, score, lives, level, over;
    function levelReset() {
      brick = [];
      for (var r = 0; r < BR; r++) {
        var row = [];
        for (var c = 0; c < BC; c++) row.push((BR - r) > 2 ? 2 : 1);
        brick.push(row);
      }
      bx = W / 2; by = H - 4; vx = 0.7; vy = -0.7;
      paddle = (W - P) / 2 | 0;
    }
    function reset() { score = 0; lives = 3; level = 1; over = false; levelReset(); }
    function left() {
      var n = 0;
      brick.forEach(function (r) { r.forEach(function (v) { if (v) n++; }); });
      return n;
    }
    reset();
    return {
      key: function (k) {
        if (over) { reset(); return; }
        if (k === 'left')  paddle = Math.max(0, paddle - 2);
        if (k === 'right') paddle = Math.min(W - P, paddle + 2);
      },
      tick: function () {
        if (over) return;
        bx += vx; by += vy;
        if (bx <= 0) { bx = 0; vx = -vx; }
        if (bx >= W - 1) { bx = W - 1; vx = -vx; }
        if (by <= 0) { by = 0; vy = -vy; }
        var r = (by | 0) - 2, c = (bx / (W / BC)) | 0;
        if (r >= 0 && r < BR && c >= 0 && c < BC && brick[r][c]) {
          brick[r][c]--; score += 10; vy = -vy;
        }
        if ((by | 0) >= H - 2 && vy > 0 && bx >= paddle - 1 && bx <= paddle + P) {
          var hit = (bx - (paddle + P / 2)) / (P / 2);
          vy = -Math.abs(vy);
          vx = Math.max(-1.1, Math.min(1.1, vx + hit * 0.5));
          by = H - 2;
        }
        if (by >= H - 1) {
          lives--;
          if (lives <= 0) { over = true; host.saveScore(score); }
          else { bx = W / 2; by = H - 4; vx = 0.7; vy = -0.7; }
        }
        if (left() === 0) { level++; score += 200; levelReset(); }
      },
      draw: function (t) {
        t.header('BREAKOUT', 'Left/Right move · Q quits');
        t.box(14, 3, W + 2, H + 2, C.blue);
        for (var r = 0; r < BR; r++) for (var c = 0; c < BC; c++) {
          if (!brick[r][c]) continue;
          t.text(15 + c * 4, 6 + r, brick[r][c] === 2 ? '███' : '▒▒▒', RC[r]);
        }
        for (var i = 0; i < P; i++) t.put(15 + paddle + i, 4 + H - 1, '█', C.white);
        t.put(15 + (bx | 0), 4 + (by | 0), '●', C.yellow, null, true);
        t.text(14, H + 6, 'Score ' + score + '  Lives ' + lives + '  Level ' + level + '   ', C.fg);
        if (over) t.center(H + 8, 'Game over — press any key.', C.red, true);
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
    var alive, ax, ay, adir, ship, score, lives, wave, shots, bombs, phase, over;
    function count() {
      var n = 0;
      alive.forEach(function (r) { r.forEach(function (v) { n += v; }); });
      return n;
    }
    function waveReset() {
      alive = [];
      for (var r = 0; r < AR; r++) alive.push(new Array(AC).fill(1));
      ax = 2; ay = 1; adir = 1; shots = []; bombs = [];
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
        if (count() === 0) { wave++; score += 500; waveReset(); }
        if (lives <= 0) { over = true; host.saveScore(score); }
      },
      draw: function (t) {
        t.header('SPACE INVADERS', 'Left/Right move · Space fires · Q quits');
        t.box(14, 3, W + 2, H + 2, C.blue);
        for (var r = 0; r < AR; r++) for (var c = 0; c < AC; c++)
          if (alive[r][c]) t.put(15 + ax + c * 4, 4 + ay + r, r === 0 ? 'Ѫ' : 'ᙢ', ACOL[r], null, true);
        shots.forEach(function (s) { t.put(15 + s.x, 4 + s.y, '|', C.white); });
        bombs.forEach(function (b) { t.put(15 + b.x, 4 + b.y, '!', C.red); });
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
                     vx: (Math.random() - 0.5) * 0.6, vy: (Math.random() - 0.5) * 0.4, size: 3 });
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
            if (r.size > 1) for (var j = 0; j < 2; j++)
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
        if (!rocks.length) { wave++; score += 300; spawnRocks(Math.min(16, wave)); }
        if (lives <= 0) { over = true; host.saveScore(score); }
      },
      draw: function (t) {
        t.header('ASTEROIDS', 'Left/Right turn · Up thrusts · Space fires · Q quits');
        t.box(10, 3, W + 2, H + 2, C.blue);
        rocks.forEach(function (r) {
          t.put(11 + (r.x | 0), 4 + (r.y | 0),
                r.size === 3 ? 'O' : r.size === 2 ? 'o' : '.', C.grey, null, true);
        });
        shots.forEach(function (s) { t.put(11 + (s.x | 0), 4 + (s.y | 0), '·', C.yellow); });
        t.put(11 + (sx | 0), 4 + (sy | 0), '▲', C.cyan, null, true);
        t.text(10, H + 6, 'Score ' + score + '  Lives ' + lives + '  Wave ' + wave + '   ', C.fg);
        if (over) t.center(H + 8, 'Game over — press any key.', C.red, true);
      }
    };
  }
});

}());
