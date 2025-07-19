#include <asm-generic/errno-base.h>
#include <asm-generic/ioctls.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#define ctrl(k) ((k) & 0x1f)

// info
struct editorconfig {
  int screenRow;
  int screenCol;
  struct termios orig_termios;
};
struct editorconfig E;

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
  raw.c_cflag |= ~(CS8);
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
  return c;
}

// output fn
void editordrawrows(void) {
  int y;
  for (y = 0; y < E.screenRow; y++) {
    write(STDOUT_FILENO, "~\r\n", 3);
  }
}

void refreshScreen(void) {

  write(STDOUT_FILENO, "\x1b[2J", 4);
  write(STDOUT_FILENO, "\x1b[H", 3);
  editordrawrows();
  write(STDOUT_FILENO, "\x1b[H", 3);
}

// input fn

void editorprocesskeys(void) {
  char c = editorRead();
  switch (c) {
  case ctrl('q'):
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);
    exit(0);
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
  printf("\r\n&buf[i]: '%s'\r\n", &buf[1]);

  editorRead();
  return -1;
}
int getTermSize(int *r, int *c) {
  struct winsize ws;
  if (1 || ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
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
