/* See LICENSE file for copyright and license details. */

#ifndef DRAW_H
#define DRAW_H

#include <bsd/string.h>
#include <err.h>
#include <ncursesw/curses.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wchar.h>
#include "defs.h"
#include "mix.h"
#include "string.h"
#include "util.h"

void	draw_error(char *);
void	draw_exit(void);
void	draw_init(void);
void	draw_redraw(struct info *);

#endif
