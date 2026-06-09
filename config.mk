PREFIX = /usr/local
MANPREFIX = ${PREFIX}/share/man

CC = gcc

CFLAGS = -std=c99 -pedantic -Wall -Wextra -O2 -D_POSIX_C_SOURCE=200809L -D_DEFAULT_SOURCE
LDFLAGS = -lpam

VERSION = 1.0
