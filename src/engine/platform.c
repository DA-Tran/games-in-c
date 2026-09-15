/* platform.c - terminal raw mode, keyboard input and timing.
 * POSIX path uses termios + select(); Windows path uses conio.
 */
#ifndef _WIN32
/* Ask glibc for clock_gettime, nanosleep and the ioctl winsize struct
 * even when the compiler is in strict -std=c99 mode. */
#  define _POSIX_C_SOURCE 200809L
#  define _DEFAULT_SOURCE 1
#endif

#include "engine.h"

#ifdef _WIN32
#  include <windows.h>
#  include <conio.h>
#else
#  include <termios.h>
#  include <unistd.h>
#  include <sys/select.h>
#  include <sys/ioctl.h>
#endif

static int g_raw_active = 0;

#ifndef _WIN32
static struct termios g_orig_termios;
#endif

/* ------------------------------------------------------------------ mode */

void scr_init(void)
{
#ifdef _WIN32
    /* Ask the console host for ANSI escape processing (Win10+). */
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (GetConsoleMode(h, &mode))
        SetConsoleMode(h, mode | 0x0004 /* ENABLE_VIRTUAL_TERMINAL_PROCESSING */);
    g_raw_active = 1;
#else
    struct termios raw;
    if (!isatty(STDIN_FILENO)) { g_raw_active = 0; return; }
    if (tcgetattr(STDIN_FILENO, &g_orig_termios) == -1) { g_raw_active = 0; return; }
    raw = g_orig_termios;
    /* Disable canonical mode and echo; keep signals so Ctrl-C still works. */
    raw.c_lflag &= ~(unsigned)(ECHO | ICANON);
    raw.c_iflag &= ~(unsigned)(IXON | ICRNL);
    raw.c_cc[VMIN]  = 1;
    raw.c_cc[VTIME] = 0;
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) { g_raw_active = 0; return; }
    g_raw_active = 1;
#endif
    scr_hide_cursor();
}

void scr_shutdown(void)
{
    scr_show_cursor();
    printf(C_RESET);
    scr_flush();
#ifndef _WIN32
    if (g_raw_active)
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &g_orig_termios);
#endif
    g_raw_active = 0;
}

/* Temporarily restore cooked mode so scanf/fgets behave normally. */
static void mode_cooked(void)
{
#ifndef _WIN32
    if (g_raw_active)
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &g_orig_termios);
#endif
}

static void mode_raw(void)
{
#ifndef _WIN32
    if (g_raw_active) {
        struct termios raw = g_orig_termios;
        raw.c_lflag &= ~(unsigned)(ECHO | ICANON);
        raw.c_iflag &= ~(unsigned)(IXON | ICRNL);
        raw.c_cc[VMIN]  = 1;
        raw.c_cc[VTIME] = 0;
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
    }
#endif
}

/* ----------------------------------------------------------------- input */

#ifndef _WIN32
#define BYTE_NONE (-1)
#define BYTE_EOF  (-2)

/* Read one byte. Returns BYTE_NONE if nothing arrives within timeout_ms,
 * or BYTE_EOF once the input stream is exhausted. Works on both a tty and
 * a pipe, so the game loops stay responsive when input is scripted. */
static int read_byte_timeout(int timeout_ms)
{
    fd_set set;
    struct timeval tv;
    unsigned char c;
    int r;

    FD_ZERO(&set);
    FD_SET(STDIN_FILENO, &set);
    tv.tv_sec  = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;

    r = select(STDIN_FILENO + 1, &set, NULL, NULL, &tv);
    if (r <= 0) return BYTE_NONE;
    r = (int)read(STDIN_FILENO, &c, 1);
    if (r == 0)  return BYTE_EOF;      /* stdin closed */
    if (r != 1)  return BYTE_NONE;
    return (int)c;
}
#endif

/* Decode a CSI sequence that has already consumed ESC. */
static int decode_escape(int (*next)(int), int timeout_ms)
{
    int a = next(timeout_ms);
    if (a < 0) return KEY_ESC;               /* lone ESC */
    if (a != '[' && a != 'O') return KEY_ESC;

    {
        int b = next(timeout_ms);
        if (b < 0) return KEY_ESC;
        switch (b) {
            case 'A': return KEY_UP;
            case 'B': return KEY_DOWN;
            case 'C': return KEY_RIGHT;
            case 'D': return KEY_LEFT;
            case 'H': return KEY_HOME;
            case 'F': return KEY_END;
            default: break;
        }
        if (b >= '0' && b <= '9') {
            int c = next(timeout_ms);          /* expect '~' */
            (void)c;
            switch (b) {
                case '1': return KEY_HOME;
                case '3': return KEY_DEL;
                case '4': return KEY_END;
                case '5': return KEY_PGUP;
                case '6': return KEY_PGDN;
                default: break;
            }
        }
    }
    return KEY_ESC;
}

#ifndef _WIN32
static int next_blocking(int timeout_ms)  { return read_byte_timeout(timeout_ms); }
#endif

int key_get(void)
{
#ifdef _WIN32
    int c = _getch();
    if (c == 0 || c == 224) {                 /* extended scan code */
        int d = _getch();
        switch (d) {
            case 72: return KEY_UP;
            case 80: return KEY_DOWN;
            case 75: return KEY_LEFT;
            case 77: return KEY_RIGHT;
            case 71: return KEY_HOME;
            case 79: return KEY_END;
            case 73: return KEY_PGUP;
            case 81: return KEY_PGDN;
            case 83: return KEY_DEL;
            default: return KEY_NONE;
        }
    }
    if (c == '\r') return KEY_ENTER;
    if (c == 8)    return KEY_BACKSPACE;
    return c;
#else
    int c;
    /* Block until something arrives; EOF is treated as a quit so scripted
     * runs terminate instead of spinning forever. */
    for (;;) {
        c = read_byte_timeout(1000);
        if (c == BYTE_EOF) return KEY_ESC;
        if (c != BYTE_NONE) break;
    }
    if (c == 27) return decode_escape(next_blocking, 30);
    if (c == '\r') return KEY_ENTER;
    return c;
#endif
}

int key_poll(void)
{
#ifdef _WIN32
    if (!_kbhit()) return KEY_NONE;
    return key_get();
#else
    int c = read_byte_timeout(0);
    if (c == BYTE_EOF)  return KEY_ESC;   /* stream ended - let games exit */
    if (c == BYTE_NONE) return KEY_NONE;
    if (c == 27) return decode_escape(next_blocking, 20);
    if (c == '\r') return KEY_ENTER;
    return c;
#endif
}

void key_flush(void)
{
#ifdef _WIN32
    while (_kbhit()) (void)_getch();
#else
    while (read_byte_timeout(0) >= 0) { /* drain */ }
#endif
}

/* Cooked line entry: echo characters, handle backspace, stop at Enter. */
/* Returns the number of characters read, or -1 if the entry was abandoned -
 * Escape pressed, or the input stream ended. Callers that retry on an empty
 * line must check for -1, or they will spin forever once stdin is exhausted.
 * That is exactly what the anagram games did. */
int read_line(char *buf, int max)
{
    int n = 0, abandoned = 0;
    scr_show_cursor();
    for (;;) {
        int c = key_get();
        if (c == KEY_ENTER || c == '\n') break;
        if (c == KEY_ESC) { n = 0; abandoned = 1; break; }
        if (c == KEY_BACKSPACE || c == 8) {
            if (n > 0) { n--; printf("\b \b"); scr_flush(); }
            continue;
        }
        if (c >= 32 && c < 127 && n < max - 1) {
            buf[n++] = (char)c;
            putchar(c);
            scr_flush();
        }
    }
    buf[n] = '\0';
    scr_hide_cursor();
    return abandoned ? -1 : n;
}

int read_int(const char *prompt, int lo, int hi)
{
    char buf[32];
    for (;;) {
        printf("\r%s%s%s ", C_CYAN, prompt, C_RESET);
        scr_flush();
        if (read_line(buf, sizeof buf) == 0) return lo - 1;   /* cancelled */
        {
            char *end;
            long v = strtol(buf, &end, 10);
            if (end != buf && v >= lo && v <= hi) return (int)v;
        }
        printf("  %sEnter a number between %d and %d.%s", C_RED, lo, hi, C_RESET);
        scr_flush();
        sleep_ms(700);
        printf("\r\033[2K");
    }
}

int confirm(const char *prompt)
{
    printf("\r%s%s%s (y/n) ", C_YELLOW, prompt, C_RESET);
    scr_flush();
    for (;;) {
        int c = tolower(key_get());
        if (c == 'y') return 1;
        if (c == 'n' || c == KEY_ESC) return 0;
    }
}

void pause_msg(const char *msg)
{
    int rows, cols;
    scr_size(&rows, &cols);
    scr_move(rows, 1);
    printf("\033[2K%s%s%s", C_GREY, msg ? msg : "Press any key to continue...", C_RESET);
    scr_flush();
    key_flush();
    key_get();
}

/* ---------------------------------------------------------------- timing */

/* Animation pauses are the point of a reveal, but they make a game untestable
 * past a few dozen turns: a race game spends most of a second per round, so a
 * few hundred scripted keystrokes ask for half an hour of real time and then
 * look like a hang. GIC_NODELAY turns every pause into a no-op so the test
 * harness can drive a game thousands of turns deep. It changes nothing for a
 * player, and the timing paths themselves are still exercised by the ordinary
 * run, which leaves it unset. */
static int delays_disabled(void)
{
    static int checked, off;
    if (!checked) {
        const char *e = getenv("GIC_NODELAY");
        off = (e && *e && *e != '0');
        checked = 1;
    }
    return off;
}

void sleep_ms(int ms)
{
    if (ms <= 0 || delays_disabled()) return;
#ifdef _WIN32
    Sleep((DWORD)ms);
#else
    struct timespec ts;
    ts.tv_sec  = ms / 1000;
    ts.tv_nsec = (long)(ms % 1000) * 1000000L;
    nanosleep(&ts, NULL);
#endif
}

void beep(int freq_hz, int ms)
{
#ifdef _WIN32
    if (freq_hz < 37)    freq_hz = 37;      /* Beep() rejects anything lower */
    if (freq_hz > 32767) freq_hz = 32767;
    Beep((DWORD)freq_hz, (DWORD)ms);
#else
    /* No portable tone generator on a POSIX terminal; use the bell. */
    (void)freq_hz; (void)ms;
    fputc('\a', stdout);
    fflush(stdout);
#endif
}

long now_ms(void)
{
#ifdef _WIN32
    return (long)GetTickCount();
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long)(ts.tv_sec * 1000L + ts.tv_nsec / 1000000L);
#endif
}

void scr_size(int *rows, int *cols)
{
    *rows = 24; *cols = 80;
#ifdef _WIN32
    {
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi)) {
            *cols = csbi.srWindow.Right - csbi.srWindow.Left + 1;
            *rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
        }
    }
#else
    {
        struct winsize ws;
        if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0) {
            *cols = ws.ws_col;
            *rows = ws.ws_row;
        }
    }
#endif
    if (*cols < 40) *cols = 40;
    if (*rows < 10) *rows = 10;
}
