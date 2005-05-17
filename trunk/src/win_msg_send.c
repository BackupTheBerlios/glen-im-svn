
#include "win_msg_send.h"
#include "tlen.h"
#include "misc.h"
#include "interface.h"
#include "support.h"

static void send_button_cb(GtkWidget *w, gpointer data);
static void close_button_cb(GtkWidget *w, gpointer data);

void win_msg_send_show(User *u, const gchar *text)
{
	GtkWidget *w;
	gchar *txt;
	
	/* tylko jedno okno dla usera */
	if(u->win_msg_send != NULL) {
		gtk_widget_show(u->win_msg_send);
		return;
	//	gtk_widget_hide(u->win_msg_send);
	//	gtk_widget_destroy(u->win_msg_send);
	}

	u->win_msg_send = create_win_msg_send();
	
	w = lookup_widget(u->win_msg_send, "label_to");
	txt = g_strdup_printf("Do: <b>%s</b>", u->name);
	gtk_label_set_markup(GTK_LABEL(w), txt);
	g_free(txt);
	
	if(text != NULL) {
		w = lookup_widget(u->win_msg_send, "text");
		text_view_set_text(GTK_TEXT_VIEW(w), text);
	}

	w = lookup_widget(u->win_msg_send, "button_send");
	g_signal_connect(G_OBJECT(w), "clicked", G_CALLBACK(send_button_cb), u);
	w = lookup_widget(u->win_msg_send, "button_close");
	g_signal_connect(G_OBJECT(w), "clicked",
		G_CALLBACK(close_button_cb), u);
	g_signal_connect(G_OBJECT(u->win_msg_send), "destroy_event",
		G_CALLBACK(close_button_cb), u);

	gtk_widget_show_all(u->win_msg_send);
}

static void send_button_cb(GtkWidget *widget, gpointer user)
{
	gchar *text;
	GtkWidget *w;
	User *u = (User *)user;

	w = lookup_widget(u->win_msg_send, "text");

	text = text_view_get_text(GTK_TEXT_VIEW(w));

	o2_send_msg(u->id, text);
	
	g_free(text);

	gtk_widget_destroy(u->win_msg_send);
	u->win_msg_send = NULL;
}

static void close_button_cb(GtkWidget *widget, gpointer user)
{
	User *u = (User *)user;

	printf("u->win_msg_send: %p\n", u->win_msg_send);

	gtk_widget_hide(u->win_msg_send);
	gtk_widget_destroy(u->win_msg_send);
	u->win_msg_send = NULL;
}

