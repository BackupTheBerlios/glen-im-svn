
#ifndef WIN_CHAT_H
#define WIN_CHAT_H

#include <gtk/gtk.h>
#include <gtkhtml/gtkhtml.h>

#include "user.h"

enum BlinkState {
	BlinkOff = 0,
	BlinkNormal,
	BlinkAlt
};

struct structChat
{
	gchar *id;
	GtkWindow *win;
	/* trzymamy wskazniki tutaj, zeby uniknac zbednych wywolan
	 * lookup_widget() */
	GtkHTMLStream *stream;
	GtkTextView *input;
	GtkHTML *output;
	gboolean enter_sends;
	struct structUser *user;
	guint blink_state;
	/* czy nalezy migac? */
	gboolean need_blinking;

	gboolean notify_sent;
};

typedef struct structChat Chat;

void chat_init(void);

Chat *win_chat_create(const gchar *id);
Chat *win_chat_get(const gchar *id, gboolean create_if_doesnt_exist,
	gboolean show);
void win_chat_set_typing(const gchar *id, gboolean on);
void win_chat_append(const gchar *id, const gchar *msg, gboolean me, const
	gchar *time);
void win_chat_update_title(const User *);

#endif

