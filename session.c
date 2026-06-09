#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <grp.h>
#include <pwd.h>
#include <security/pam_appl.h>
#include <sys/ioctl.h>
#include <linux/kd.h>
#include <limits.h>

#include "session.h"
#include "config.h"
#include "utils.h"
#include "term.h"

int start_session(Start *s){

  int ret = pam_open_session(s->pamh, 0);
  if (ret != PAM_SUCCESS){ return 1; }
  pam_setcred(s->pamh, PAM_ESTABLISH_CRED); 

  char **env = pam_getenvlist(s->pamh);
  if (env) {
    for (int i = 0; env[i]; i++) {
        putenv(env[i]);
    }
  }
  setenv("TERM", "linux", 1);
  setenv("HOME", s->pw->pw_dir, 1);
  setenv("USER", s->pw->pw_name, 1);
  setenv("LOGNAME", s->pw->pw_name, 1);
  setenv("SHELL", s->pw->pw_shell, 1); 

  char path[PATH_MAX];
  snprintf(path, sizeof(path), "/usr/local/bin:/usr/bin:/bin:/usr/sbin:/sbin:%s/.local/bin", s->pw->pw_dir);
  setenv("PATH", path, 1);

  char tty[32];
  snprintf(tty, sizeof(tty), "/dev/tty%d", tty_number);
  chown(tty, s->pw->pw_uid, s->pw->pw_gid);

  ioctl(0, KDSKBMODE, K_UNICODE);

  if (initgroups(s->pw->pw_name, s->pw->pw_gid) == -1){ return 1; }
  if (setgid(s->pw->pw_gid) == -1){ return 1; }
  if (setuid(s->pw->pw_uid) == -1){ return 1; }
  
  chdir(s->pw->pw_dir); 

  char *args[] = { "/bin/sh", "-c", (char*)s->cmd, NULL };   

  execvp(args[0], args);
  perror("exec failed");
  sleep(SLEEP_TIME);

  _exit(1);
}

bool prepare_session(Config *lz, Start *s, Session *sessions, int num, int by, int bx, int bh, int bw, const int *xcount, const int *wcount){
  s->pamh = lz->pamh;

  const struct passwd *tmp_pw = getpwnam(lz->login);
  if (!tmp_pw) {
    char us[BUF_SIZE];
    snprintf(us, sizeof(us), "Error: %.230s not found", lz->login);
    draw_err(lz->fd, by, bh, bx, bw, us);
    free(sessions);
    return false;
  }
  s->pw = malloc(sizeof(struct passwd));
  *s->pw = *tmp_pw;
  s->pw->pw_name = strdup(tmp_pw->pw_name);
  s->pw->pw_dir = strdup(tmp_pw->pw_dir);
  s->pw->pw_shell = strdup(tmp_pw->pw_shell);
  
  int total = *xcount + *wcount;
  if (total == 0){
    s->cmd = strdup(s->pw->pw_shell);
  } else {

    if (num < *xcount) {

      const char *abs_exec = get_abs_path(sessions[num].exec);
      char full_cmd[PATH_MAX];
#if XINIT_USE == 0
      snprintf(full_cmd, sizeof(full_cmd), "startx %s/.xinitrc -- :%d vt%d", s->pw->pw_dir, display_number, tty_number);
#else
      snprintf(full_cmd, sizeof(full_cmd), "exec /usr/bin/xinit %s -- /usr/bin/Xorg :%d vt%d", abs_exec, display_number, tty_number);
#endif
      s->cmd = strdup(full_cmd);
      free((char *)abs_exec);
    } else {
        const char *abs_exec = get_abs_path(sessions[num].exec);
        s->cmd = abs_exec;
      }
    } 

  s->fd = lz->fd;
  return true;
}

