
#include <gnome.h>
#include "win_prefs.h"
#include "prefs.h"
#include "interface.h"
#include "misc.h"
#include "support.h"
#include "win_main.h"
#include "tlen.h"
#include "user.h"
#include "emotes.h"

static GtkWidget *win = NULL;

static void fill_emote_set_combo();

static void cancel_cb(GtkWidget *w, gpointer data);
static void accept_cb(GtkWidget *w, gpointer data);

static void checkbutton_emots_cb(GtkWidget *w, gpointer data);
static void checkbutton_autoaway_cb(GtkWidget *w, gpointer data);
static void radio_start_status_cb(GtkWidget *w, gpointer data);

void win_prefs_show()
{
	GtkWidget *w;
	
	if(win != NULL) {
		gtk_widget_show_all(win);
		return;
	}

	win = create_win_prefs();

	/* Identyfikacja */
	w = lookup_widget(win, "entry_id");
	gtk_entry_set_text(GTK_ENTRY(w), pref_tlen_id);
	w = lookup_widget(win, "entry_pass");
	gtk_entry_set_text(GTK_ENTRY(w), pref_tlen_pass);

	/* Okno g³ówne */
	// nic na razie

	/* Okno rozmowy */
	w = lookup_widget(win, "checkbutton_emots");
	g_signal_connect(G_OBJECT(w), "toggled",
		G_CALLBACK(checkbutton_emots_cb), NULL);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w),
		pref_chat_use_emotes);

	fill_emote_set_combo();

	/* Stan */
	w = lookup_widget(win, "checkbutton_autoaway");
	g_signal_connect(G_OBJECT(w), "toggled",
		G_CALLBACK(checkbutton_autoaway_cb), NULL);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w),
		pref_status_auto_away);

	w = lookup_widget(win, "checkbutton_autoaway_desc");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w),
		pref_status_auto_away_desc);

	w = lookup_widget(win, "spinbutton_away_time");
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(w),
		(gdouble)pref_status_auto_away_time);

	w = lookup_widget(win, "spinbutton_bbl_time");
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(w),
		(gdouble)pref_status_auto_bbl_time);

		/* --- */

	w = lookup_widget(win, "checkbutton_desc_save");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w),
		pref_status_save_desc);
	w = lookup_widget(win, "radiobutton_status_save");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w),
		pref_status_save_status);
	w = lookup_widget(win, "radiobutton_start_status");
	g_signal_connect(G_OBJECT(w), "toggled",
		G_CALLBACK(radio_start_status_cb), NULL);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w),
		!pref_status_save_status);
	
	// FIXME
	/* glade nie posiada combobox zwyklego, dla uproszczenia sprawy z
	 * interface.c wylaczamy tutaj mozliwosc edycji */
	//gtk_entry_set_editable(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(w))),
	//	FALSE);
	
	w = lookup_widget(win, "comboboxentry_start_status");
	gtk_combo_box_set_active(GTK_COMBO_BOX(w),
		o2_presence_to_index(pref_status_on_start));
	gtk_widget_set_sensitive(w, !pref_status_save_status);

	/* Pozostale */
	w = lookup_widget(win, "entry_browser");
	gtk_entry_set_text(GTK_ENTRY(w), pref_misc_browser_cmd);

	/* przyciski */
	w = lookup_widget(win, "button_accept");
	g_signal_connect(G_OBJECT(w), "clicked", G_CALLBACK(accept_cb),
		NULL);

	w = lookup_widget(win, "button_cancel");
	g_signal_connect(G_OBJECT(w), "clicked", G_CALLBACK(cancel_cb),
		NULL);

	g_signal_connect(G_OBJECT(win), "destroy_event",
		G_CALLBACK(cancel_cb), NULL);
		
	gtk_widget_show_all(win);
}

static void checkbutton_emots_cb(GtkWidget *w, gpointer data)
{
	gboolean on = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w));

	w = lookup_widget(win, "comboboxentry_emote_set");
	gtk_widget_set_sensitive(w, on);
	w = lookup_widget(win, "label_emote_set");
	gtk_widget_set_sensitive(w, on);
}

static void checkbutton_autoaway_cb(GtkWidget *w, gpointer data)
{
	gboolean on = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w));

	w = lookup_widget(win, "label_aa1");
	gtk_widget_set_sensitive(w, on);
	w = lookup_widget(win, "spinbutton_away_time");
	gtk_widget_set_sensitive(w, on);
	w = lookup_widget(win, "spinbutton_bbl_time");
	gtk_widget_set_sensitive(w, on);
	w = lookup_widget(win, "label_aa11");
	gtk_widget_set_sensitive(w, on);
	w = lookup_widget(win, "label_aa2");
	gtk_widget_set_sensitive(w, on);
	w = lookup_widget(win, "label_aa22");
	gtk_widget_set_sensitive(w, on);
	w = lookup_widget(win, "checkbutton_autoaway_desc");
	gtk_widget_set_sensitive(w, on);
}

static void radio_start_status_cb(GtkWidget *w, gpointer data)
{
	gboolean on = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w));
	
	w = lookup_widget(win, "comboboxentry_start_status");
	gtk_widget_set_sensitive(w, on);
}

static void cancel_cb(GtkWidget *w, gpointer data)
{
	gtk_widget_hide(win);
	gtk_widget_destroy(win);
	win = NULL;
}

static void accept_cb(GtkWidget *widget, gpointer data)
{
	GtkWidget *w;
	gchar *s;
	gboolean b;

	w = lookup_widget(win, "entry_id");
	s = (gchar *)gtk_entry_get_text(GTK_ENTRY(w));
	if(strcmp(s, pref_tlen_id) != 0) {
		g_free(pref_tlen_id);
		pref_tlen_id = g_strdup(s);

		/* XXX: powtorne zalogowanie? */
	}

	w = lookup_widget(win, "entry_pass");
	s = (gchar *)gtk_entry_get_text(GTK_ENTRY(w));
	if(strcmp(s, pref_tlen_pass) != 0) {
		memset(pref_tlen_pass, 0, strlen(pref_tlen_pass));
		g_free(pref_tlen_pass);
		pref_tlen_pass = g_strdup(s);
	}

	/* Okno rozmowy */
	w = lookup_widget(win, "checkbutton_emots");
	b = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w));
	if(b == FALSE) {
		emote_unload_set();
	} else if(b == TRUE && pref_chat_use_emotes == FALSE) {
		emote_load_set(pref_chat_emote_set);
	} else if(b == TRUE && pref_chat_use_emotes == TRUE) {
		w = lookup_widget(win, "comboboxentry_emote_set");
		s = (gchar *)gtk_combo_box_get_active_text(GTK_COMBO_BOX(w));
		if(strcmp(s, pref_chat_emote_set) != 0) {
			emote_unload_set();
			emote_load_set(s);
			g_free(pref_chat_emote_set);
			pref_chat_emote_set = g_strdup(s);
		}
	}

	pref_chat_use_emotes = b;
	
	/* Stan */
	w = lookup_widget(win, "checkbutton_autoaway");
	pref_status_auto_away =
		gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w));
	
	if(pref_status_auto_away == TRUE) {
		w = lookup_widget(win, "spinbutton_away_time");
		pref_status_auto_away_time =
			(guint)gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(w));

		w = lookup_widget(win, "spinbutton_bbl_time");
		pref_status_auto_bbl_time =
			(guint)gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(w));

		w = lookup_widget(win, "checkbutton_autoaway_desc");
		pref_status_auto_away_desc =
			gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w));
	}
	
		/* ---- */
	
	w = lookup_widget(win, "checkbutton_desc_save");
	pref_status_save_desc =
		gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w));
	w = lookup_widget(win, "radiobutton_status_save");
	pref_status_save_status =
		gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w));
	
	w = lookup_widget(win, "comboboxentry_start_status");
	pref_status_on_start =
		get_status_num(gtk_combo_box_get_active_text(
					GTK_COMBO_BOX(w)));
	
	/* Pozostale */
	w = lookup_widget(win, "entry_browser");
	s = (gchar *)gtk_entry_get_text(GTK_ENTRY(w));
	g_free(pref_misc_browser_cmd);
	pref_misc_browser_cmd = g_strdup(s);

	gtk_widget_hide(win);
	gtk_widget_destroy(win);
	win = NULL;
	
	prefs_save();
	win_main_reset_autoaway_timer();
}

static void fill_emote_set_combo()
{
	GtkWidget *w;
	GDir *dir;
	gchar *set;
	gchar *path;
	gchar buf[1024];
	FILE *fp;
	int i, active=0;

  	path = gnome_program_locate_file(NULL, GNOME_FILE_DOMAIN_APP_PIXMAP,
		"glen/emotes/", FALSE, NULL);

	dir = g_dir_open(path, 0, NULL);

	if(dir == NULL) {
		/*XXX okienko */
		g_warning("Nie mozna otworzyc %s", path);
		g_free(path);
		return;
	}

	w = lookup_widget(win, "comboboxentry_emote_set");
	gtk_widget_set_sensitive(w, pref_chat_use_emotes);
	
	i = 0;
	for(set = (gchar *)g_dir_read_name(dir); set != NULL; 
		set = (gchar *)g_dir_read_name(dir)) {

		snprintf(buf, sizeof(buf), "%s/%s/emo-utf.xml", path, set);

		fp = fopen(buf, "r");
		if(fp == NULL)
			continue;

		fclose(fp);
		gtk_combo_box_append_text(GTK_COMBO_BOX(w), set);
		
		if(strcmp(set, pref_chat_emote_set) == 0)
			active = i;

		i++;
	}

	gtk_combo_box_set_active(GTK_COMBO_BOX(w), active);

	g_dir_close(dir);
	g_free(path);
}
