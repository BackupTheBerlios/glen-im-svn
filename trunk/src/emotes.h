
#ifndef EMOTES_H
#define EMOTES_H

#include <gnome.h>

typedef struct {
	gchar *tooltip;	/* co pojawi sie po najechaniu na emotke myszka */
	GSList *tags;	/* np. :), :-) */
	gchar *data;	/* zawartosc pliku */
	gsize size;	/* rozmiar powyzszego */
	gchar *url;	/* nazwa pliku */
} Emote;

typedef struct {
	gchar *template; /* tag w html */
	GSList *emotes; /* lista Emote */
} EmoteSet;

gboolean emote_load_set(const gchar *name);
Emote *emote_get(const gchar *tag);
Emote *emote_get_by_url(const gchar *url);
void emote_unload_set();

extern EmoteSet * emote_set;

#endif

