
#include <gnome.h>
#include "user.h"
#include "tlen.h"
#include "dialog_auth.h"
#include "misc.h"

extern GtkWidget *win_main;

struct dialog_auth_item {
	GtkWidget *dialog;
	gchar id[256];
};

/* trzymamy tu liste userow dla ktorych mamy otwarte okienka z prosba o
 * autoryzacje (prawdopodobnie w 99% bedzie to jeden element */
static GSList *dialog_list = NULL;

static void response_cb(GtkDialog *dialog, gint response, gpointer data);

void dialog_auth_create(const gchar *id)
{
	GtkWidget *dialog;
	GtkWidget *label;
	GtkWidget *hbox;
	GtkWidget *image;
	GtkWidget *button;

	gchar *s;
	GSList *i;

	struct dialog_auth_item *item = NULL;

	for(i = dialog_list; i != NULL; i = i->next) {
		item = i->data;
		if(g_strcasecmp(id, item->id) == 0)
			/* XXX: wyrzuc dialog na wierzch */
			return;
	}

	dialog = gtk_dialog_new();
	button = gtk_dialog_add_button(GTK_DIALOG(dialog), "_Zablokuj", 1);
	image = gtk_image_new_from_stock(GTK_STOCK_OPEN,
		GTK_ICON_SIZE_BUTTON);
	gtk_button_set_image(GTK_BUTTON(button), image);

	gtk_dialog_add_buttons(GTK_DIALOG(dialog),
		GTK_STOCK_NO, GTK_RESPONSE_NO,
		GTK_STOCK_YES, GTK_RESPONSE_YES, NULL);
	
	gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
	gtk_container_set_border_width(GTK_CONTAINER(dialog), 6);
	gtk_dialog_set_has_separator(GTK_DIALOG(dialog), FALSE);
	gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_YES);

	s = g_strdup_printf(
		"<span weight=\"bold\" size=\"larger\">"
		"%s chce dodać Cię do swojej listy</span>\n\n"
		"Czy wyrażasz na to zgodę? Możesz również zablokować tego "
		"użytkownika, aby uniemożliwić mu na stałe prośby o"
		" dodanie Ciebie do jego listy\n", id);

	hbox = gtk_hbox_new(FALSE, 12);

	label = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(label), s);
	gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
	//g_object_set(G_OBJECT(label), "yalign", 0, NULL);

	image = gtk_image_new_from_stock(GTK_STOCK_DIALOG_QUESTION,
		GTK_ICON_SIZE_DIALOG);
	//g_object_set(G_OBJECT(image), "yalign", 0, NULL);

	gtk_box_pack_start(GTK_BOX(hbox), image, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, TRUE, 0);
	
	gtk_box_set_spacing(GTK_BOX(GTK_DIALOG(dialog)->vbox), 12);
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), hbox);

	g_free(s);

	printf("auth_dialog dla '%s'\n", id);

	item = g_malloc0(sizeof(struct dialog_auth_item));
	item->dialog = dialog;
	strncpy(item->id, id, sizeof(item->id));

	dialog_list = g_slist_append(dialog_list, item);

	g_signal_connect(G_OBJECT(dialog), "response",
		G_CALLBACK(response_cb), item);

	gtk_widget_show_all(dialog);
}

static void response_cb(GtkDialog *dialog, gint response, gpointer data)
{
	struct dialog_auth_item *item = data;

	gtk_widget_hide(GTK_WIDGET(dialog));
	gtk_widget_destroy(GTK_WIDGET(dialog));

	if(response == GTK_RESPONSE_NO) {
		o2_contact_remove(item->id);
		o2_unsubscribe_request(item->id);
	} else if(response == GTK_RESPONSE_YES) {
		/* Nie robimy nic, poniewaz wczesniej potwierdzamy subscribe.
		 * Tak dziala protokol. Dzieki za wskazowke, whoami :) */
	} else if(response == 1) { /* zablokuj */
		printf("Jak juz mi sie zechce, zablokuje %s\n", item->id);
	}

	dialog_list = g_slist_remove(dialog_list, item);
	g_free(item);
}

