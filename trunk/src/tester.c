
#include "win_main.h"
#include "win_chat.h"
#include "interface.h"
#include "tlen.h"
#include "support.h"
#include "dock.h"
#include "misc.h"
#include <string.h>

void but1(GtkWidget *, GdkEvent *);
void but2(GtkWidget *, GdkEvent *);
void but3(GtkWidget *, GdkEvent *);
void but4(GtkWidget *, GdkEvent *);

extern GtkWidget * dock_widget;

GtkWindow * tester_create()
{
	GtkWidget *w;
	GtkWidget *h;
	GtkWidget *b;

	w = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(w), "Tester");

	h = gtk_hbox_new(FALSE, 5);
	
	gtk_container_set_border_width(GTK_CONTAINER(w), 10);
	gtk_container_add(GTK_CONTAINER(w), h);

	b = gtk_button_new_with_label("  1  ");
	g_signal_connect(G_OBJECT(b), "clicked", G_CALLBACK(but1), NULL);
	gtk_box_pack_start(GTK_BOX(h), b, TRUE, TRUE, 0);
	b = gtk_button_new_with_label("  2  ");
	g_signal_connect(G_OBJECT(b), "clicked", G_CALLBACK(but2), NULL);
	gtk_box_pack_start(GTK_BOX(h), b, TRUE, TRUE, 0);
	b = gtk_button_new_with_label("  3  ");
	g_signal_connect(G_OBJECT(b), "clicked", G_CALLBACK(but3), NULL);
	gtk_box_pack_start(GTK_BOX(h), b, TRUE, TRUE, 0);
	b = gtk_button_new_with_label("  4  ");
	g_signal_connect(G_OBJECT(b), "clicked", G_CALLBACK(but4), NULL);
	gtk_box_pack_start(GTK_BOX(h), b, TRUE, TRUE, 0);

	gtk_widget_show_all(GTK_WIDGET(w));

	return GTK_WINDOW(w);
}

void but1(GtkWidget *w, GdkEvent *e)
{
}

void but2(GtkWidget *w, GdkEvent *e)
{
}

void but3(GtkWidget *w, GdkEvent *e)
{
}

void but4(GtkWidget *w, GdkEvent *e)
{
}
