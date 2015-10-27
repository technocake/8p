/* See LICENSE file for copyright and license details. */

#ifndef SELECT_H
#define SELECT_H

#include <ncursesw/curses.h>
#include <wchar.h>
#include "defs.h"
#include "mix.h"
#include "play.h"
#include "util.h"

void	select_exit(struct info *);
void	select_init(struct info *);
void	select_changepos(struct info *, wint_t);
void	select_select(struct info *);

#endif
