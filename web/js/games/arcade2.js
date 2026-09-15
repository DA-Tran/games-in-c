/* arcade2.js - the arcade cabinets, artillery duels, tower defence maps,
 * simulations and text adventures. Mirrors src/games/arcademisc.c and sim.c.
 */
(function () {
'use strict';
var G = window.GIC, C = G.COL, reg = G.register, rnd = G.rnd, shuffle = G.shuffle;

var W = 72, H = 20;
var M_SHOOT = 0, M_RUN = 1, M_MAZE = 2, M_THRUST = 3;

/* name, machine, density, speed, gravity, shots, diggable */
var CAB = [
  ['Centipede',        M_SHOOT,  14,  70, 0, 1, 0],
  ['Missile Command',  M_SHOOT,   8,  90, 0, 1, 0],
  ['Dig Dug',          M_MAZE,   20, 110, 0, 1, 1],
  ['Q*bert',           M_MAZE,    6, 130, 0, 0, 0],
  ['Donkey Kong',      M_THRUST,  9, 100, 1, 0, 0],
  ['Lunar Lander',     M_THRUST,  4, 100, 1, 0, 0],
  ['Moon Patrol',      M_RUN,    10,  80, 1, 1, 0],
  ['Defender',         M_THRUST, 12,  70, 0, 1, 0],
  ['Tempest',          M_THRUST, 14,  75, 0, 1, 0],
  ['Robotron',         M_MAZE,   22,  85, 0, 1, 0],
  ['Berzerk',          M_MAZE,   14, 100, 0, 1, 0],
  ['Bomberman',        M_MAZE,   18, 120, 0, 1, 1],
  ['Boulder Dash',     M_MAZE,   24, 120, 1, 0, 1],
  ['Lode Runner',      M_MAZE,   16, 110, 1, 0, 1],
  ['Pitfall',          M_RUN,     9,  85, 1, 0, 0],
  ['Kaboom',           M_SHOOT,  16,  60, 0, 0, 0],
  ['Tron Light Cycles',M_THRUST, 10,  70, 0, 0, 0],
  ['Doodle Jump',      M_RUN,     8,  70, 1, 0, 0],
  ['Icy Tower',        M_RUN,     8,  75, 1, 0, 0],
  ['Helicopter Game',  M_RUN,    12,  60, 1, 0, 0],
  ['Jetpack Joyride',  M_RUN,    14,  65, 1, 1, 0],
  ['Crossy Road',      M_RUN,    16,  95, 0, 0, 0],
  ['Whack-a-Mole',     M_SHOOT,  10, 110, 0, 1, 0],
  ['Scramble',         M_RUN,    12,  70, 1, 1, 0],
  ['Time Pilot',       M_RUN,    14,  70, 0, 1, 0],
  ['Pinball',          M_THRUST,  6,  55, 1, 0, 0],
  ['Minefield Run',    M_SHOOT,  18, 100, 0, 0, 0],
  ['Snake Charmer',    M_MAZE,   12, 110, 0, 0, 0],
  ['Cannon Angle',     M_SHOOT,   6, 120, 0, 1, 0],
  ['Echo Maze',        M_MAZE,   14, 130, 0, 0, 0]
];

function blankGrid() {
  var g = [], r, c;
  for (r = 0; r < H; r++) { g.push([]); for (c = 0; c < W; c++) g[r].push(' '); }
  return g;
}
function paint(t, grid, px, py, name, hint, score, lives, level) {
  var r, c;
  t.header('ARCADE', name + ' — ' + hint + '  score ' + score + ', lives ' + lives + ', level ' + level);
  for (r = 0; r < H; r++) for (c = 0; c < W; c++) {
    if (r === py && c === px) { t.text(2 + c, 3 + r, 'A', C.white, '#2b4a6b', true); continue; }
    var ch = grid[r][c];
    if (ch === ' ') continue;
    t.text(2 + c, 3 + r, ch,
           ch === '#' ? C.grey : ch === '*' ? C.yellow : ch === 'o' ? C.green :
           ch === 'X' ? C.red : ch === '^' ? C.cyan : C.white);
  }
}

reg('arcademisc', {
  title: 'Arcade', help: 'Arrows move · Space fires or jumps',
  start: function (host, p) {
    var v = p.variant | 0;
    if (v < 0 || v > 29) v = 0;
    var Cb = CAB[v], machine = Cb[1], density = Cb[2], gravity = Cb[4], shots = Cb[5], dig = Cb[6];
    var grid = blankGrid(), px = W >> 1, py = H - 1, vy = 0, vx = 0;
    var score = 0, lives = 3, level = 1, tick = 0, over = false;
    var fall = [], shot = {x: -1, y: -1}, ground = [], enemies = [], pellets = 0;
    var objs = [], pad = 8, fuel = 400, dist = 0;

    function reset() {
      var i, r, c;
      grid = blankGrid();
      score = 0; lives = 3; level = 1; tick = 0; over = false;
      px = W >> 1; py = H - 1; vy = 0; vx = 0; dist = 0; fuel = 400;
      fall = []; shot = {x: -1, y: -1}; enemies = []; objs = [];
      if (machine === M_RUN) { ground = []; for (i = 0; i < W; i++) ground.push(H - 2); px = 8; py = H - 3; }
      if (machine === M_MAZE) {
        for (r = 0; r < H; r++) for (c = 0; c < W; c++)
          grid[r][c] = (r === 0 || c === 0 || r === H - 1 || c === W - 1) ? '#'
                     : (rnd(100) < density ? '#' : (rnd(100) < 12 ? '*' : ' '));
        px = 1; py = 1; grid[1][1] = ' ';
        var ne = Math.min(12, 4 + (density >> 3));
        for (i = 0; i < ne; i++) enemies.push({x: W - 2 - rnd(8), y: H - 2 - rnd(6), d: rnd(4)});
        pellets = 0;
        for (r = 0; r < H; r++) for (c = 0; c < W; c++) if (grid[r][c] === '*') pellets++;
      }
      if (machine === M_THRUST) {
        px = W >> 1; py = 2;
        pad = 8 + rnd(W - 20);
        var no = Math.min(16, density);
        for (i = 0; i < no; i++) objs.push({x: rnd(W), y: 4 + rnd(H - 8)});
      }
    }
    reset();

    function stepShoot() {
      var i;
      grid = blankGrid();
      if (tick % Math.max(2, 12 - Math.min(8, density >> 2)) === 0 && fall.length < 40)
        fall.push({x: rnd(W), y: 0});
      for (i = 0; i < fall.length; i++) {
        if (tick % 2 === 0) fall[i].y++;
        if (fall[i].y >= H) {
          if (v === 22) score -= 5; else lives--;   /* a missed mole only costs points */
          fall.splice(i, 1); i--;
          continue;
        }
        grid[fall[i].y][fall[i].x] = 'X';
      }
      if (shot.y >= 0) {
        shot.y--;
        if (shot.y < 0) shot.x = -1;
        else {
          for (i = 0; i < fall.length; i++)
            if (fall[i].y === shot.y && fall[i].x === shot.x) {
              score += 10 * level;
              fall.splice(i, 1);
              shot.y = -1; shot.x = -1;
              break;
            }
          if (shot.y >= 0) grid[shot.y][shot.x] = '^';
        }
      }
      for (i = 0; i < fall.length; i++)
        if (fall[i].y === py && fall[i].x === px) { lives--; fall.splice(i, 1); i--; }
      if (score > level * 200) level++;
      if (lives <= 0) over = true;
    }
    function stepRun() {
      var i, r;
      grid = blankGrid();
      for (i = 0; i < W - 1; i++) ground[i] = ground[i + 1];
      var prev = ground[W - 2], next = prev;
      if (rnd(100) < density) next = prev + (rnd(2) ? -2 : 2);
      ground[W - 1] = Math.max(6, Math.min(H - 1, next));
      for (i = 0; i < W; i++) for (r = ground[i]; r < H; r++) grid[r][i] = '#';
      if (tick % 7 === 0 && ground[W - 1] > 0) grid[ground[W - 1] - 1][W - 1] = '*';
      if (gravity) {
        vy++;
        py += vy >> 1;
        if (py >= ground[px] - 1) { py = ground[px] - 1; vy = 0; }
        if (py < 0) { py = 0; vy = 0; }
      }
      if (py >= 0 && py < H) {
        if (grid[py][px] === '#') { lives--; py = ground[px] - 1; vy = 0; }
        if (grid[py][px] === '*') { score += 25; grid[py][px] = ' '; }
      }
      dist++; score++;
      if (dist % 300 === 0) level++;
      if (lives <= 0) over = true;
    }
    function stepMaze() {
      var DR = [-1,1,0,0], DC = [0,0,-1,1], i;
      if (tick % 3 === 0)
        for (i = 0; i < enemies.length; i++) {
          var e = enemies[i];
          if (rnd(100) < 40) {
            var want = e.y < py ? 1 : e.y > py ? 0 : e.x < px ? 3 : 2;
            if (grid[e.y + DR[want]] && grid[e.y + DR[want]][e.x + DC[want]] !== '#') e.d = want;
          }
          var nr = e.y + DR[e.d], nc = e.x + DC[e.d];
          if (nr > 0 && nr < H - 1 && nc > 0 && nc < W - 1 && grid[nr][nc] !== '#') { e.y = nr; e.x = nc; }
          else e.d = rnd(4);
          if (e.x === px && e.y === py) { lives--; px = 1; py = 1; if (lives <= 0) over = true; }
        }
      if (!pellets) { level++; score += 200; over = true; }
    }
    function stepThrust() {
      var i;
      grid = blankGrid();
      for (i = 0; i < W; i++) grid[H - 1][i] = '#';
      for (i = 0; i < 4; i++) if (pad + i < W) grid[H - 1][pad + i] = '=';
      for (i = 0; i < objs.length; i++) {
        if (tick % 4 === 0) objs[i].x = (objs[i].x + 1) % W;
        grid[objs[i].y][objs[i].x] = 'X';
      }
      if (gravity && tick % 2 === 0) vy++;
      px += vx >> 1;
      py += vy >> 1;
      if (px < 0) px = W - 1;
      if (px >= W) px = 0;
      if (py < 0) { py = 0; vy = 0; }
      if (py >= H - 1) {
        if (px >= pad && px < pad + 4 && vy <= 4) { score += 500 + fuel; level++; pad = 8 + rnd(W - 20); fuel = 400; }
        else lives--;
        py = 2; px = W >> 1; vx = 0; vy = 0;
        if (lives <= 0) over = true;
      }
      for (i = 0; i < objs.length; i++)
        if (objs[i].x === px && objs[i].y === py) {
          lives--; py = 2; px = W >> 1; vx = 0; vy = 0;
          if (lives <= 0) over = true;
        }
    }
    return {
      tick: function () {
        if (over) return;
        if (machine === M_SHOOT) stepShoot();
        else if (machine === M_RUN) stepRun();
        else if (machine === M_MAZE) stepMaze();
        else stepThrust();
        tick++;
      },
      key: function (k) {
        if (over) { host.saveScore(score); reset(); return; }
        if (machine === M_MAZE) {
          var DR = [-1,1,0,0], DC = [0,0,-1,1], nx = px, ny = py, d;
          if (k === 'up') ny--; if (k === 'down') ny++;
          if (k === 'left') nx--; if (k === 'right') nx++;
          if (nx > 0 && nx < W - 1 && ny > 0 && ny < H - 1) {
            if (grid[ny][nx] === '#') { if (dig && (k === 'space' || k === 'enter')) { grid[ny][nx] = ' '; score += 2; } }
            else {
              if (grid[ny][nx] === '*') { score += 10; pellets--; grid[ny][nx] = ' '; }
              px = nx; py = ny;
            }
          }
          if ((k === 'space' || k === 'enter') && dig)
            for (d = 0; d < 4; d++) {
              var br = py + DR[d], bc = px + DC[d];
              if (br > 0 && br < H - 1 && bc > 0 && bc < W - 1 && grid[br][bc] === '#') { grid[br][bc] = ' '; score += 2; }
            }
          return;
        }
        if (machine === M_THRUST) {
          if (fuel > 0) {
            if (k === 'left')  { vx--; fuel--; }
            if (k === 'right') { vx++; fuel--; }
            if (k === 'up' || k === 'space') { vy -= 3; fuel -= 2; }
          }
          vx = Math.max(-6, Math.min(6, vx));
          return;
        }
        if (k === 'left'  && px > 0)     px--;
        if (k === 'right' && px < W - 1) px++;
        if (machine === M_SHOOT && (k === 'space' || k === 'enter') && shots && shot.y < 0)
          { shot.x = px; shot.y = py - 1; }
        if (machine === M_RUN) {
          if (k === 'up' || k === 'space') { if (py >= ground[px] - 1) vy = -6; }
          if (k === 'down' && py < H - 1) py++;
        }
      },
      draw: function (t) {
        var hint = machine === M_SHOOT ? (shots ? 'arrows move, Space fires' : 'arrows move to catch')
                 : machine === M_RUN   ? (gravity ? 'Space jumps, arrows steer' : 'arrows steer')
                 : machine === M_MAZE  ? (dig ? 'arrows move, Space digs' : 'arrows move')
                                       : 'arrows thrust - land gently on the pad';
        paint(t, grid, px, py, CAB[v][0], hint, score, lives, level);
        if (machine === M_THRUST) t.text(2, 4 + H, 'fuel ' + fuel + '  vertical speed ' + vy, C.grey);
        if (over) t.text(2, 5 + H, 'Out of lives — press any key.', C.red);
      }
    };
  }
});

/* ============================================================= ARTILLERY */
reg('artillery', {
  title: 'Artillery', help: 'Left/right aim · up/down power · Space fires',
  start: function (host, p) {
    var v = p.variant | 0;
    if (v < 0 || v > 8) v = 0;
    var NAME = ['Gorillas','Scorched Earth','Artillery Duel Wind','Artillery Duel Gravity',
      'Artillery Duel Terrain','Artillery Duel Multi Shot','Artillery Moving Target',
      'Tank Battle','Worms Lite'];
    var wind = v === 2, grav = v === 3 ? 4 : 2, destructible = (v === 1 || v === 4 || v === 8);
    var shots = v === 5 ? 3 : 1, moving = v === 6;
    var terrain = [], myx = 5, foex = W - 6, angle = 45, power = 50, windnow = 0;
    var hits = 0, taken = 0, score = 0, shell = null, over = 0, lastmiss = 0;

    function reset() {
      terrain = [];
      for (var i = 0; i < W; i++) {
        var z = i % 17;
        terrain.push(H - 3 - ((z < 9 ? z : 17 - z) >> 2));
      }
      myx = 5; foex = W - 6; hits = 0; taken = 0; score = 0; over = 0; shell = null;
      windnow = wind ? rnd(9) - 4 : 0;
    }
    function fire() {
      shell = { fx: myx * 100, fy: (terrain[myx] - 2) * 100,
                vx: power * (90 - angle) / 45, vy: -power * angle / 30, step: 0 };
    }
    function returnFire() {
      /* It ranges in: every miss nudges its aim, so accuracy climbs. */
      var accuracy = Math.max(10, 30 + hits * 8 - Math.abs(lastmiss));
      if (rnd(100) < accuracy) { taken++; score -= 25; lastmiss = 0; }
      else lastmiss += rnd(2) ? 3 : -3;
      if (taken >= 3) over = 2;
      if (moving) { foex = Math.max(10, Math.min(W - 6, foex + (rnd(2) ? 3 : -3))); }
      if (wind) windnow = rnd(9) - 4;
    }
    reset();
    return {
      tick: function () {
        if (over || !shell) return;
        var i;
        for (i = 0; i < 3 && shell; i++) {
          shell.fx += shell.vx; shell.fy += shell.vy;
          shell.vy += grav * 2;
          shell.vx += windnow;
          shell.step++;
          var gx = shell.fx / 100 | 0, gy = shell.fy / 100 | 0;
          if (gx < 0 || gx >= W || gy >= H || shell.step > 400) { shell = null; returnFire(); break; }
          if (gy >= 0 && gy >= terrain[gx]) {
            if (gx >= foex - 1 && gx <= foex + 1) { hits++; score += 100; if (hits >= 3) over = 1; }
            if (destructible)
              for (var d = -2; d <= 2; d++)
                if (gx + d >= 0 && gx + d < W && terrain[gx + d] < H - 1) terrain[gx + d]++;
            shell = null;
            if (!over) returnFire();
            break;
          }
        }
      },
      key: function (k) {
        if (over) { host.saveScore(score); reset(); return; }
        if (shell) return;
        if (k === 'left'  && angle > 5)   angle -= 5;
        if (k === 'right' && angle < 85)  angle += 5;
        if (k === 'up'    && power < 100) power += 5;
        if (k === 'down'  && power > 10)  power -= 5;
        if (k === 'space' || k === 'enter') fire();
      },
      draw: function (t) {
        var grid = blankGrid(), i, r;
        for (i = 0; i < W; i++) for (r = terrain[i]; r < H; r++) grid[r][i] = '#';
        if (terrain[myx] > 0)  grid[terrain[myx] - 1][myx] = 'A';
        if (terrain[foex] > 0) grid[terrain[foex] - 1][foex] = 'X';
        if (shell) {
          var gx = shell.fx / 100 | 0, gy = shell.fy / 100 | 0;
          if (gx >= 0 && gx < W && gy >= 0 && gy < H) grid[gy][gx] = '^';
        }
        paint(t, grid, -1, -1, NAME[v], 'left/right aim, up/down power, Space fires', score, 3 - taken, hits + 1);
        t.text(2, 4 + H, 'angle ' + angle + '  power ' + power + '  wind ' +
               (windnow >= 0 ? '+' : '') + windnow + '  gravity ' + grav +
               '  hits ' + hits + ', taken ' + taken, C.grey);
        if (over) t.text(2, 6 + H, over === 1 ? 'Target destroyed — press any key.'
                                              : 'You were knocked out — press any key.',
                         over === 1 ? C.green : C.red);
      }
    };
  }
});

/* ========================================================= TOWER DEFENCE */
reg('towerdefence', {
  title: 'Tower defence', help: 'Arrows move · Enter builds · U upgrades',
  start: function (host, p) {
    var v = p.variant | 0;
    if (v < 0 || v > 5) v = 0;
    var NAME = ['Forest','Desert','Space','Castle','Cyber','Underwater'];
    var path = [], towers = [], creeps = [], money = 120, health = 20, wave = 1;
    var tick = 0, cr = H >> 1, cc = W >> 1, score = 0, over = false;

    function reset() {
      var i, r, c;
      path = []; towers = []; creeps = [];
      money = 120; health = 20; wave = 1; tick = 0; score = 0; over = false;
      cr = H >> 1; cc = W >> 1;
      for (r = 0; r < H; r++) { towers.push([]); for (c = 0; c < W; c++) towers[r].push(0); }
      /* Each map bends its lane differently, which changes where towers pay. */
      for (i = 0; i < W; i++) {
        var base = H >> 1;
        if (v === 0) base += ((i / 6 | 0) % 2) ? 3 : -3;
        if (v === 1) base += (i % 24 < 12) ? (i % 12) - 6 : 6 - (i % 12);
        if (v === 2) base += (i % 8 < 4) ? 2 : -2;
        if (v === 3) base += (i < (W >> 1)) ? -4 : 4;
        if (v === 4) base += ((i * 7) % 11) - 5;
        if (v === 5) base += (i % 16 < 8) ? -3 : 3;
        path.push(Math.max(1, Math.min(H - 2, base)));
      }
    }
    reset();
    return {
      tick: function () {
        if (over) return;
        var i, r, c;
        if (tick % Math.max(4, 26 - Math.min(18, wave)) === 0 && creeps.length < 40)
          creeps.push({x: 0, hp: 4 + wave * 2});
        for (i = 0; i < creeps.length; i++) {
          if (tick % 3 === 0) creeps[i].x++;
          if (creeps[i].x >= W) { health--; creeps.splice(i, 1); i--; }
        }
        if (tick % 2 === 0)
          for (r = 0; r < H; r++) for (c = 0; c < W; c++) {
            if (!towers[r][c]) continue;
            for (i = 0; i < creeps.length; i++) {
              var dx = creeps[i].x - c, dy = path[creeps[i].x] - r;
              if (dx * dx + dy * dy <= 16) {
                creeps[i].hp -= towers[r][c];
                if (creeps[i].hp <= 0) { money += 8; score += 15; creeps.splice(i, 1); }
                break;
              }
            }
          }
        if (health <= 0) over = true;
        if (tick % 400 === 399) wave++;
        tick++;
      },
      key: function (k) {
        if (over) { host.saveScore(score); reset(); return; }
        if (k === 'up'    && cr > 0)     cr--;
        if (k === 'down'  && cr < H - 1) cr++;
        if (k === 'left'  && cc > 0)     cc--;
        if (k === 'right' && cc < W - 1) cc++;
        if ((k === 'enter' || k === 'space') && money >= 30 && !towers[cr][cc] && path[cc] !== cr)
          { towers[cr][cc] = 3; money -= 30; }
        if (k === 'u' && money >= 40 && towers[cr][cc]) { towers[cr][cc] += 3; money -= 40; }
      },
      draw: function (t) {
        var grid = blankGrid(), i, r, c;
        for (i = 0; i < W; i++) grid[path[i]][i] = '.';
        for (r = 0; r < H; r++) for (c = 0; c < W; c++) if (towers[r][c]) grid[r][c] = 'o';
        for (i = 0; i < creeps.length; i++) grid[path[creeps[i].x]][creeps[i].x] = 'X';
        paint(t, grid, cc, cr, NAME[v], 'arrows move, Enter builds (30), U upgrades (40)', score, health, wave);
        t.text(2, 4 + H, 'money ' + money + '  health ' + health + '  wave ' + wave +
               '  creeps ' + creeps.length, C.grey);
        if (over) t.text(2, 6 + H, 'The lane was overrun — press any key.', C.red);
      }
    };
  }
});

/* =========================================================== SIMULATIONS */
var SIM = [
 ['Hammurabi', ['bushels','acres','people','rats'], [2800,1000,100,0],
  ['buy land','sell land','feed the people','plant crops'], 20, 'keep the city alive for twenty years'],
 ['Lemonade Stand', ['cash','cups','lemons','reputation'], [50,0,0,50],
  ['buy lemons','make cups','set a high price','set a low price'], 20, 'end the season with the most cash'],
 ['Oregon Trail', ['miles','food','health','oxen'], [0,400,100,4],
  ['travel on','rest a day','hunt','trade'], 30, 'reach two thousand miles alive'],
 ['Star Trek 1971', ['energy','torpedoes','shields','klingons'], [3000,10,100,12],
  ['fire phasers','fire a torpedo','raise shields','warp away'], 25, 'clear the quadrant'],
 ['Drug Wars', ['cash','stock','heat','debt'], [2000,0,0,5500],
  ['buy stock','sell stock','lie low','pay the debt'], 30, 'clear the debt and bank the rest'],
 ['Sim Farm', ['cash','crops','livestock','soil'], [500,0,0,80],
  ['plant','buy livestock','fertilise','sell at market'], 25, 'keep the soil good and the farm solvent'],
 ['Sim City Lite', ['funds','population','industry','pollution'], [1000,100,0,0],
  ['build housing','build industry','raise taxes','clean up'], 30, 'grow the city without choking it'],
 ['Railroad Tycoon Lite', ['cash','track','trains','freight'], [800,0,1,0],
  ['lay track','buy a train','run freight','raise capital'], 25, 'build a network that pays'],
 ['Civilisation Lite', ['food','production','science','population'], [100,10,0,20],
  ['farm','build','research','expand'], 30, 'reach a hundred science'],
 ['Risk', ['armies','territories','cards','enemies'], [20,3,0,9],
  ['attack','fortify','recruit','trade cards'], 25, 'take every territory'],
 ['Stock Market Sim', ['cash','shares','price','trend'], [5000,0,50,0],
  ['buy','sell','hold','short'], 30, 'beat the market'],
 ['Elevator Simulator', ['floor','waiting','served','patience'], [1,0,0,100],
  ['go up','go down','open the doors','express'], 30, 'serve everyone before patience runs out'],
 ['Traffic Light Sim', ['queued','passed','crashes','cycle'], [0,0,0,0],
  ['green north','green east','all red','shorten the cycle'], 30, 'pass the most cars without a crash'],
 ['Ant Colony Sim', ['ants','food','tunnels','threat'], [20,50,1,0],
  ['forage','dig','breed','defend'], 30, 'grow the colony past two hundred'],
 ['Epidemic Sim', ['healthy','infected','recovered','funds'], [1000,5,0,300],
  ['vaccinate','quarantine','treat','do nothing'], 25, 'stop the outbreak'],
 ['Ecosystem Balance', ['grass','rabbits','foxes','seasons'], [500,40,8,0],
  ['seed grass','cull rabbits','cull foxes','leave it alone'], 30, 'keep all three alive'],
 ['Power Grid Sim', ['capacity','demand','fuel','outages'], [100,80,200,0],
  ['build capacity','buy fuel','load shed','upgrade'], 25, 'keep the lights on'],
 ['Airport Control', ['stacked','landed','fuel warnings','runways'], [0,0,0,2],
  ['land one','hold one','open a runway','divert'], 30, 'land them all'],
 ['Restaurant Tycoon', ['cash','tables','staff','rating'], [600,4,2,50],
  ['hire','buy tables','advertise','improve the menu'], 25, 'reach a rating of ninety'],
 ['Space Colony', ['air','water','colonists','power'], [100,100,10,100],
  ['recycle air','drill for water','grow the colony','build power'], 30, 'survive thirty months'],
 ['Wa-Tor', ['fish','sharks','water','chronons'], [200,20,100,0],
  ['seed fish','seed sharks','widen the sea','just watch'], 30, 'keep both species alive'],
 ['Bridge Builder', ['budget','spans','strength','load'], [1000,0,0,0],
  ['add a span','reinforce','test the load','cut costs'], 20, 'carry the load within budget']
];

/* The feedback loop is the game, so the ones with a real loop get a real one. */
function simStep(v, r, act) {
  var i, log = '';
  if (v === 0) {
    var price = 17 + rnd(10), harvest = 1 + rnd(5);
    if (act === 0) { if (r[0] >= price * 10) { r[0] -= price * 10; r[1] += 10; } }
    if (act === 1) { if (r[1] >= 10) { r[1] -= 10; r[0] += price * 10; } }
    if (act === 2) { var need = r[2] * 20;
                     if (r[0] >= need) r[0] -= need; else { r[2] -= ((need - r[0]) / 20) | 0; r[0] = 0; } }
    if (act === 3) r[0] += r[1] * harvest;
    r[3] = (r[0] / 40) | 0;
    r[0] -= r[3];
    r[2] += rnd(5) - 1;
    log = 'land at ' + price + ' bushels an acre, harvest ' + harvest + ' per acre, rats ate ' + r[3];
  } else if (v === 1) {
    var weather = rnd(100), sold;
    if (act === 0) { if (r[0] >= 20) { r[0] -= 20; r[2] += 10; } }
    if (act === 1) { var n = Math.min(r[2], 10); r[2] -= n; r[1] += n; }
    sold = Math.min(r[1], (weather / 8) | 0);
    if (act === 2) { sold = sold >> 1; r[0] += sold * 4; r[3] -= 2; }
    else { r[0] += sold * 2; r[3] += 1; }
    r[1] -= sold;
    log = 'weather ' + weather + ', sold ' + sold + ' cups';
  } else if (v === 2) {
    if (act === 0) { r[0] += 40 + r[3] * 10; r[1] -= 30; r[2] -= 4; }
    if (act === 1) { r[2] += 12; r[1] -= 12; }
    if (act === 2) { r[1] += 30 + rnd(60); r[2] -= 2; }
    if (act === 3) { if (r[1] > 50) { r[1] -= 50; r[3]++; } }
    if (r[1] < 0) { r[2] += (r[1] / 4) | 0; r[1] = 0; }
    log = rnd(100) < 18 ? (r[2] -= 10, 'dysentery in the party') : r[0] + ' miles covered so far';
  } else if (v === 20) {
    /* The classic predator-prey oscillation, in integers. */
    var births = (r[0] / 4) | 0, eaten = r[1] * 3;
    if (act === 0) r[0] += 50;
    if (act === 1) r[1] += 5;
    if (act === 2) r[2] += 20;
    if (eaten > r[0]) eaten = r[0];
    r[0] += births - eaten;
    r[1] += ((eaten / 8) | 0) - ((r[1] / 5) | 0);
    if (r[0] > r[2] * 6) r[0] = r[2] * 6;
    log = births + ' fish born, ' + eaten + ' eaten';
  } else if (v === 15) {
    var grazed = Math.min(r[1] * 4, r[0]), hunted = Math.min(r[2] * 3, r[1]);
    if (act === 0) r[0] += 200;
    if (act === 1) r[1] -= (r[1] / 4) | 0;
    if (act === 2) r[2] -= (r[2] / 4) | 0;
    r[0] += 80 - grazed;
    r[1] += ((grazed / 5) | 0) - hunted;
    r[2] += ((hunted / 4) | 0) - ((r[2] / 6) | 0);
    r[3]++;
    log = grazed + ' grass grazed, ' + hunted + ' rabbits taken';
  } else if (v === 14) {
    var newcases = ((r[1] * (r[0] > 0 ? 2 : 0)) / 3) | 0;
    if (act === 0) { if (r[3] >= 100) { r[3] -= 100; r[0] -= 100; r[2] += 100; } }
    if (act === 1) { r[3] -= 50; newcases = (newcases / 3) | 0; }
    if (act === 2) { r[3] -= 60; r[2] += r[1] >> 1; r[1] -= r[1] >> 1; }
    if (newcases > r[0]) newcases = r[0];
    r[0] -= newcases;
    r[1] += newcases - ((r[1] / 4) | 0);
    r[2] += (r[1] / 4) | 0;
    r[3] += 40;
    log = newcases + ' new cases this week';
  } else if (v === 6) {
    if (act === 0) { if (r[0] >= 200) { r[0] -= 200; r[1] += 120; } }
    if (act === 1) { if (r[0] >= 300) { r[0] -= 300; r[2] += 40; r[3] += 15; } }
    if (act === 2) { r[0] += r[1] >> 1; r[1] -= (r[1] / 20) | 0; }
    if (act === 3) { if (r[0] >= 150) { r[0] -= 150; r[3] -= 30; } }
    r[0] += r[2] * 3;
    r[3] += (r[2] / 8) | 0;
    if (r[3] > 60) r[1] -= r[3] >> 1;          /* people leave a filthy city */
    log = 'industry paid ' + (r[2] * 3) + ', pollution ' + Math.max(0, r[3]);
  } else {
    var swing = rnd(20) - 8;
    if (act === 0) { if (r[0] >= 50) { r[0] -= 50; r[1] += 10; } }
    if (act === 1) { if (r[1] >= 5)  { r[1] -= 5;  r[2] += 8; } }
    if (act === 2) { r[3] += 10; r[0] -= 20; }
    if (act === 3) { r[0] += r[2] * 2; }
    r[0] += r[1] + swing;
    r[2] += (r[3] / 20) | 0;
    log = 'conditions moved by ' + (swing >= 0 ? '+' : '') + swing;
  }
  for (i = 0; i < 4; i++) if (r[i] < 0) r[i] = 0;
  return log;
}

reg('sim', {
  title: 'Simulation', help: 'Arrows choose · Enter commits',
  start: function (host, p) {
    var v = p.variant | 0;
    if (v < 0 || v > 21) v = 0;
    var S = SIM[v], res = [], turn = 0, cur = 0, log = '';
    function reset() { res = S[2].slice(); turn = 0; cur = 0; log = ''; }
    reset();
    return {
      key: function (k) {
        if (turn >= S[4]) { host.saveScore(res[0] + res[1] + res[2]); reset(); return; }
        if (k === 'up')   { cur = (cur + 3) % 4; return; }
        if (k === 'down') { cur = (cur + 1) % 4; return; }
        if (k !== 'enter' && k !== 'space') return;
        log = simStep(v, res, cur);
        turn++;
      },
      draw: function (t) {
        var i;
        t.header('SIMULATION', S[0] + ' — turn ' + Math.min(turn + 1, S[4]) + ' of ' + S[4] +
                 '. Arrows choose, Enter commits');
        t.text(6, 3, 'Goal: ' + S[5], C.grey);
        for (i = 0; i < 4; i++) {
          t.text(8, 5 + i, (S[1][i] + '              ').slice(0, 14), C.white);
          t.text(24, 5 + i, ('      ' + res[i]).slice(-6), C.yellow);
        }
        t.text(6, 10, 'this turn:', C.grey);
        for (i = 0; i < 4; i++)
          t.text(8, 11 + i, (S[3][i] + '                          ').slice(0, 26), C.white,
                 i === cur ? '#2b4a6b' : null);
        t.text(6, 16, turn >= S[4] ? 'Run complete — press any key.' : log,
               turn >= S[4] ? C.green : C.yellow);
      }
    };
  }
});

/* ======================================================== TEXT ADVENTURE */
var ADV = [
 ['Text Adventure',      ['hall','cellar','study','garden','attic','gate'],
  ['lamp','key','rope','coin','map'], 'shadow', 20, 30, 'You step out through the gate.'],
 ['Colossal Cave Lite',  ['grate','crawl','hall of mists','dome','canyon','debris'],
  ['lamp','keys','cage','rod','bird'], 'dwarf', 18, 30, 'You surface with the treasure.'],
 ['Zork Lite',           ['west of house','kitchen','living room','cellar','maze','altar'],
  ['lantern','sword','sack','egg','torch'], 'grue', 24, 30, 'The trophy case is full.'],
 ['Escape the Room',     ['office','closet','corridor','vault','lobby','stairwell'],
  ['keycard','note','battery','screwdriver','code'], 'alarm', 14, 25, 'The door clicks open.'],
 ['Quest for the Grail', ['chapel','bridge','forest','castle','ford','cave'],
  ['shield','relic','horn','banner','chalice'], 'black knight', 26, 32, 'The grail is yours.'],
 ['Choose Your Path',    ['crossroads','village','river','ridge','market','shrine'],
  ['charm','letter','flask','token','cloak'], 'stranger', 16, 28, 'You chose well.'],
 ['Vampire Castle',      ['crypt','chapel','tower','library','cellar','courtyard'],
  ['stake','garlic','mirror','cross','flask'], 'count', 30, 30, 'Dawn breaks and the count is dust.'],
 ['Zombie Survival',     ['mall','pharmacy','rooftop','car park','clinic','tunnel'],
  ['bat','medkit','fuel','radio','ammo'], 'horde', 34, 30, 'The helicopter lifts off.'],
 ['Space Trader',        ['dock','hold','bridge','market','engine bay','airlock'],
  ['fuel cell','manifest','scanner','credits','sidearm'], 'pirate', 22, 30, 'You dock rich and whole.'],
 ['Monster Arena',       ['pit','stands','tunnel','gate','armoury','ring'],
  ['blade','tonic','charm','shield','net'], 'champion', 36, 34, 'The crowd is on its feet.'],
 ['Wizard Duel',         ['circle','tower','vault','observatory','cloister','stair'],
  ['wand','grimoire','rune','phial','focus'], 'rival', 28, 28, 'Their last shield falls.'],
 ['Gladiator Manager',   ['barracks','sands','gate','infirmary','stable','box'],
  ['trident','net','balm','helm','contract'], 'champion', 30, 32, 'Your stable takes the laurel.'],
 ['Merchant Sim',        ['wharf','warehouse','road','bazaar','counting house','inn'],
  ['ledger','silk','spice','letter of credit','guard'], 'brigand', 20, 30, 'The caravan gets through.'],
 ['Tower Climb RPG',     ['landing','stair','gallery','cell','spire','vault'],
  ['sword','potion','ring','scroll','lantern'], 'warden', 32, 32, 'You reach the spire.'],
 ['Pet Monster Battler', ['meadow','lab','gym','cave','route','centre'],
  ['ball','potion','stone','badge','berry'], 'champion', 26, 30, 'The badge is yours.'],
 ['Dungeon of Doom',     ['entrance','crypt','armoury','well','shrine','deep'],
  ['sword','shield','potion','amulet','torch'], 'lich', 38, 34, 'The amulet is recovered.'],
 ['Rogue',               ['room','corridor','vault','stair','larder','shrine'],
  ['mace','ration','scroll','ring','wand'], 'hobgoblin', 24, 30, 'You climb out alive.'],
 ['NetHack Lite',        ['level','sokoban','mines','temple','shop','oracle'],
  ['pick-axe','ration','wand','amulet','scroll'], 'mind flayer', 30, 30, 'You ascend.'],
 ['Angband Lite',        ['town','level','pit','vault','stair','cave'],
  ['lantern','potion','scroll','ring','blade'], 'balrog', 40, 36, "Morgoth's servant falls."],
 ['Hunt the Wumpus',     ['cave','tunnel','pit room','bat roost','hollow','den'],
  ['arrow','arrow','arrow','arrow','rope'], 'wumpus', 12, 20, 'The wumpus is slain.']
];

reg('textadv', {
  title: 'Adventure', help: 'Arrows travel · T takes · F fights',
  start: function (host, p) {
    var v = p.variant | 0;
    if (v < 0 || v > 19) v = 0;
    var A = ADV[v], NROOM = 12;
    var exits = [], item = [], here = 0, hp = A[5], foehp = A[4], carried = [], ncarry = 0;
    var moves = 0, foeroom = 6, exitroom = NROOM - 1, log = '', over = 0;
    function reset() {
      var i;
      exits = []; item = []; carried = [false,false,false,false,false];
      /* A ring so everywhere is reachable, plus shortcuts so it is not a corridor. */
      for (i = 0; i < NROOM; i++) {
        exits.push([(i + 1) % NROOM, (i + NROOM - 1) % NROOM,
                    rnd(100) < 40 ? rnd(NROOM) : -1, rnd(100) < 40 ? rnd(NROOM) : -1]);
        item.push(-1);
      }
      for (i = 0; i < 5; i++) item[1 + rnd(NROOM - 1)] = i;
      here = 0; hp = A[5]; foehp = A[4]; ncarry = 0; moves = 0; over = 0;
      foeroom = (NROOM >> 1) + rnd(NROOM >> 1);
      log = 'You are in the ' + A[1][0] + '.';
    }
    reset();
    return {
      key: function (k) {
        if (over) { host.saveScore(over === 1 ? 500 + ncarry * 50 : ncarry * 20); reset(); return; }
        var dir = k === 'up' ? 0 : k === 'down' ? 1 : k === 'right' ? 2 : k === 'left' ? 3 : -1;
        if (dir >= 0) {
          if (exits[here][dir] < 0) { log = 'You cannot go that way.'; return; }
          if (here === foeroom && foehp > 0) { log = 'The ' + A[3] + ' will not let you past.'; return; }
          here = exits[here][dir];
          moves++;
          log = 'You go ' + ['north','south','east','west'][dir] + '.';
          if (here === exitroom && foehp <= 0) over = 1;
          return;
        }
        if (k === 't') {
          if (item[here] < 0) { log = 'There is nothing to take.'; return; }
          carried[item[here]] = true;
          ncarry++;
          log = 'Taken: the ' + A[2][item[here]] + '.';
          item[here] = -1;
          return;
        }
        if (k === 'f') {
          if (here !== foeroom || foehp <= 0) { log = 'There is nothing to fight.'; return; }
          /* What you carry is the whole difference between winning and losing. */
          var mine = 3 + ncarry * 2 + rnd(4), theirs = 2 + rnd(5);
          foehp -= mine;
          hp -= theirs;
          log = foehp <= 0 ? 'You strike for ' + mine + ' — the ' + A[3] + ' falls.'
                           : 'You hit for ' + mine + ', it hits back for ' + theirs + '.';
          if (hp <= 0) over = 2;
        }
      },
      draw: function (t) {
        var i, j, col = 12;
        t.header('ADVENTURE', A[0] + ' — arrows go north/south/east/west, T takes, F fights');
        t.text(6, 3, 'You are in the ' + A[1][here % 6] + ' (' + here + ').', C.white);
        t.text(6, 5, 'Exits:', C.grey);
        for (i = 0; i < 4; i++)
          if (exits[here][i] >= 0) { t.text(col, 5, ['north','south','east','west'][i], C.cyan); col += 8; }
        if (item[here] >= 0) t.text(6, 7, 'You can see a ' + A[2][item[here]] + ' here.', C.yellow);
        if (here === foeroom && foehp > 0) t.text(6, 8, 'A ' + A[3] + ' blocks the way (' + foehp + ').', C.red);
        if (here === exitroom && foehp <= 0) t.text(6, 8, 'The way out is here.', C.green);
        t.text(6, 11, 'health ' + hp + '   carrying ' + ncarry + ' of 5   moves ' + moves, C.white);
        for (i = 0, j = 0; i < 5; i++)
          if (carried[i]) { t.text(6 + j * 14, 12, A[2][i], C.grey); j++; }
        t.text(6, 14, over ? (over === 1 ? A[6] : 'You did not make it out.') : log,
               over === 1 ? C.green : over === 2 ? C.red : C.yellow);
      }
    };
  }
});

})();
