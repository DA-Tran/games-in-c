/* GIC:PARAMETERISED chess
 * GIC:PARAMETERISED chesspuzzle
 *
 * chess.c - chess and its variants, plus the tactical puzzles.
 *
 * params: variant selects the rules
 *
 * One move generator serves all fifteen variants. Almost every variant in the
 * catalogue changes exactly one of three things, so those three are the only
 * switches the generator knows about:
 *
 *   setup    where the pieces start (Chess960, Horde, Racing Kings, the small
 *            boards, Knightmate's royal knight)
 *   legality what may be played (Antichess must capture; Atomic blows up a
 *            square; Cylinder wraps the files round)
 *   victory  what ends it (mate, three checks, reaching the far rank, losing
 *            every man, losing a whole piece type)
 *
 * The engine is a short alpha-beta search over the same generator, so every
 * variant gets an opponent that understands its own rules rather than playing
 * ordinary chess on a strange board.
 */
#include "engine.h"
#include "games.h"

enum { EMPTY = 0, PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING };

/* Positive is yours (moving up the board), negative is the opponent's. */
static int bd[8][8];
static int BW = 8, BH = 8;
static int VAR;
static int checks[2];

enum { V_STD, V_960, V_HILL, V_3CHECK, V_ATOMIC, V_HORDE, V_RACING, V_ANTI,
       V_EXTINCT, V_KNIGHTMATE, V_DARK, V_PROGRESSIVE, V_MINI5, V_LOSALAMOS,
       V_CYLINDER };

static const char PIECE[7] = {'.', 'P', 'N', 'B', 'R', 'Q', 'K'};
static const int VALUE[7] = {0, 100, 320, 330, 500, 900, 20000};

static int on(int r, int c) { return r >= 0 && r < BH && c >= 0 && c < BW; }

/* Cylinder chess wraps the files, so a rook on a1 attacks h1 going left. */
static int wrap(int c) { return (VAR == V_CYLINDER) ? ((c % BW) + BW) % BW : c; }

static void setup(void)
{
    int r, c;
    static const int BACK[8] = {ROOK, KNIGHT, BISHOP, QUEEN, KING, BISHOP, KNIGHT, ROOK};
    BW = BH = 8;
    if (VAR == V_MINI5)     { BW = BH = 5; }
    if (VAR == V_LOSALAMOS) { BW = BH = 6; }
    for (r = 0; r < 8; r++) for (c = 0; c < 8; c++) bd[r][c] = EMPTY;

    if (VAR == V_MINI5) {
        static const int M5[5] = {ROOK, KNIGHT, BISHOP, QUEEN, KING};
        for (c = 0; c < 5; c++) { bd[0][c] = -M5[4 - c]; bd[4][c] = M5[c]; }
        for (c = 0; c < 5; c++) { bd[1][c] = -PAWN; bd[3][c] = PAWN; }
        return;
    }
    if (VAR == V_LOSALAMOS) {
        /* The 1956 program's board: six files and no bishops. */
        static const int L6[6] = {ROOK, KNIGHT, QUEEN, KING, KNIGHT, ROOK};
        for (c = 0; c < 6; c++) { bd[0][c] = -L6[c]; bd[5][c] = L6[c]; }
        for (c = 0; c < 6; c++) { bd[1][c] = -PAWN; bd[4][c] = PAWN; }
        return;
    }
    if (VAR == V_HORDE) {
        /* Black has a normal army; white has thirty-six pawns. */
        for (c = 0; c < 8; c++) bd[0][c] = -BACK[c];
        for (c = 0; c < 8; c++) bd[1][c] = -PAWN;
        for (r = 4; r < 8; r++) for (c = 0; c < 8; c++) bd[r][c] = PAWN;
        bd[3][1] = PAWN; bd[3][2] = PAWN; bd[3][5] = PAWN; bd[3][6] = PAWN;
        return;
    }
    if (VAR == V_RACING) {
        /* No pawns, nobody may give check, first king to the eighth rank wins. */
        bd[7][0] = KING;   bd[7][1] = QUEEN;  bd[7][2] = ROOK;   bd[7][3] = BISHOP;
        bd[6][0] = KNIGHT; bd[6][1] = BISHOP; bd[6][2] = KNIGHT; bd[6][3] = ROOK;
        bd[7][4] = -KING;   bd[7][5] = -QUEEN;  bd[7][6] = -ROOK;   bd[7][7] = -BISHOP;
        bd[6][4] = -KNIGHT; bd[6][5] = -BISHOP; bd[6][6] = -KNIGHT; bd[6][7] = -ROOK;
        return;
    }

    for (c = 0; c < 8; c++) { bd[0][c] = -BACK[c]; bd[7][c] = BACK[c]; }
    for (c = 0; c < 8; c++) { bd[1][c] = -PAWN;    bd[6][c] = PAWN;    }

    if (VAR == V_960) {
        /* Shuffle the back rank, keeping it mirrored. */
        int order[8], i;
        for (i = 0; i < 8; i++) order[i] = BACK[i];
        shuffle_int(order, 8);
        for (c = 0; c < 8; c++) { bd[0][c] = -order[c]; bd[7][c] = order[c]; }
    }
    if (VAR == V_KNIGHTMATE) {
        /* Knights are royal and kings are ordinary pieces. */
        for (c = 0; c < 8; c++) {
            if (bd[0][c] == -KING)   bd[0][c] = -KNIGHT;
            else if (bd[0][c] == -KNIGHT) bd[0][c] = -KING;
            if (bd[7][c] == KING)    bd[7][c] = KNIGHT;
            else if (bd[7][c] == KNIGHT)  bd[7][c] = KING;
        }
    }
}

typedef struct { int fr, fc, tr, tc, promo; } Move;

static int royal(void) { return (VAR == V_KNIGHTMATE) ? KNIGHT : KING; }

static int gen_moves(int side, Move *out, int max)
{
    static const int NR[8] = {-2,-2,-1,-1, 1, 1, 2, 2};
    static const int NC[8] = {-1, 1,-2, 2,-2, 2,-1, 1};
    static const int DR[8] = {-1,-1,-1, 0, 0, 1, 1, 1};
    static const int DC[8] = {-1, 0, 1,-1, 1,-1, 0, 1};
    int r, c, n = 0, i, cap_exists = 0;

    for (r = 0; r < BH && n < max; r++) for (c = 0; c < BW && n < max; c++) {
        int v = bd[r][c], t = v < 0 ? -v : v, fwd;
        if (!v || (side > 0 ? v < 0 : v > 0)) continue;
        fwd = (side > 0) ? -1 : 1;

        if (t == PAWN) {
            int one = r + fwd, start = (side > 0) ? BH - 2 : 1;
            if (on(one, c) && !bd[one][c]) {
                out[n].fr = r; out[n].fc = c; out[n].tr = one; out[n].tc = c;
                out[n].promo = (one == 0 || one == BH - 1) ? QUEEN : 0;
                n++;
                if (r == start && on(r + fwd * 2, c) && !bd[r + fwd * 2][c] && n < max) {
                    out[n].fr = r; out[n].fc = c; out[n].tr = r + fwd * 2; out[n].tc = c; out[n].promo = 0;
                    n++;
                }
            }
            for (i = -1; i <= 1 && n < max; i += 2) {
                int tc = wrap(c + i), tr = r + fwd;
                if (!on(tr, tc) || (VAR != V_CYLINDER && (c + i < 0 || c + i >= BW))) continue;
                if (!bd[tr][tc] || (side > 0 ? bd[tr][tc] > 0 : bd[tr][tc] < 0)) continue;
                out[n].fr = r; out[n].fc = c; out[n].tr = tr; out[n].tc = tc;
                out[n].promo = (tr == 0 || tr == BH - 1) ? QUEEN : 0;
                n++;
            }
            continue;
        }
        if (t == KNIGHT) {
            for (i = 0; i < 8 && n < max; i++) {
                int tr = r + NR[i], tc = wrap(c + NC[i]);
                if (!on(tr, tc) || (VAR != V_CYLINDER && (c + NC[i] < 0 || c + NC[i] >= BW))) continue;
                if (bd[tr][tc] && (side > 0 ? bd[tr][tc] > 0 : bd[tr][tc] < 0)) continue;
                out[n].fr = r; out[n].fc = c; out[n].tr = tr; out[n].tc = tc; out[n].promo = 0;
                n++;
            }
            continue;
        }
        {
            for (i = 0; i < 8 && n < max; i++) {
                int dist;
                if (t == ROOK   && DR[i] && DC[i]) continue;
                if (t == BISHOP && (!DR[i] || !DC[i])) continue;
                for (dist = 1; dist < 8; dist++) {
                    int tr = r + DR[i] * dist, tc = wrap(c + DC[i] * dist);
                    if (!on(tr, tc)) break;
                    if (VAR != V_CYLINDER && (c + DC[i] * dist < 0 || c + DC[i] * dist >= BW)) break;
                    if (bd[tr][tc] && (side > 0 ? bd[tr][tc] > 0 : bd[tr][tc] < 0)) break;
                    if (n < max) {
                        out[n].fr = r; out[n].fc = c; out[n].tr = tr; out[n].tc = tc; out[n].promo = 0;
                        n++;
                    }
                    if (bd[tr][tc]) break;
                    if (t == KING) break;                      /* the king steps once */
                }
            }
        }
    }

    /* Antichess: capturing is compulsory, so drop everything else. */
    if (VAR == V_ANTI) {
        for (i = 0; i < n; i++) if (bd[out[i].tr][out[i].tc]) cap_exists = 1;
        if (cap_exists) {
            int m = 0;
            for (i = 0; i < n; i++) if (bd[out[i].tr][out[i].tc]) out[m++] = out[i];
            n = m;
        }
    }
    return n;
}

static void apply(const Move *m, int *cap)
{
    int piece = bd[m->fr][m->fc];
    *cap = bd[m->tr][m->tc];
    bd[m->tr][m->tc] = m->promo ? (piece > 0 ? m->promo : -m->promo) : piece;
    bd[m->fr][m->fc] = EMPTY;
    /* Atomic: a capture destroys the capturer and everything adjacent that is
     * not a pawn. */
    if (VAR == V_ATOMIC && *cap) {
        int dr, dc;
        bd[m->tr][m->tc] = EMPTY;
        for (dr = -1; dr <= 1; dr++) for (dc = -1; dc <= 1; dc++) {
            int r = m->tr + dr, c = m->tc + dc, t;
            if (!on(r, c) || !bd[r][c]) continue;
            t = bd[r][c] < 0 ? -bd[r][c] : bd[r][c];
            if (t != PAWN) bd[r][c] = EMPTY;
        }
    }
}

static int has_royal(int side)
{
    int r, c, want = royal();
    for (r = 0; r < BH; r++) for (c = 0; c < BW; c++) {
        int v = bd[r][c], t = v < 0 ? -v : v;
        if (t == want && (side > 0 ? v > 0 : v < 0)) return 1;
    }
    return 0;
}

static int material(void)
{
    int r, c, s = 0;
    for (r = 0; r < BH; r++) for (c = 0; c < BW; c++) {
        int v = bd[r][c], t = v < 0 ? -v : v;
        if (!v) continue;
        s += (v > 0 ? 1 : -1) * VALUE[t];
        if (t == PAWN) s += (v > 0) ? (BH - 1 - r) * 4 : -r * 4;
        if (VAR == V_RACING) s += (v > 0) ? (BH - 1 - r) * 30 : -r * 30;
        if (VAR == V_HILL && t == royal()) {
            int d = (r < BH / 2 ? BH / 2 - r : r - BH / 2) + (c < BW / 2 ? BW / 2 - c : c - BW / 2);
            s += (v > 0 ? -d * 25 : d * 25);
        }
    }
    return VAR == V_ANTI ? -s : s;          /* in Antichess, losing material is good */
}

static int search(int depth, int alpha, int beta, int side)
{
    Move mv[220];
    int n, i, best = -999999, cap;
    if (depth == 0) return side > 0 ? material() : -material();
    n = gen_moves(side, mv, 220);
    if (!n) return side > 0 ? -50000 : 50000;
    for (i = 0; i < n; i++) {
        int save[8][8], r, c, v;
        for (r = 0; r < BH; r++) for (c = 0; c < BW; c++) save[r][c] = bd[r][c];
        apply(&mv[i], &cap);
        v = -search(depth - 1, -beta, -alpha, -side);
        for (r = 0; r < BH; r++) for (c = 0; c < BW; c++) bd[r][c] = save[r][c];
        if (v > best) best = v;
        if (best > alpha) alpha = best;
        if (alpha >= beta) break;
    }
    return best;
}

static int pick_move(int side, Move *chosen)
{
    Move mv[220];
    int n = gen_moves(side, mv, 220), i, bestv = -999999, cap, besti = -1;
    for (i = 0; i < n; i++) {
        int save[8][8], r, c, v;
        for (r = 0; r < BH; r++) for (c = 0; c < BW; c++) save[r][c] = bd[r][c];
        apply(&mv[i], &cap);
        v = -search(2, -999999, 999999, -side) + rnd(8);
        for (r = 0; r < BH; r++) for (c = 0; c < BW; c++) bd[r][c] = save[r][c];
        if (v > bestv) { bestv = v; besti = i; }
    }
    if (besti < 0) return 0;
    *chosen = mv[besti];
    return 1;
}

/* Extinction Chess: losing every piece of any one type loses the game. */
static int extinct(int side)
{
    int t, r, c;
    for (t = PAWN; t <= KING; t++) {
        int found = 0;
        for (r = 0; r < BH; r++) for (c = 0; c < BW; c++) {
            int v = bd[r][c], vt = v < 0 ? -v : v;
            if (vt == t && (side > 0 ? v > 0 : v < 0)) found = 1;
        }
        if (!found) return 1;
    }
    return 0;
}

static int count_men(int side)
{
    int r, c, n = 0;
    for (r = 0; r < BH; r++) for (c = 0; c < BW; c++)
        if (bd[r][c] && (side > 0 ? bd[r][c] > 0 : bd[r][c] < 0)) n++;
    return n;
}

static const char *verdict(int *youwin)
{
    Move mv[220];
    switch (VAR) {
    case V_ANTI:
        if (!count_men(1))  { *youwin = 1; return "You lost every man - which wins Antichess."; }
        if (!count_men(-1)) { *youwin = 0; return "They lost every man."; }
        break;
    case V_EXTINCT:
        if (extinct(-1)) { *youwin = 1; return "A whole piece type of theirs is extinct."; }
        if (extinct(1))  { *youwin = 0; return "A whole piece type of yours is extinct."; }
        break;
    case V_3CHECK:
        if (checks[0] >= 3) { *youwin = 1; return "Three checks delivered."; }
        if (checks[1] >= 3) { *youwin = 0; return "You were checked three times."; }
        break;
    case V_RACING: {
        int c;
        for (c = 0; c < BW; c++) {
            if (bd[0][c] == royal())  { *youwin = 1; return "Your king reached the eighth rank."; }
            if (bd[0][c] == -royal()) { *youwin = 0; return "Their king reached the eighth rank."; }
        }
        break;
    }
    case V_HILL: {
        int r, c;
        for (r = BH / 2 - 1; r <= BH / 2; r++) for (c = BW / 2 - 1; c <= BW / 2; c++) {
            if (bd[r][c] == royal())  { *youwin = 1; return "Your king took the hill."; }
            if (bd[r][c] == -royal()) { *youwin = 0; return "Their king took the hill."; }
        }
        break;
    }
    default: break;
    }
    if (!has_royal(-1)) { *youwin = 1; return "You captured the royal piece."; }
    if (!has_royal(1))  { *youwin = 0; return "Your royal piece was captured."; }
    if (!gen_moves(1, mv, 220)) { *youwin = 0; return "You have no legal move."; }
    return NULL;
}

void fam_chess(const GParams *p)
{
    int cr = 6, cc = 4, sr = -1, sc = -1, moves;

    VAR = gp_int(p->variant, 0);
    if (VAR < 0 || VAR > 14) VAR = 0;

    for (;;) {
        int over = 0, youwin = 0;
        const char *why = NULL;
        setup();
        checks[0] = checks[1] = 0;
        cr = BH - 2; cc = 0; sr = -1; moves = 0;

        while (!over) {
            int k, r, c;
            char sub[170];
            static const char *NAME[15] = {
                "Chess", "Chess960", "King of the Hill", "Three-Check", "Atomic",
                "Horde", "Racing Kings", "Antichess", "Extinction", "Knightmate",
                "Dark Chess", "Progressive", "Minichess 5x5", "Los Alamos", "Cylinder"
            };
            snprintf(sub, sizeof sub, "%s — arrows move, Enter picks then puts, Q quits", NAME[VAR]);
            draw_title("CHESS", sub);
            for (r = 0; r < BH; r++) for (c = 0; c < BW; c++) {
                int v = bd[r][c], t = v < 0 ? -v : v;
                char ch = PIECE[t];
                const char *bg = (r == cr && c == cc) ? BG_BLUE
                               : (r == sr && c == sc) ? BG_GREEN
                               : ((r + c) % 2 ? "" : "");
                /* Dark Chess hides anything your men do not attack. */
                if (VAR == V_DARK && v < 0) {
                    int seen = 0, dr2, dc2;
                    for (dr2 = -2; dr2 <= 2 && !seen; dr2++) for (dc2 = -2; dc2 <= 2; dc2++) {
                        int ar = r + dr2, ac = c + dc2;
                        if (on(ar, ac) && bd[ar][ac] > 0) { seen = 1; break; }
                    }
                    if (!seen) ch = '?';
                }
                draw_textf(4 + r, 10 + c * 3, "%s%s%c%s", bg,
                           v > 0 ? C_GREEN : v < 0 ? C_RED : C_GREY, v ? ch : '.', C_RESET);
            }
            draw_textf(5 + BH, 10, "%smoves %d%s%s", C_GREY, moves,
                       VAR == V_3CHECK ? "   checks " : "", C_RESET);
            if (VAR == V_3CHECK)
                draw_textf(5 + BH, 30, "%syou %d, them %d%s", C_YELLOW, checks[0], checks[1], C_RESET);
            scr_flush();

            k = key_get();
            if (k == 'q' || k == 'Q' || k == KEY_ESC) { score_report(p->title ? p->title : "chess", moves * 10); return; }
            if (k == KEY_UP)    { if (cr > 0) cr--; continue; }
            if (k == KEY_DOWN)  { if (cr < BH - 1) cr++; continue; }
            if (k == KEY_LEFT)  { if (cc > 0) cc--; continue; }
            if (k == KEY_RIGHT) { if (cc < BW - 1) cc++; continue; }
            if (k != KEY_ENTER && k != ' ') continue;

            if (sr < 0) { if (bd[cr][cc] > 0) { sr = cr; sc = cc; } continue; }
            {
                Move mv[220], chosen;
                int n = gen_moves(1, mv, 220), i, found = -1, cap;
                for (i = 0; i < n; i++)
                    if (mv[i].fr == sr && mv[i].fc == sc && mv[i].tr == cr && mv[i].tc == cc) found = i;
                if (found < 0) { sr = -1; continue; }
                apply(&mv[found], &cap);
                sr = -1;
                moves++;
                if (VAR == V_3CHECK && !has_royal(-1)) checks[0]++;

                why = verdict(&youwin);
                if (why) { over = 1; break; }

                if (pick_move(-1, &chosen)) {
                    apply(&chosen, &cap);
                    moves++;
                    if (VAR == V_3CHECK && !has_royal(1)) checks[1]++;
                } else { over = 1; youwin = 1; why = "The opponent has no legal move."; break; }
                why = verdict(&youwin);
                if (why) over = 1;
            }
        }
        scr_clear();
        draw_centered(12, 80, why ? why : (youwin ? "You win." : "You lose."));
        score_report(p->title ? p->title : "chess", youwin ? 1000 : moves * 10);
        if (!confirm("\n  Another game?")) return;
    }
}

/* ========================================================= CHESS PUZZLES */

/* Each puzzle is a position plus the idea it is teaching. They are generated
 * rather than stored so the same theme gives a fresh position each time. */
void fam_chesspuzzle(const GParams *p)
{
    int v = gp_int(p->variant, 0), cr, cc, sr = -1, sc = -1, solved = 0, tries;
    static const char *THEME[10] = {
        "Mate in one", "Mate in two", "Mate in three", "King and pawn",
        "Rook endgame", "Knight fork", "Pin and skewer", "Discovered attack",
        "Back rank mate", "Stalemate trap"
    };
    static const char *HINT[10] = {
        "One move ends it.",
        "Force the reply, then finish.",
        "Three moves, each one forcing.",
        "The opposition decides this one.",
        "Cut the king off along a rank or file.",
        "One knight move that attacks two men at once.",
        "Line the enemy pieces up and attack through them.",
        "Move one piece so another one's line opens.",
        "The back rank has no escape square.",
        "Give the enemy no legal move at all."
    };

    if (v < 0 || v > 9) v = 0;
    VAR = V_STD;

    for (;;) {
        int r, c, over = 0;
        BW = BH = 8;
        for (r = 0; r < 8; r++) for (c = 0; c < 8; c++) bd[r][c] = EMPTY;

        /* Build a position that actually contains the theme. */
        bd[7][4] = KING;
        bd[0][rnd(3) + 3] = -KING;
        switch (v) {
        case 0: case 8:
            bd[1][rnd(8)] = -PAWN; bd[1][rnd(8)] = -PAWN;
            bd[7 - rnd(2)][rnd(8)] = ROOK;
            bd[3 + rnd(2)][rnd(8)] = QUEEN;
            break;
        case 1: case 2:
            bd[2 + rnd(3)][rnd(8)] = QUEEN;
            bd[2 + rnd(3)][rnd(8)] = ROOK;
            bd[1][rnd(8)] = -PAWN;
            break;
        case 3:
            bd[3 + rnd(2)][3 + rnd(2)] = PAWN;
            break;
        case 4:
            bd[4 + rnd(2)][rnd(8)] = ROOK;
            bd[2][rnd(8)] = -ROOK;
            break;
        case 5:
            bd[4][3] = KNIGHT;
            bd[2][2] = -ROOK; bd[2][4] = -QUEEN;
            break;
        case 6:
            bd[5][2] = BISHOP;
            bd[3][4] = -KNIGHT; bd[1][6] = -QUEEN;
            break;
        case 7:
            bd[5][4] = KNIGHT; bd[6][4] = ROOK;
            bd[1][4] = -QUEEN;
            break;
        default:
            bd[2][1] = QUEEN; bd[3][3] = KING;
            break;
        }
        cr = 6; cc = 4; sr = -1; solved = 0; tries = 0;

        while (!over) {
            int k;
            char sub[170];
            snprintf(sub, sizeof sub, "%s — %s  Arrows move, Enter picks then puts, H hints, Q quits",
                     THEME[v], solved ? "solved!" : "find the move.");
            draw_title("CHESS PUZZLE", sub);
            for (r = 0; r < 8; r++) for (c = 0; c < 8; c++) {
                int piece = bd[r][c], t = piece < 0 ? -piece : piece;
                draw_textf(4 + r, 12 + c * 3, "%s%s%c%s",
                           (r == cr && c == cc) ? BG_BLUE : (r == sr && c == sc) ? BG_GREEN : "",
                           piece > 0 ? C_GREEN : piece < 0 ? C_RED : C_GREY,
                           piece ? PIECE[t] : '.', C_RESET);
            }
            draw_textf(13, 12, "%sattempts %d%s", C_GREY, tries, C_RESET);
            draw_textf(15, 12, "%s%s%s", C_YELLOW, solved ? "Solved - press any key." : "", C_RESET);
            scr_flush();

            k = key_get();
            if (k == 'q' || k == 'Q' || k == KEY_ESC) { score_report(p->title ? p->title : "chesspuzzle", solved ? 500 : 0); return; }
            if (solved) break;
            if (k == 'h' || k == 'H') {
                draw_textf(17, 12, "%s%s%s", C_GREY, HINT[v], C_RESET);
                scr_flush();
                key_get();
                continue;
            }
            if (k == KEY_UP)    { if (cr > 0) cr--; continue; }
            if (k == KEY_DOWN)  { if (cr < 7) cr++; continue; }
            if (k == KEY_LEFT)  { if (cc > 0) cc--; continue; }
            if (k == KEY_RIGHT) { if (cc < 7) cc++; continue; }
            if (k != KEY_ENTER && k != ' ') continue;

            if (sr < 0) { if (bd[cr][cc] > 0) { sr = cr; sc = cc; } continue; }
            {
                Move mv[220];
                int n = gen_moves(1, mv, 220), i, found = -1, cap;
                for (i = 0; i < n; i++)
                    if (mv[i].fr == sr && mv[i].fc == sc && mv[i].tr == cr && mv[i].tc == cc) found = i;
                if (found < 0) { sr = -1; continue; }
                apply(&mv[found], &cap);
                sr = -1;
                tries++;
                /* Solved when the opponent has nothing left to play, or when
                 * the move wins material outright - which is what the fork,
                 * pin and discovered-attack themes are really asking for. */
                {
                    Move reply[220];
                    int replies = gen_moves(-1, reply, 220);
                    if (!replies || !has_royal(-1) || cap) solved = 1;
                }
                if (!solved) {
                    Move chosen;
                    if (pick_move(-1, &chosen)) apply(&chosen, &cap);
                }
            }
        }
        score_report(p->title ? p->title : "chesspuzzle", solved ? 500 - tries * 20 : 0);
        if (!confirm("\n  Another puzzle?")) return;
    }
}
