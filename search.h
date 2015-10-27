/* See LICENSE file for copyright and license details. */

#ifndef SEARCH_H
#define SEARCH_H

#include <err.h>
#include <jansson.h>
#include <ncursesw/curses.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include "defs.h"
#include "draw.h"
#include "fetch.h"
#include "select.h"
#include "string.h"

void	search_init(struct info *);
void	search_exit(struct info *);
void	search_backspace(struct info *);
void	searchstr_clear(struct info *);
void	search_delete(struct info *);
void	search_changepos(struct info *, wint_t);
void	search_addchar(struct info *, wint_t);
void	search_search(struct info *);
int	search_nextmix(struct info *);

#endif
