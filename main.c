#include <asm-generic/errno-base.h>
#include <asm-generic/ioctls.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#define ctrl(k) ((k) & 0x1f)
#define version "0.0"

enum navKeys {
  left = 1000,
  right,
  up,
  down,
  pg_up,
  pg_down,
};

// info
struct editorconfig {
  int curX, curY;
  int screenRow;
  int screenCol;
  struct termios orig_termios;
};
struct editorconfig E;

// buffer to write at once
struct appendBuf {
  char *c;
  int len;
};
#define appendBuf_init {NULL, 0} /* empty */

void abAppend(struct appendBuf *ab, const char *s, int len) {
  char *new = realloc(ab->c, ab->len + len);
  if (new == NULL)
    return;
  memcpy(&new[ab->len], s, len);
  ab->c = new;
  ab->len += len;
}

void abFree(struct appendBuf *ab) { free(ab->c); }
// functions

void die(const char *s);
void disable_raw(void);
void enable_raw(void);
char editorRead(void);
void editordrawrows(struct appendBuf *ab);
void refreshScreen(void);
void editorprocesskeys(void);
void editorCursorMove(int key);
int getCursorPos(int *row, int *col);
int getTermSize(int *r, int *c);
void initeditor(void);

void die(const char *s) {
  write(STDOUT_FILENO, "\x1b[2J", 4);
  write(STDOUT_FILENO, "\x1b[H", 3);
  perror(s);
  exit(1);
}

// terminal stuff

void disable_raw(void) {
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1)
    die("tcsetattr");
}

void enable_raw(void) {
  if (tcgetattr(STDIN_FILENO, &E.orig_termios) == -1)
    die("tcgetattr");
  atexit(disable_raw);

  struct termios raw = E.orig_termios;
  // tcgetattr(STDIN_FILENO, &raw);

  raw.c_iflag &= ~(IXON | ICRNL | BRKINT | INPCK | ISTRIP);
  raw.c_cflag |= CS8;
  // raw.c_cflag &= ~CSIZE;
  raw.c_oflag &= ~(OPOST);
  raw.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);
  raw.c_cc[VMIN] = 0;
  raw.c_cc[VTIME] = 1;

  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1)
    die("tcsetattr");
}

char editorRead(void) {
  int n;
  char c;
  while ((n = read(STDIN_FILENO, &c, 1)) != 1) {
    if (n == -1 && errno != EAGAIN)
      die("read");
  }

  if (c == '\x1b') {
    char seq[3];
    if (read(STDIN_FILENO, &seq[0], 1) != 1)
      return '\x1b';
    if (read(STDIN_FILENO, &seq[1], 1) != 1)
      return '\x1b';

    if (seq[0] == '[') {
      if (seq[1] >= '0' && seq[1] <= '9') {
        if (read(STDIN_FILENO, &seq[2], 1) != 1)
          return '\x1b';
        if (seq[2] == '~') {
          switch (seq[1]) {
          case '5':
            pg_up;
          case '6':
            pg_down;
          }
        }
      } else {
        switch (seq[1]) {
        case 'A':
          return up;
        case 'B':
          return down;
        case 'C':
          return right;
        case 'D':
          return left;
        }
      }
    }

    return '\x1b';

  } else {
    return c;
  }
}

// output fn
void editordrawrows(struct appendBuf *ab) {
  int y;
  for (y = 0; y < E.screenRow; y++) {
    if (y == E.screenRow / 3) {
      char welcome[80];
      int welcomeLen = snprintf(welcome, sizeof(welcome),
                                "Arnv text editor --version %s", version);
      if (welcomeLen > E.screenCol)
        welcomeLen = E.screenCol;
      int padding = (E.screenCol - welcomeLen) / 2;
      if (padding) {
        abAppend(ab, "~", 1);
        padding--;
      }
      while (padding--) {
        abAppend(ab, " ", 1);
      }
      abAppend(ab, welcome, welcomeLen);
    }

    else {
      abAppend(ab, "~", 1);
    }

    abAppend(ab, "\x1b[K", 3);
    if (y < E.screenRow - 1) {
      abAppend(ab, "\r\n", 2);
    }
  }
}

void refreshScreen(void) {
  struct appendBuf ab = appendBuf_init;

  abAppend(&ab, "\x1b[?25l", 6);
  // abAppend(&ab, "\x1b[2J", 4);
  abAppend(&ab, "\x1b[H", 3);

  editordrawrows(&ab);

  char buf[32];
  snprintf(buf, sizeof(buf), "\x1b[%d;%dH", E.curY + 1, E.curX + 1);
  abAppend(&ab, buf, strlen(buf));

  abAppend(&ab, "\x1b[H", 3);
  abAppend(&ab, "\x1b[?25h", 6);

  write(STDOUT_FILENO, ab.c, ab.len);
  abFree(&ab);
}

// input fn

void editorprocesskeys(void) {
  int c = editorRead();
  switch (c) {
  case ctrl('q'):
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);
    exit(0);
    break;

  case pg_up:
  case pg_down: {
    int l = E.screenRow;
    while (l--) {
      editorCursorMove(c == pg_up ? up : down);
    }
  }

  case up:
  case down:
  case left:
  case right:
    editorCursorMove(c);
    break;
  }
}

void editorCursorMove(int key) {
  switch (key) {

  case left:
    if (E.curX != 0) {
      E.curX--;
    }
    break;
  case right:
    if (E.curX != E.screenCol - 1) {
      E.curX++;
    }
    break;

  case up:
    if (E.curY != 0) {
      E.curY--;
    }
    break;

  case down:
    if (E.curY != E.screenRow - 1) {
      E.curY++;
    }
    break;
  }
}

// initialising the editor
int getCursorPos(int *row, int *col) {
  char buf[32];
  unsigned int i = 0;

  if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4)
    return -1;

  while (i < sizeof(buf) - 1) {
    if (read(STDIN_FILENO, &buf[i], 1) != 1)
      break;
    if (buf[i] == 'R')
      break;
    i++;
  }
  buf[i] = '\0';
  if (buf[0] != '\x1b' || buf[1] != '[')
    return -1;
  if (sscanf(&buf[2], "%d;%d", row, col) != 2)
    return -1;
  return 0;
}
int getTermSize(int *r, int *c) {
  struct winsize ws;
  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
    if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12)
      return -1;
    return getCursorPos(r, c);
  } else {
    *c = ws.ws_col;
    *r = ws.ws_row;
    return 0;
  }
}

void initeditor(void) {
  E.curX = 0;
  E.curY = 0;
  if (getTermSize(&E.screenRow, &E.screenCol) == -1)
    die("getTermSize");
}

// main fn

int main(void) {
  enable_raw();
  initeditor();
  while (1) {
    refreshScreen();
    editorprocesskeys();
  }
  return 0;
}
