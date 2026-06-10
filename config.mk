PREFIX = /usr/local
MANPREFIX = ${PREFIX}/share/man

CC = cc

CFLAGS = -std=c11 -pedantic -Wall -Wextra -Werror -O2 -D_POSIX_C_SOURCE=200809L -D_XOPEN_SOURCE=700 -fstack-protector-strong -D_FORTIFY_SOURCE=2
LDFLAGS = -lpam

VERSION = 1.0
