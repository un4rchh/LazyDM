#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include "term.h"
#include "config.h"

#define FILL_SIZE   1024

static void term_bg_color(int fd, int bg);

void term_clear(int fd){
  write(fd, "\033[2J\033[H", 7);
}

void term_goto(int fd, int y, int x){
  char buf[BUF_SIZE];
  snprintf(buf, sizeof(buf), "\033[%d;%dH", y, x);
  write(fd, buf, strlen(buf));
}

void term_fg_color(int fd, int fg){
  if (!((fg >= 30 && fg <= 37) || (fg >= 90 && fg <= 97))) {
        return;
    }
    char buf[BUF_SIZE];
    snprintf(buf, sizeof(buf), "\033[%dm", fg);
    write(fd, buf, strlen(buf));
}

void term_bg_color(int fd, int bg) {
    if (!((bg >= 40 && bg <= 47) || (bg >= 100 && bg <= 107))) {
        return;
    }
    char buf[BUF_SIZE];
    snprintf(buf, sizeof(buf), "\033[%dm", bg); 
    write(fd, buf, strlen(buf));
}

void term_fill_bg(int fd, int color) {
    struct winsize ws;
    if (ioctl(fd, TIOCGWINSZ, &ws) == -1) {
        ws.ws_col = 80;
        ws.ws_row = 24;
    }
    
    int total_chars = ws.ws_col * ws.ws_row;
    
    term_bg_color(fd, color);
    
    char spaces[FILL_SIZE];
    memset(spaces, ' ', FILL_SIZE);
    
    int remaining = total_chars;
    while (remaining > 0) {
        int to_write = (remaining > FILL_SIZE) ? FILL_SIZE : remaining;
        write(fd, spaces, to_write);
        remaining -= to_write;
    }
    
    term_goto(fd, 1, 1);
}

void term_reset(int fd){
  write(fd, "\033[0m", 4);
}

void term_box(int fd, int x, int y, int w, int h){
  term_goto(fd, y, x);
  write(fd, "╔", 3);
  for (int i = 0; i < w - 2; i++){
    write(fd, "═", 3);
  }
  write(fd, "╗", 3);

  for (int i = 1; i < h - 1; i++){
    term_goto(fd, y+i, x);
    write(fd, "║", 3);
    term_goto(fd, y + i, x + w - 1);
    write(fd, "║", 3);
  }

    term_goto(fd, y + h - 1, x);
    write(fd, "╚", 3);
    for (int i = 0; i < w - 2; i++)
        write(fd, "═", 3);
    write(fd, "╝", 3);
}

void draw_err(int fd, int by, int bh, int bx, int bw, const char *msg){
  write(fd, "\033[?25l", 6);
  term_goto(fd, by + bh + 2, bx + (bw - strlen(msg)) / 2);
  write(fd, msg, strlen(msg));
  sleep(SLEEP_TIME);
  write(fd, "\033[?25h", 6);
}
