/*** includes ***/

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>

/*** preprocessor definitions ***/

#define CTRL_KEY(k) ((k) & 0x1f)

/*** function prototypes ***/

// Enables raw mode 
void enableRawMode(void);

// Disables raw mode
void disableRawMode(void);

// Prints error message and exits program
void die(const char *s);

// Low-level keypressing
char editorReadKey(void);

// Mapping keypresses to editor operations
void editorProcessKeypress(void);

// Clear screen and reposition cursor
void editorRefreshScreen(void);

/*** data ***/

// Stores data from original terminal
struct termios orig_termios;

/*** terminal ***/

void enableRawMode(void) {
  if (tcgetattr(STDIN_FILENO, &orig_termios) == -1) die("tcgetattr");
  atexit(disableRawMode);

  struct termios raw = orig_termios;
  raw.c_iflag &= ~(IXON | ICRNL | INPCK | BRKINT | ISTRIP);
  raw.c_oflag &= ~(OPOST);
  raw.c_cflag |= (CS8);
  raw.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);
  raw.c_cc[VMIN] = 0;
  raw.c_cc[VTIME] = 1;

  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) die("tcsetattr");
} // enableRawMode

void disableRawMode(void) {
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios) == -1) {
    die("tcsetattr");
  } // if
} // disableRawMode

void die(const char *s) {
  editorRefreshScreen();
  
  perror(s);
  exit(1);
} // die

char editorReadKey(void) {
  int nread;
  char c;
  while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
    if (nread == -1 && errno != EAGAIN) die("read");
  } // while
  return c;
} // editorReadKey

/*** output ***/

void editorRefreshScreen(void) {
  write(STDOUT_FILENO, "\x1b[2J", 4);
  write(STDOUT_FILENO, "\x1b[H", 3);
} // editorRefreshScreen

/** input ***/

void editorProcessKeypress(void) {
  char c = editorReadKey();

  switch(c) {
  case CTRL_KEY('q'):
    exit(0);
    break;
  } // case
} // editorProcessKeypress

/*** init ***/

int main(void) {
  enableRawMode();
  
  while (1) {
    editorRefreshScreen();
    editorProcessKeypress();
  } // while
  return 0;
} // main
