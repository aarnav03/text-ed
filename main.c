#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

struct termios orig_termios;

void disable_raw(void) { tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios); }

void enable_raw(void) {
  tcgetattr(STDIN_FILENO, &orig_termios);
  atexit(disable_raw);

  struct termios raw = orig_termios;
  // tcgetattr(STDIN_FILENO, &raw);

  raw.c_iflag &= ~(IXON | ICRNL | BRKINT | INPCK | ISTRIP);
  raw.c_cflag |= ~(CS8);
  raw.c_oflag &= ~(OPOST);
  raw.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}
int main(void) {
  enable_raw();
  char c;
  while (read(STDIN_FILENO, &c, 1) == 1 && c != 'q') {
    if (iscntrl(c)) {
      printf("%d ctrl\r\n", c);
    } else {
      printf("%d -> ascii (%c) \r\n", c, c);
    }
  }
  return 0;
}
