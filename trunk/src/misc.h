
#ifndef MISC_H
#define MISC_H

#include <gnome.h>
#include <glib.h>
#include <time.h>

gchar *toutf(const gchar *str);
gchar *fromutf(const gchar *str);
gchar *markup_text(const gchar *str);
/* sigsegv@tlen.pl -> sigsegv */
gchar *strip_user(const gchar *str);
const gchar *get_time(time_t *);
const gchar *get_date(time_t *);
/* TLEN_PRESENCE_AVAILABLE -> Dostêpny */
const gchar *get_status_desc(guint status);
/* "Dostêpny" -> TLEN_PRESENCE_AVAILABLE */
guint get_status_num(const gchar *name);

/* Bezpieczny sprintf. Zwraca 0 gdy wszystko ok, -1 gdy cos nie wyszlo */
int xsprintf(char *buf, size_t buflen, const char *fmt, ...);

gchar *text_view_get_text(GtkTextView *v);
void text_view_clear(GtkTextView *v);
void text_view_set_text(GtkTextView *v, const gchar *);

GtkWidget * yes_no_dialog(GtkWidget *win, const gchar *markup);

#define warn_if_fail(cond) \
	if(!(cond)) g_warning("Uwaga: %s:%i:%s: warunek %s nie spelniony",\
		__FILE__,__LINE__,__PRETTY_FUNCTION__, #cond);

#endif

