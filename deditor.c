/*** includes ***/

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <string.h>

/*** preprocessor definitions ***/

#define CTRL_KEY(k) ((k) & 0x1f)
#define ABUF_INIT { NULL, 0 }
#define DEDITOR_VERSION  "0.0.1"

/*** data ***/

// Stores data from original terminal
struct editorConfig {
  struct termios orig_termios;
  int screenRows;
  int screenCols;
};

struct editorConfig E;

// stores buffer data
struct abuf {
  char * b;
  int len;
};

/*** function prototypes ***/

// Enables raw mode 
void enableRawMode(void);

// Disables raw mode
void disableRawMode(void);

// Prints error message and exits program
void die(const char *);

// Low-level keypressing
char editorReadKey(void);

// Mapping keypresses to editor operations
void editorProcessKeypress(void);

// Clear screen and reposition cursor
void editorRefreshScreen(void);

// draw rows with tildes
void editorDrawRows(struct abuf * ab);

// gets size of terminal window
int getWindowSize(int *, int *);

// initializes all fields in E struct
void initEditor(void);

// gets the position of the cursor
int getCursorPosition(int *, int *);

// append string to buffer
void abAppend(struct abuf * ab, const char * s, int len);

// destructor for ab
void abFree(struct abuf * ab);

/*** terminal ***/

void enableRawMode(void) {
  if (tcgetattr(STDIN_FILENO, &E.orig_termios) == -1) die("tcgetattr");
  atexit(disableRawMode);

  struct termios raw = E.orig_termios;
  raw.c_iflag &= ~(IXON | ICRNL | INPCK | BRKINT | ISTRIP);
  raw.c_oflag &= ~(OPOST);
  raw.c_cflag |= (CS8);
  raw.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);
  raw.c_cc[VMIN] = 0;
  raw.c_cc[VTIME] = 1;

  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) die("tcsetattr");
} // enableRawMode

void disableRawMode(void) {
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1) {
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

int getWindowSize(int * rows, int * cols) {
  struct winsize ws;

  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
    if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) return -1;
    return getCursorPosition(rows, cols);
  } else {
    *cols = ws.ws_col;
    *rows = ws.ws_row;
    return 0;
  } // if
} // getWindowSize

int getCursorPosition(int * rows, int * cols) {
  char buf[32];
  unsigned int i = 0;
  
  if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4) return -1;

  while (i < sizeof(buf) - 1) {
    if (read(STDIN_FILENO, &buf[i], 1) != 1) break;
    if (buf[i] == 'R') break;
    i++;
  } // while
  buf[i] = '\0';

  if (buf[0] != '\x1b' || buf[1] != '[') return -1;
  if (sscanf(&buf[2], "%d;%d", rows, cols) != 2) return -1;

  return 0;
} // getCursorPosition

/*** append buffer ***/

void abAppend(struct abuf *ab, const char *s, int len) {
  char *new = realloc(ab->b, ab->len + len);
  if (new == NULL) return;
  memcpy(&new[ab->len], s, len);
  ab->b = new;
  ab->len += len;
}

void abFree(struct abuf *ab) {
  free(ab->b);
}

/*** output ***/

void editorRefreshScreen(void) {
  struct abuf ab = ABUF_INIT;

  abAppend(&ab, "\x1b[?25l", 6);
  abAppend(&ab, "\x1b[H", 3);

  editorDrawRows(&ab);

  abAppend(&ab, "\x1b[H", 3);
  abAppend(&ab, "\x1b[?25H", 6);

  write(STDOUT_FILENO, ab.b, ab.len);
  abFree(&ab);
} // editorRefreshScreen

void editorDrawRows(struct abuf * ab) {
  int y;
  for (y = 0; y < E.screenRows; y++) {
    if (y == E.screenRows / 3) {
      char welcome[80];
      int welcomelen = snprintf(welcome, sizeof(welcome),
				"Deditor -- version %s", DEDITOR_VERSION);
      if (welcomelen > E.screenCols) welcomelen = E.screenCols;
      int padding = (E.screenCols - welcomelen) / 2;
      if (padding) {
	abAppend(ab, "~", 1);
	padding--;
      } // if
      while (padding--) abAppend(ab, " ", 1);
      abAppend(ab, welcome, welcomelen);
    } else {
      abAppend(ab, "~", 1);
    } // if

    abAppend(ab, "\x1b[K", 3);
    if (y < E.screenRows - 1) {
      abAppend(ab, "\r\n", 2);
    } // if
  } // for
} // editorDrawRows

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

void initEditor(void) {
  if (getWindowSize(&E.screenRows, &E.screenCols) == -1) die("getWindowSize");
} // initEditor

int main(void) {
  enableRawMode();
  initEditor();
  
  while (1) {
    editorRefreshScreen();
    editorProcessKeypress();
  } // while
  return 0;
} // main
