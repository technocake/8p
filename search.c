/* See LICENSE file for copyright and license details. */

#include "search.h"

static void	searchstr_pop(struct info *);
static void	searchstr_push(struct info *, wint_t);
static size_t	searchstr_length(struct info *);

void
search_init(struct info *data)
{
	data->pstate = data->state;
	data->state = SEARCH;
	searchstr_clear(data);
	data->scroll = 0;
}

void
search_exit(struct info *data)
{
	data->state = data->pstate;
	searchstr_clear(data);
	data->scroll = 0;
}

void
search_backspace(struct info *data)
{
	if (data->cursor_pos > 0) {
		data->cursor_pos--;
		searchstr_pop(data);
	}
}

void
search_delete(struct info *data)
{
	searchstr_pop(data);
}

void
search_changepos(struct info *data, wint_t c)
{
	switch(c) {
	case KEY_LEFT:
		if (data->cursor_pos > 0)
			data->cursor_pos--;
		break;
	case KEY_RIGHT:
		if (data->cursor_pos < (int)searchstr_length(data))
			data->cursor_pos++;
		break;
	default:
		break;
	}
}

void
search_addchar(struct info *data, wint_t c)
{
	/* Filter out control characters */
	switch (c) {
	case L'\0':	/* FALLTHROUGH */
	case L'\a':	/* FALLTHROUGH */
	case L'\b':	/* FALLTHROUGH */
	case L'\t':	/* FALLTHROUGH */
	case L'\n':	/* FALLTHROUGH */
	case L'\v':	/* FALLTHROUGH */
	case L'\f':	/* FALLTHROUGH */
	case L'\r':	return;
	default:	break;
	}

	searchstr_push(data, c);
	data->cursor_pos++;
}

void
search_search(struct info *data)
{
	char errormsg[] = "Search returned no results.";
	char *js, *tmp, *url;
	int tmp_len;
	size_t len;
	struct search_node *it;
	int errn;
	json_t *root, *status, *mix_set, *mixes;
	int i;

	/* Initialize variables */
	root = NULL;
	data->mlist = NULL;

	/* Set data->search_str to the entered smart id.
	 * If no search string is entered, default to smart id "all".
	 */
	if (data->search_str != NULL)
		free(data->search_str);
	if (searchstr_length(data) == 0) {
		len = strlen("all") + 1;
		data->search_str = malloc(len * sizeof(char));
		if (data->search_str == NULL)
			err(1, NULL);
		(void)snprintf(data->search_str, len, "all");
	} else {
		/* Convert wchar * search string to char */
		len = searchstr_length(data) * MB_CUR_MAX + 1;
		data->search_str = malloc(len * sizeof(char));
		if (data->search_str == NULL)
			err(1, NULL);
		data->search_str[0] = '\0';
		tmp = malloc(MB_CUR_MAX * sizeof(char));
		if (tmp == NULL)
			err(1, NULL);
		for (it = data->slist_head; it != NULL; it = it->next) {
			tmp_len = wctomb(tmp, it->c);
			tmp[tmp_len] = '\0';
			(void)strlcat(data->search_str, tmp, len);
		}
		free(tmp);
	}
	searchstr_clear(data);

	/* Build url */
	len = strlen("http://8tracks.com/explore/") +
	    strlen(data->search_str) + strlen("?format=json&include=mixes[liked]") + 1;
	url = malloc(len * sizeof(char));
	if (url == NULL)
		err(1, NULL);
	(void)snprintf(url, len,
	    "http://8tracks.com/explore/%s?format=json&include=mixes[liked]",
	    data->search_str);

	/* Start the search */
	data->state = SEARCHING;
	draw_redraw(data);

	/* Fetch the url */
	js = malloc(1);
	if (js == NULL)
		err(1, NULL);
	errn = fetch(&js, url);
	if (errn == ERROR)
		goto error;

	/* Parse json*/
	root = json_loads(js, 0, NULL);
	if (root == NULL)
		goto error;
	status = json_object_get(root, "status");
	if (strcmp(json_string_value(status), "200 OK") != 0)
		goto error;
	mix_set = json_object_get(root, "mix_set");
	if (mix_set == NULL)
		goto error;
	mixes = json_object_get(mix_set, "mixes");
	if (!json_is_array(mixes))
		goto error;
	data->mlist_size = json_array_size(mixes);
	data->mlist = malloc(sizeof(struct mix *) * data->mlist_size);
	if (data->mlist == NULL)
		err(1, NULL);
	for (i = 0; i < (int)data->mlist_size; i++)
		data->mlist[i] = mix_create(json_array_get(mixes, i));

	select_init(data);

	/* Cleanup */
	json_decref(root);
	free(url);
	free(js);

	return;

error:
	if (url)
		free(url);
	if (js)
		free(js);
	if (root)
		json_decref(root);

	draw_error(errormsg);
	data->state = data->pstate;

	return;
}

int
search_nextmix(struct info *data)
{
	char *js, *url;
	int errn;
	size_t len;
	json_t *root, *status, *mix_set;

	js = NULL;
	url = NULL;
	root = NULL;

	/* Build url */
	len = strlen("http://8tracks.com/sets/") + strlen(data->playtoken) +
	    strlen("/next_mix?mix_id=") + intlen(data->m->id) +
	    strlen("&include=mixes[liked]&smart_id=") +
	    strlen(data->search_str) + 1;
	url = malloc(len * sizeof(char));
	if (url == NULL)
		err(1, NULL);
	(void)snprintf(url, len,
	    "http://8tracks.com/sets/%s/next_mix?mix_id=%d&include=mixes[liked]&smart_id=%s",
	    data->playtoken, data->m->id, data->search_str);

	/* Fetch url */
	js = malloc(1);
	if (js == NULL)
		err(1, NULL);
	errn = fetch(&js, url);
	if (errn == ERROR)
		goto error;

	/* Parse json */
	root = json_loads(js, 0, NULL);
	if (root == NULL)
		goto error;
	status = json_object_get(root, "status");
	if (strcmp(json_string_value(status), "200 OK") != 0)
		goto error;
	mix_set = json_object_get(root, "next_mix");
	if (mix_set == NULL)
		goto error;

	/* Set mix */
	mix_free(data->m);
	data->m = mix_create(mix_set);
	if (data->m == NULL)
		goto error;

	/* Cleanup */
	free(url);
	free(js);
	json_decref(root);
	
	return SUCCESS;

error:
	if (js)
		free(js);
	if (url)
		free(url);
	if (root)
		json_decref(root);

	return ERROR;
}

void
searchstr_clear(struct info *data)
{
	struct search_node *it, *tmp;

	it = data->slist_head;
	while (it != NULL) {
		tmp = it->next;
		free(it);
		it = tmp;
	}
	data->cursor_pos = 0;
	data->slist_head = NULL;
}

static size_t
searchstr_length(struct info *data)
{
	struct search_node *it;
	size_t len;

	for (len = 0, it = data->slist_head; it != NULL; it = it->next)
		len++;

	return len;
}

static void
searchstr_pop(struct info *data)
{
	struct search_node *it, *tmp;
	int i;

	if (data->slist_head == NULL)
		return;
	if (data->cursor_pos >= (int)searchstr_length(data))
		return;
	it = data->slist_head;
	if (data->cursor_pos == 0) {
		data->slist_head = it->next;
		free(it);
	} else {
		for (i = 1; i < data->cursor_pos; i++)
			it = it->next;
		tmp = it->next;
		it->next = it->next->next;
		free(tmp);
	}
}

static void
searchstr_push(struct info *data, wint_t c)
{
	struct search_node *it, *new;
	int i;

	new = malloc(sizeof(struct search_node));
	if (new == NULL)
		err(1, NULL);
	new->c = c;
	new->next = NULL;

	if (data->cursor_pos == 0) {
		new->next = data->slist_head;
		data->slist_head = new;
	} else {
		it = data->slist_head;
		for (i = 1; i < data->cursor_pos; i++)
			it = it->next;
		new->next = it->next;
		it->next = new;
	}
}
