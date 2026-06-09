#ifndef CONFIG_H
#define CONFIG_H

#include <limits.h>
#include <termios.h>
#include <security/pam_appl.h>

/* 
 * =====================================================================
 * GENERAL CONSTANTS & BUFFER SIZES
 * =====================================================================
 */

#define BUF_SIZE    256  // Maximum buffer size for strings (login, password, session names)
#define MAX_SES     50   // Maximum number of desktop sessions to load from .desktop files
#define SLEEP_TIME  2    // Time (in seconds) to pause and show errors before returning to login

/* 
 * =====================================================================
 * FEATURE TOGGLES (INVERTED LOGIC: 0 = Enabled/True, 1 = Disabled/False)
 * =====================================================================
 */

// XINIT_USE: Controls how X11 sessions are launched.
// 0 = Use ~/.xinitrc (runs: xinit ~/.xinitrc -- Xorg ...)
// 1 = Direct WM launch (runs: xinit /path/to/wm -- Xorg ...)
#define XINIT_USE           1

// AUTOLOGIN: Controls whether the DM automatically logs in the default user.
// 0 = Autologin ENABLED (skips login/password prompt)
// 1 = Autologin DISABLED (shows standard login prompt)
#define AUTOLOGIN           1

// AUTOLOGIN_SESSION: Controls how the session is selected when autologin is active.
// 0 = Auto-select the default_session defined below (skips session prompt)
// 1 = Prompt the user to manually select a session from the list
#define AUTOLOGIN_SESSION   1

// MOTD_SH: Controls the execution of the Message of the Day script.
// 0 = MOTD ENABLED (executes motd_path and draws it above the box)
// 1 = MOTD DISABLED (skips script execution)
#define MOTD_SH             1

/* 
 * =====================================================================
 * MAIN CONFIGURATION STRUCTURE
 * Holds the global state of the Display Manager during runtime.
 * =====================================================================
 */
typedef struct {
  int fd;                         // File descriptor for the active TTY device
  char tty_path[PATH_MAX];        // Absolute path to the TTY (e.g., "/dev/tty5")
  char login[BUF_SIZE];           // Buffer to store the entered username
  char passwd[BUF_SIZE];          // Buffer to store the entered password
  pam_handle_t *pamh;             // Pointer to the PAM (Pluggable Authentication Modules) handle
  struct termios old_term;        // Saved original terminal settings (to restore on exit/crash)
} Config;

/* 
 * =====================================================================
 * GLOBAL & STATIC CONFIGURATION VARIABLES
 * =====================================================================
 */

// Standard system binary paths used by get_abs_path() to resolve executable names
static const char *bin_path[]__attribute__((unused)) = { "/usr/local/bin", "/usr/bin", "/bin", "/usr/sbin", "/sbin", NULL };

// X11 and TTY configuration
static const int display_number = 0;   // X11 display number (usually :0 for the first X server)
static const int tty_number = 7;       // The TTY number where LazyDM will run (e.g., /dev/tty5)

// Terminal UI Colors (ANSI 256-color codes)
static const int fg_color = 47;        // Foreground text color
static const int bg_color = 40;        // Background fill color

// Session directories (Standard XDG paths for Display Managers)
#define xdirpath "/usr/share/xsessions"       // Directory for X11 .desktop files
#define wdirpath "/usr/share/wayland-sessions" // Directory for Wayland .desktop files

// PAM Configuration
#define pam_service "login" // The PAM service name to use for authentication

// Default values for Autologin and UI
#define default_user "user"       // Default username (used if AUTOLOGIN is 0, or pre-filled)
#define default_session "vxwm"    // Default session name to auto-select (must match the 'Name=' in .desktop)

// MOTD Script
#define motd_path "/etc/lazydm/motd.sh" // Absolute path to the bash script that generates the MOTD art

#endif // CONFIG_H
