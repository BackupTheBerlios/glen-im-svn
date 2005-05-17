
#include "prefs.h"
#include "win_desc.h"
#include "win_main.h"
#include "support.h"
#include "misc.h"
#include "tlen.h"
#include "interface.h"

GtkWidget *win_desc = NULL;
gint win_desc_status;
gchar *win_desc_desc = NULL;

static void radio_clicked_cb(GtkWidget *w, gpointer user_data);
static void accept_clicked_cb(GtkWidget *w, gpointer user_data);
static void cancel_clicked_cb(GtkWidget *w, gpointer user_data);

void win_desc_show()
{
	GtkWidget *b;
	GdkPixbuf *pb;
	guint status = o2_get_status();
	gchar *desc;

	if(win_desc != NULL)
		gtk_widget_show_all(win_desc);

	win_desc = create_win_desc();
	gtk_widget_show_all(win_desc);

	pb = gtk_image_get_pixbuf(GTK_IMAGE(get_status_icon(o2_get_status())));
	gtk_window_set_icon(GTK_WINDOW(win_desc), pb);

	b = lookup_widget(win_desc, "radio_avail");
	g_signal_connect(G_OBJECT(b), "toggled",
		G_CALLBACK(radio_clicked_cb), "1");
	if(status == TLEN_PRESENCE_AVAILABLE)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(b), TRUE);

	b = lookup_widget(win_desc, "radio_chatty");
	g_signal_connect(G_OBJECT(b), "toggled",
		G_CALLBACK(radio_clicked_cb), "2");
	if(status == TLEN_PRESENCE_CHATTY)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(b), TRUE);

	b = lookup_widget(win_desc, "radio_dnd");
	g_signal_connect(G_OBJECT(b), "toggled",
		G_CALLBACK(radio_clicked_cb), "3");
	if(status == TLEN_PRESENCE_DND)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(b), TRUE);

	b = lookup_widget(win_desc, "radio_away");
	g_signal_connect(G_OBJECT(b), "toggled",
		G_CALLBACK(radio_clicked_cb), "4");
	if(status == TLEN_PRESENCE_AWAY)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(b), TRUE);

	b = lookup_widget(win_desc, "radio_bbl");
	g_signal_connect(G_OBJECT(b), "toggled",
		G_CALLBACK(radio_clicked_cb), "5");
	if(status == TLEN_PRESENCE_EXT_AWAY)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(b), TRUE);

	b = lookup_widget(win_desc, "radio_invisible");
	g_signal_connect(G_OBJECT(b), "toggled",
		G_CALLBACK(radio_clicked_cb), "6");
	if(status == TLEN_PRESENCE_INVISIBLE)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(b), TRUE);

	b = lookup_widget(win_desc, "radio_unavail");
	g_signal_connect(G_OBJECT(b), "toggled",
		G_CALLBACK(radio_clicked_cb), "7");
	if(status == TLEN_PRESENCE_UNAVAILABLE)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(b), TRUE);

	b = lookup_widget(win_desc, "image_status");

	gtk_image_set_from_pixbuf(GTK_IMAGE(b), pb);
	gtk_widget_queue_draw(b);

	b = lookup_widget(win_desc, "entry_desc");
	desc = toutf(o2_get_desc());
	if(desc != NULL) {
		gtk_entry_set_text(GTK_ENTRY(b), desc);
	}
	g_free(desc);
	gtk_widget_grab_focus(b);

	b = lookup_widget(win_desc, "btn_ustaw");
	g_signal_connect(G_OBJECT(b), "clicked",
		G_CALLBACK(accept_clicked_cb), NULL);

	b = lookup_widget(win_desc, "btn_anuluj");
	g_signal_connect(G_OBJECT(b), "clicked",
		G_CALLBACK(cancel_clicked_cb), NULL);

	g_signal_connect(G_OBJECT(win_desc), "delete_event",
		G_CALLBACK(cancel_clicked_cb), NULL);
}

static void radio_clicked_cb(GtkWidget *w, gpointer id)
{
	GtkWidget *img;
	GdkPixbuf *pb;
	guint status = o2_get_status();

	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w)) == FALSE)
		return;

	if(g_strcasecmp(id, "1") == 0)
		status = TLEN_PRESENCE_AVAILABLE;
	else if(g_strcasecmp(id, "2") == 0)
		status = TLEN_PRESENCE_CHATTY;
	else if(g_strcasecmp(id, "3") == 0)
		status = TLEN_PRESENCE_DND;
	else if(g_strcasecmp(id, "4") == 0)
		status = TLEN_PRESENCE_AWAY;
	else if(g_strcasecmp(id, "5") == 0)
		status = TLEN_PRESENCE_EXT_AWAY;
	else if(g_strcasecmp(id, "6") == 0)
		status = TLEN_PRESENCE_INVISIBLE;
	else if(g_strcasecmp(id, "7") == 0)
		status = TLEN_PRESENCE_UNAVAILABLE;

	img = lookup_widget(win_desc, "image_status");
	pb = gtk_image_get_pixbuf(GTK_IMAGE(get_status_icon(status)));
	gtk_image_set_from_pixbuf(GTK_IMAGE(img), pb);

	win_desc_status = status;
}

static void accept_clicked_cb(GtkWidget *w, gpointer udata)
{
	gchar *text;
	gchar *desc;
	GtkWidget *desc_entry;

	desc_entry = lookup_widget(win_desc, "entry_desc");
	text = (gchar *)gtk_entry_get_text(GTK_ENTRY(desc_entry));
	if(strlen(text) == 0)
		desc = NULL;
	else
		desc = text;

	o2_set_status(win_desc_status, desc);

	gtk_widget_hide(win_desc);
	gtk_widget_destroy(win_desc);
	win_desc = NULL;
}

static void cancel_clicked_cb(GtkWidget *w, gpointer data)
{
	gtk_widget_hide(win_desc);
	gtk_widget_destroy(win_desc);
	win_desc = NULL;
}
