/* See LICENSE file for copyright and license details. */

#include "play.h"

void
play_exit(struct info *data)
{
	libvlc_media_player_release(data->vlc_mp);
	libvlc_release(data->vlc_inst);
}

int
play_init(struct info *data)
{
	data->vlc_inst = libvlc_new(0, NULL);
	if (data->vlc_inst == NULL)
		goto error;
	data->vlc_mp = libvlc_media_player_new(data->vlc_inst);
	if (data->vlc_mp == NULL)
		goto error;

	/* Prevent VLC from printing errors to the console by directing stderr
	 * to /dev/null.  These error messages mess up the ncurses window.
	 */
	(void)freopen("/dev/null", "wb", stderr);

	return SUCCESS;

error:
	if (data->vlc_inst)
		libvlc_release(data->vlc_inst);

	return ERROR;
}

void
play_next(struct info *data)
{
	int errn;
	struct track *t;
	libvlc_media_t *media;

	/* Check if we are already on the last track.
	 * If so request a similar mix and continue playing.
	 */
	if (data->m->track_count > 0) {
		if (data->m->track[data->m->track_count-1]->last == TRUE) {
			errn = search_nextmix(data);
			if (errn == ERROR)
				return;
		}
	}

	t = mix_nexttrack(data);
	if (t == NULL)
		return;

	media = libvlc_media_new_location(data->vlc_inst, t->url);
	if (media == NULL)
		return;
	libvlc_media_player_set_media(data->vlc_mp, media);
	(void)libvlc_media_player_play(data->vlc_mp);
	libvlc_media_release(media);
}

void
play_nextmix(struct info *data)
{
	int errn;

	errn = search_nextmix(data);
	if (errn == ERROR)
		return;
	data->scroll = 0;
	play_next(data);
}

int
play_skip(struct info *data)
{
	struct track *t;

	t = data->m->track[data->m->track_count-1];
	if (t == NULL)
		return ERROR;
	if (t->skip_allowed == TRUE && t->last == FALSE) {
		play_next(data);
		return SUCCESS;
	} else
		return ERROR;
}

int
play_isoverthirtymark(struct info *data)
{
	if (libvlc_media_player_get_time(data->vlc_mp) >= (30 * 1000))
		return TRUE;
	else
		return FALSE;
}

int
play_ended(struct info *data)
{
	/* VLC states: IDLE/CLOSE=0, OPENING=1, BUFFERING=2, PLAYING=3, 
	 * PAUSE=4, STOPPING=5, ENDED=6, ERROR=7
	 */
	if (libvlc_media_player_get_state(data->vlc_mp) >= 6)
		return TRUE;
	else
		return FALSE;
}

void
play_togglepause(struct info *data)
{
	libvlc_media_player_pause(data->vlc_mp);
}
