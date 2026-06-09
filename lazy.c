#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <pwd.h>
#include <grp.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <linux/kd.h>
#include <errno.h>
#include <ctype.h>
#include <strings.h>

#include "utils.h"
#include "term.h"
#include "auth.h"
#include "session.h"
#include "desktop.h"
#include "config.h"

static volatile sig_atomic_t running = 1;

static void compute_layout(int fd, int bw, int bh, int *bx, int *by, int *term_x, int *term_y);
static void signal_handler(int sig);
static bool init_tty(Config *lz);
static void draw_ui(int fd, int bw, int bh, int *bx, int *by, int *term_x, int *term_y);
static bool get_user_input(Config *lz, int bx, int by);
static bool authenticate(Config *lz, int bx, int by, int bw, int bh);
static bool ses_init(Config *lz, Session *sessions, int *xcount, int *wcount, int bx, int by, int bh, int bw, int *num);
static bool launch_session(Config *lz, Session *sessions, Start *s, pid_t pid, int by, int bh, int bx, int bw, int num, int xcount, int wcount);
static void clean(Start *s, Session *sessions);

void compute_layout(int fd, int bw, int bh, int *bx, int *by, int *term_x, int *term_y){
  struct winsize ws;
  ioctl(fd, TIOCGWINSZ, &ws);
  *term_x = ws.ws_col;
  *term_y = ws.ws_row; 

  *bx = (*term_x - bw)/2;
  *by = ((*term_y - bh)*2)/3;
}

void signal_handler(int sig){
  if (sig == SIGINT){
    return;
  }
  running = 0;
}

bool init_tty(Config *lz){
  if (isatty(STDIN_FILENO)){
    const char *curr = ttyname(STDIN_FILENO);
    if (curr){
      snprintf(lz->tty_path, sizeof(lz->tty_path), "%s", curr);
    } else {
      snprintf(lz->tty_path, sizeof(lz->tty_path), "/dev/tty");
    }
  } else {
    snprintf(lz->tty_path, sizeof(lz->tty_path), "/dev/tty%d", tty_number);
    }
    lz->fd = open(lz->tty_path, O_RDWR);  
    if (lz->fd < 0){ return false; }

  struct sigaction sa;
  sa.sa_handler = signal_handler;
  sa.sa_flags = 0;
  sigemptyset(&sa.sa_mask);
  sigaction(SIGTERM, &sa, NULL);
  sigaction(SIGINT, &sa, NULL);
  sigaction(SIGHUP, &sa, NULL);

  if (tcgetattr(lz->fd, &lz->old_term) == -1){
    close(lz->fd);
    free(lz);
    return false;
  }
  return true;
}

void draw_ui(int fd, int bw, int bh, int *bx, int *by, int *term_x, int *term_y){
  term_clear(fd);
  term_fill_bg(fd, bg);
  term_fg_color(fd, fg); 
  compute_layout(fd, bw, bh, bx, by, term_x, term_y);
#if MOTD_SH == 0  
  draw_motd(fd, motd_path, *by, *bx, bw);
#endif
  term_box(fd, *bx, *by, bw, bh);

  int len = strlen("LazyDM");
  int x = *bx + (bw - len) / 2;
  term_goto(fd, *by, x);
  write(fd, "LazyDM", len); 
}

bool get_user_input(Config *lz, int bx, int by){

#if AUTOLOGIN == 0
    char login[BUF_SIZE];
    char hostname[BUF_SIZE];
    if (gethostname(hostname, sizeof(hostname)) == 0){
      snprintf(login, sizeof(login), "%.200s login: %.30s",hostname, default_user);
    }
    term_goto(lz->fd, by + 5, bx + 2);
    write(lz->fd, login, strlen(login));
    strncpy(lz->login, default_user, BUF_SIZE-1);
    lz->login[BUF_SIZE-1] = '\0';
#else
    char login[BUF_SIZE];
    char hostname[BUF_SIZE];
    if (gethostname(hostname, sizeof(hostname)) == 0){
      snprintf(login, sizeof(login), "%.240s login: ", hostname);
    }
    term_goto(lz->fd, by + 5, bx + 2);
    write(lz->fd, login, strlen(login));

    struct termios current_term = lz->old_term;
    current_term.c_lflag |= (ICANON | ECHO | ICRNL | ISIG);
    if(tcsetattr(lz->fd, TCSAFLUSH, &current_term) == -1){
      term_reset(lz->fd);
      close(lz->fd);
      free(lz);
      exit(1);
    }

    tcflush(lz->fd, TCIFLUSH);

    ssize_t ln = read(lz->fd, lz->login, BUF_SIZE - 1);
    if (ln <= 0){
      if (errno == EINTR){
        memset(lz->login, 0, sizeof(lz->login));
        tcsetattr(lz->fd, TCSANOW, &current_term);
        term_reset(lz->fd);
        term_clear(lz->fd);
        return false;
      }

      term_reset(lz->fd);
      close(lz->fd);
      free(lz);
      exit(1);
    }
    lz->login[ln] = '\0';
    strip_newline(lz->login);

    if (strlen(lz->login) == 0){
      return false;
    }
#endif

  struct termios passwd_term = lz->old_term;
  passwd_term.c_lflag &= ~ECHO;
  passwd_term.c_lflag |= (ICANON | ICRNL | ISIG);

  if (tcsetattr(lz->fd, TCSAFLUSH, &passwd_term) == -1){
    term_reset(lz->fd);
    close(lz->fd);
    free(lz);
    exit(1);
  }

  term_goto(lz->fd, by + 7, bx + 2);
  write(lz->fd, "Password: ", 10);

  ssize_t ps = read(lz->fd, lz->passwd, BUF_SIZE - 1);
  if (ps <= 0){

    if (errno == EINTR){
      memset(lz->passwd, 0, sizeof(lz->passwd));
      tcsetattr(lz->fd, TCSAFLUSH, &passwd_term);
      term_reset(lz->fd);
      term_clear(lz->fd);
      return false;
    }
    term_reset(lz->fd);
    close(lz->fd);
    free(lz);
    exit(1);
  }
  lz->passwd[ps] = '\0';
  strip_newline(lz->passwd);

  if(tcsetattr(lz->fd, TCSANOW, &lz->old_term) == -1){
    term_reset(lz->fd);
    close(lz->fd);
    free(lz);
    exit(1);
  }
  return true;
}

bool authenticate(Config *lz, int bx, int by, int bw, int bh){
  const char *pam_auth = auth_user(lz->login, lz->passwd, &lz->pamh, pam_service);
  if (pam_auth != NULL){ 
    draw_err(lz->fd, by, bh, bx, bw, pam_auth);
    return false;
  }
  return true;
}

bool ses_init(Config *lz, Session *sessions, int *xcount, int *wcount, int bx, int by, int bh, int bw, int *num){
  (void)bh;
  (void)bw;
  load_session(sessions, xcount, xdirpath, 0);
  load_session(sessions + *xcount, wcount, wdirpath, 1);

#if AUTOLOGIN_SESSION == 0
    write(lz->fd, "\033[?25l", 6);
    char sess[BUF_SIZE];
    snprintf(sess, sizeof(sess), "Select: %s", default_session);
    term_goto(lz->fd, by + 10, bx + 2);
    write(lz->fd, sess, strlen(sess));
    for (int i = 0; i < *xcount + *wcount; i++){
      if (strcasecmp(sessions[i].name, default_session) == 0){
        *num = i;
        break;
      }
    }
#elif XINIT_USE == 0 && AUTOLOGIN_SESSION != 0
    write(lz->fd, "\033[?25l", 6);
    term_goto(lz->fd, by + 10, bx + 2);
    write(lz->fd, "Auto-start: xinitrc", strlen("Auto-start: xinitrc"));
    *num = 0;
#else 
      term_goto(lz->fd, by + 10, bx + 2);
      for (int i = 0; i < *xcount + *wcount; i++){
        char ses[BUF_SIZE];
        snprintf(ses, sizeof(ses), "%s[%d]  ", sessions[i].name, i);
        write(lz->fd, ses, strlen(ses));
      }

      struct termios input_session = lz->old_term;
      input_session.c_lflag &= ~ICANON;
      input_session.c_lflag |= (ICRNL | ISIG | ECHO);

      if (tcsetattr(lz->fd, TCSAFLUSH, &input_session) == -1){
        term_reset(lz->fd);
        close(lz->fd);
        free(lz);
        exit(1);
      }

      int total = *xcount + *wcount;
      
      write(lz->fd, "\033[?25l", 6);
 
      if (total != 0){
        char count[16];
        ssize_t sn = read(lz->fd, count, sizeof(count)-1);
        if (sn <= 0){
          term_reset(lz->fd);
          close(lz->fd);
          free(lz);
          exit(1);
        }
        count[sn] = '\0';
        strip_newline(count);
        *num = atoi(count);
      }

      if(tcsetattr(lz->fd, TCSANOW, &lz->old_term) == -1){
      term_reset(lz->fd);
      close(lz->fd);
      free(lz);
      exit(1);
      }

      if (total > 0 && (*num < 0 || *num >= total)){
        draw_err(lz->fd, by, bh, bx, bw, "Error: bad session number"); 
        return false;
      }
#endif
  return true;
}

bool launch_session(Config *lz, Session *sessions, Start *s, pid_t pid, int by, int bh, int bx, int bw, int num, int xcount, int wcount){
  if (pid == 0){
    setsid();
    ioctl(lz->fd, TIOCSCTTY, 1);
    dup2(lz->fd, STDIN_FILENO); 
    if (lz->fd > 2) close(lz->fd);

    int devnull = open("/dev/null", O_WRONLY);
      if (devnull != -1) {
        dup2(devnull, STDOUT_FILENO);
        dup2(devnull, STDERR_FILENO); 
        close(devnull);
      }
   
    start_session(s);

    char ses_name[BUF_SIZE];
    if (xcount + wcount > 0){
      snprintf(ses_name, sizeof(ses_name), "Error: failed start %.220s", sessions[num].name);
    } else {
      snprintf(ses_name, sizeof(ses_name), "Error: failed start %.220s", s->pw->pw_shell);
    }

    draw_err(STDERR_FILENO, by, bh, bx, bw, ses_name);
    _exit(1);

  } else if (pid > 0) {

      int status;
      waitpid(pid, &status, 0); 

      if (WIFSIGNALED(status)) {
          draw_err(lz->fd, by, bh, bx, bw, "Error: session crashed");
          sleep(SLEEP_TIME);
      }

      ioctl(lz->fd, KDSKBMODE, K_XLATE);

      close(lz->fd);
      lz->fd = open(lz->tty_path, O_RDWR);
      if(lz->fd < 0){
        free(lz);
        exit(1);
      }
     
      tcsetattr(lz->fd, TCSAFLUSH, &lz->old_term);
      tcflush(lz->fd, TCIOFLUSH); 

      pam_close_session(s->pamh, 0);
      pam_setcred(s->pamh, PAM_DELETE_CRED);
      pam_end(s->pamh, PAM_SUCCESS);
      lz->pamh = NULL;
      clean(s, sessions);
    
  } else {

    draw_err(lz->fd, by, bh, bx, bw, "Error: failed fork start");
    clean(s, sessions);
    return false;
  }
  return true;
}

void clean(Start *s, Session *sessions){
  if (s){
    if (s->cmd) { free((char *)s->cmd); }
    if (s->pw){
      if (s->pw->pw_name) { free(s->pw->pw_name); }
      if (s->pw->pw_shell) { free(s->pw->pw_shell); }
      if (s->pw->pw_dir) { free(s->pw->pw_dir); }
      free(s->pw);
    }
    free(s);
  }
  if (sessions) { free(sessions); }
}

int main() {

  Config *lz = (Config *)calloc(1, sizeof(Config));
  if (!lz){
    return 1;
  }

  if (!init_tty(lz)) { return 1; }

  while(running){ 

    int bw = 70, bh = 20, bx, by, term_x, term_y;
    draw_ui(lz->fd, bw, bh, &bx, &by, &term_x, &term_y); 

    if (!get_user_input(lz, bx, by)){ continue; }
    
    if (!authenticate(lz, bx, by, bw, bh)) { continue; }

    Session *sessions = malloc(sizeof(Session)*MAX_SES);
    if (!sessions){
      draw_err(lz->fd, by, bh, bx, bw, "Error: malloc failed");
      continue;
    }

    int xcount = 0, wcount = 0;
    int num = 0;
    if (!ses_init(lz, sessions, &xcount, &wcount, bx, by, bh, bw, &num)){
      clean(NULL, sessions);
      continue;
    }

    Start *s = calloc(1, sizeof(Start));
    if (!s){
      clean(NULL, sessions);
      return 1;
    }
    if (!prepare_session(lz, s, sessions, num, by, bx, bh, bw, &xcount, &wcount)) {
      clean(s, sessions);
      continue;
    }

    pid_t pid = fork();
    if (!launch_session(lz, sessions, s, pid, by, bh, bx, bw, num, xcount, wcount)){
      continue;
    }
  }
  term_reset(lz->fd);
  close(lz->fd);  
  free(lz);

  return 0;
}
