
#ifndef WIN_MAIN_H
#define WIN_MAIN_H

#include <gnome.h>
#include <gtk/gtk.h>
#include "win_chat.h"
#include "user.h"

extern GtkWidget *win_main;

GtkWidget * win_main_create(void);

void win_main_start_icon_blinking(guint status);
void win_main_stop_icon_blinking(void);

void win_main_reset_autoaway_timer(void);

void win_main_set_status(guint status, const gchar *desc);

gboolean win_main_is_iconified(void);
void win_main_toggle_hidden(void);

//void win_main_add_user(const gchar *id, const gchar *name, const gchar *desc,
//	const gchar *group, guint status);
//void win_main_update_user_status(const gchar *id, const gchar *desc, guint status);

void win_main_user_add(User *u);
void win_main_user_update(User *u);
void win_main_user_remove(User *u);
void win_main_user_set_group(User *u, Group *g);

void win_main_group_add(Group *g);
void win_main_group_remove(Group *g);
void win_main_group_update(Group *g);

//Group * win_main_group_get(const gchar *name);

void win_main_quit(void);

void win_main_clear(void);

void win_main_update_all(void);
void win_main_expand_all(void);

/* Zapisuje pozycjê i rozmiar okna do zmiennych, które potem wyl±duj± w configu. */
void win_main_geometry_save(void);
/* Przywraca pozycjê i rozmiar zapisane funkcj± wy¿ej. */
void win_main_geometry_restore(void);

/* FIXME: lepsze miejsce dla tego */
GtkWidget *get_status_icon(guint status);
GtkWidget *get_status_icon_new(guint status);
GdkPixbuf *get_status_icon_pixbuf(guint status);

#endif

