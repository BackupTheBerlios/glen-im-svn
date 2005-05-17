
#include "win_edit.h"
#include "win_main.h"
#include "interface.h"
#include "support.h"
#include "tlen.h"
#include "misc.h"

static void accept_clicked_cb(gpointer data);
static void cancel_clicked_cb(gpointer data);
static void fill_combobox_group(GtkWidget *, User *);

static GtkWidget *win = NULL;
static gboolean existing = FALSE;

void win_edit_create(User *user)
{
	GtkWidget *w;
	
	/* tylko jedno okno dozwolone */
	if(win != NULL)
		return;

	win = create_win_edit();
	
	if(user == NULL) {
		existing = FALSE;
		gtk_window_set_title(GTK_WINDOW(win), "Nowy kontakt");
	} else {
		existing = TRUE;
	}
	
	w = lookup_widget(win, "entry_id");
	g_assert(w != NULL);
	
	if(existing == TRUE) {
		gtk_entry_set_text(GTK_ENTRY(w), user->id);
		gtk_widget_set_sensitive(w, FALSE);
	}

	w = lookup_widget(win, "entry_name");
	g_assert(w != NULL);
	
	if(existing == TRUE) {
		if(USER_FLAG_ISSET(user, UserFlagNoName))
			gtk_entry_set_text(GTK_ENTRY(w), "");
		else
			gtk_entry_set_text(GTK_ENTRY(w), user->name);
	}

	w = lookup_widget(win, "combobox_group");
	g_assert(w != NULL);

	if(existing == TRUE)
		fill_combobox_group(w, user);
	else
		gtk_widget_set_sensitive(w, FALSE);

	w = lookup_widget(win, "button_accept");
	g_assert(w != NULL);
	g_signal_connect_swapped(G_OBJECT(w), "clicked",
		G_CALLBACK(accept_clicked_cb), user);
	
	w = lookup_widget(win, "button_cancel");
	g_assert(w != NULL);
	g_signal_connect_swapped(G_OBJECT(w), "clicked",
		G_CALLBACK(cancel_clicked_cb), user);
	g_signal_connect_swapped(G_OBJECT(win), "delete_event",
		G_CALLBACK(cancel_clicked_cb), user);

	if(existing == TRUE) {
		gtk_window_set_icon(GTK_WINDOW(win),
			get_status_icon_pixbuf(user->status));
	} else {
		gtk_window_set_icon(GTK_WINDOW(win),
			get_status_icon_pixbuf(TLEN_PRESENCE_AVAILABLE));
	}

	gtk_widget_show(win);
}

void accept_clicked_cb(gpointer user)
{
	User *u;
	gchar *name, *group, *id;
	GtkWidget *w;
	Group *old, *g;

	name = group = id = NULL;
	
	gtk_widget_hide(win);

	w = lookup_widget(win, "combobox_group");
	group = (gchar *)gtk_combo_box_get_active_text(GTK_COMBO_BOX(w));

	w = lookup_widget(win, "entry_name");
	name = (gchar *)gtk_entry_get_text(GTK_ENTRY(w));

	w = lookup_widget(win, "entry_id");
	id = (gchar *)gtk_entry_get_text(GTK_ENTRY(w));

	if(group != NULL && strlen(group) == 0)
		group = "Kontakty";

	u = user_get(id);
	
	if(u != NULL)
		existing = TRUE;
	
	if(existing == FALSE) {
		printf("-- user nie istnieje, dodaje: %s\n", name);
		u = user_new_full(id, name, NULL, TLEN_PRESENCE_UNAVAILABLE);

		g = group_get(GROUP_NAME_NO_AUTH);
		if(g == NULL)
			g = group_new(GROUP_NAME_WAIT_AUTH);

		user_set_group(u, g);
		win_main_user_add(u);

		USER_FLAG_CLEAR(u, UserFlagWantsSubscribtion);

		o2_contact_add(id, NULL, group);
		o2_subscribe_request(id);

		gtk_widget_destroy(win);
		win = NULL;

		return;
	}

	g = group_get(group);
	if(g == NULL)
		g = group_new(group);

	if(u == NULL)
		u = (User *)user;
	
	old = u->group;

	/* zmiana nazwy */
	if(strcmp(u->name, name) != 0) {
		if(strlen(name) == 0)
			user_set_name(u, NULL);
		else {
			user_set_name(u, name);
		}
	}

	/* grupa zmienila sie */
	if(old != u->group) {
		win_main_user_set_group(u, g);
	}

	o2_contact_add(u->id, u->name, group);
	if(USER_FLAG_ISSET(u, UserFlagNoSubscribtion)) {
		o2_subscribe_request(u->id);
	}

	win_main_user_update(u);

	gtk_widget_destroy(win);
	win = NULL;
}

void cancel_clicked_cb(gpointer user)
{
	gtk_widget_hide(win);
	
/* user nie byl dotykany przez win_main, czyli anulowane tworzenie nowego */
	if(existing == TRUE && ((User *)user)->row == NULL) {
		user_remove(user);
	}

	gtk_widget_destroy(win);
	win = NULL;
}

static void fill_combobox_group(GtkWidget *cb, User *u)
{
	GSList *l;
	Group *g;//, *k;
	int i = 0;

	if(existing == TRUE)
		gtk_combo_box_append_text(GTK_COMBO_BOX(cb), u->group->name);

	for(l = group_get_list(); l != NULL; l = l->next) {
		g = (Group *)l->data;
		if(existing == TRUE && g == u->group)// || g == k)
			continue;

		gtk_combo_box_append_text(GTK_COMBO_BOX(cb), g->name);
		i++;
	}

	gtk_combo_box_set_active(GTK_COMBO_BOX(cb), 0);
}

