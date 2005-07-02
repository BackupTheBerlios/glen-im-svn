
#include "dock.h"
#include "win_main.h"
#include "win_desc.h"
#include "tlen.h"
#include "misc.h"

GtkWidget *dock_widget = NULL;
static GtkWidget *image;

static GtkMenu *menu;
static GtkTooltips *tips;

static void create_menu();

static gboolean clicked_cb(GtkWidget *, GdkEvent *);
static void menu_item_cb(gchar *index);

void dock_create()
{
	GtkWidget *eb; /* wrzucamy tu pixmape, zeby reagowala na
			    eventy takie jake klikniecie */
	dock_widget = GTK_WIDGET(egg_tray_icon_new("glen"));
	create_menu();

	eb = gtk_event_box_new();
	image = gtk_image_new();

	gtk_container_add(GTK_CONTAINER(eb), image);
	gtk_container_add(GTK_CONTAINER(dock_widget), eb);
	gtk_widget_show_all(dock_widget);

	g_signal_connect(G_OBJECT(dock_widget), "button_press_event",
		G_CALLBACK(clicked_cb), NULL);

	tips = gtk_tooltips_new();

	dock_set_status(TLEN_PRESENCE_UNAVAILABLE, NULL);
}

void dock_set_status(guint status, const gchar *desc)
{
	GdkPixbuf *pb;
	gchar *s, *p;

	pb = gtk_image_get_pixbuf(GTK_IMAGE(get_status_icon_new(status)));
	gtk_image_set_from_pixbuf(GTK_IMAGE(image), pb);

	p = (gchar *)get_status_desc(status);

	if(desc != NULL && strlen(desc) > 0)
		s = g_strdup_printf("glen - %s:\n%s", p, desc);
	else
		s = g_strdup_printf("glen - %s", p);

	g_free(p);

	//p = toutf(s);
	p = s;
	gtk_tooltips_set_tip(tips, dock_widget, p, NULL);
	
	g_free(s);
//	g_free(p);
}

static gboolean clicked_cb(GtkWidget *dock, GdkEvent *ev)
{
	GdkEventButton *eb = (GdkEventButton *)ev;

	if(eb->button == 1) {
		/*
		XXX: To sprawia problemy przy okienku na innym pulpicie
		if(win_main_is_iconified()) {
			gtk_window_deiconify(GTK_WINDOW(win_main));
			gtk_widget_grab_focus(win_main);
		} else { */
			/*if(GTK_WIDGET_VISIBLE(win_main))
				gtk_widget_hide(win_main);
			else {*/
				//gtk_widget_show(win_main);
			//	gtk_widget_grab_focus(win_main);
			//}
			win_main_toggle_hidden();
		//}
	} else if(eb->button == 3) {
		gtk_menu_popup(menu, NULL, NULL, NULL, NULL, eb->button,
			eb->time);
	}

	return TRUE;
}

static void create_menu()
{
	GtkWidget *i;
	gchar *s;

	menu = GTK_MENU(gtk_menu_new());
	g_assert(menu != NULL);

	s = toutf("_Dostêpny");
	i = gtk_image_menu_item_new_with_mnemonic(s);
	g_free(s);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), i);
	g_signal_connect_swapped(G_OBJECT(i), "activate",
		G_CALLBACK(menu_item_cb), "1");
	gtk_widget_show(i);
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(i),
		get_status_icon_new(TLEN_PRESENCE_AVAILABLE));
	
	s = toutf("_Porozmawiajmy");
	i = gtk_image_menu_item_new_with_mnemonic(s);
	g_free(s);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), i);
	g_signal_connect_swapped(G_OBJECT(i), "activate",
		G_CALLBACK(menu_item_cb), "2");
	gtk_widget_show(i);
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(i),
		get_status_icon_new(TLEN_PRESENCE_CHATTY));

	s = toutf("_Jestem zajêty");
	i = gtk_image_menu_item_new_with_mnemonic(s);
	g_free(s);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), i);
	g_signal_connect_swapped(G_OBJECT(i), "activate",
		G_CALLBACK(menu_item_cb), "3");
	gtk_widget_show(i);
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(i),
		get_status_icon_new(TLEN_PRESENCE_DND));

	s = toutf("_Zaraz wracam");
	i = gtk_image_menu_item_new_with_mnemonic(s);
	g_free(s);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), i);
	g_signal_connect_swapped(G_OBJECT(i), "activate",
		G_CALLBACK(menu_item_cb), "4");
	gtk_widget_show(i);
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(i),
		get_status_icon_new(TLEN_PRESENCE_AWAY));

	s = toutf("_Wrócê pó¼niej");
	i = gtk_image_menu_item_new_with_mnemonic(s);
	g_free(s);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), i);
	g_signal_connect_swapped(G_OBJECT(i), "activate",
		G_CALLBACK(menu_item_cb), "5");
	gtk_widget_show(i);
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(i),
		get_status_icon_new(TLEN_PRESENCE_EXT_AWAY));

	s = toutf("_Niewidoczny");
	i = gtk_image_menu_item_new_with_mnemonic(s);
	g_free(s);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), i);
	g_signal_connect_swapped(G_OBJECT(i), "activate",
		G_CALLBACK(menu_item_cb), "6");
	gtk_widget_show(i);
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(i),
		get_status_icon_new(TLEN_PRESENCE_INVISIBLE));

	s = toutf("Niedostêpn_y");
	i = gtk_image_menu_item_new_with_mnemonic(s);
	g_free(s);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), i);
	g_signal_connect_swapped(G_OBJECT(i), "activate",
		G_CALLBACK(menu_item_cb), "7");
	gtk_widget_show(i);
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(i),
		get_status_icon_new(TLEN_PRESENCE_UNAVAILABLE));

	/* separator */
	i = gtk_menu_item_new();
	gtk_container_add(GTK_CONTAINER(menu), i);
	gtk_widget_set_sensitive(i, FALSE);
	gtk_widget_show(i);

	s = toutf("_Ustaw stan opisowy");
	i = gtk_menu_item_new_with_mnemonic(s);
	g_free(s);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), i);
	g_signal_connect_swapped(G_OBJECT(i), "activate",
		G_CALLBACK(menu_item_cb), "8");
	gtk_widget_show(i);
	
	/* separator */
	i = gtk_menu_item_new();
	gtk_container_add(GTK_CONTAINER(menu), i);
	gtk_widget_set_sensitive(i, FALSE);
	gtk_widget_show(i);
	
	s = toutf("_Koniec");
	i = gtk_menu_item_new_with_mnemonic(s);
	g_free(s);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), i);
	g_signal_connect_swapped(G_OBJECT(i), "activate",
		G_CALLBACK(menu_item_cb), "9");
	gtk_widget_show(i);
}

static void menu_item_cb(gchar * i)
{
	gint status = -1;
		
	if(g_strcasecmp(i, "1") == 0)
		status = TLEN_PRESENCE_AVAILABLE;
	else if(g_strcasecmp(i, "2") == 0)
		status = TLEN_PRESENCE_CHATTY;
	else if(g_strcasecmp(i, "3") == 0)
		status = TLEN_PRESENCE_DND;
	else if(g_strcasecmp(i, "4") == 0)
		status = TLEN_PRESENCE_AWAY;
	else if(g_strcasecmp(i, "5") == 0)
		status = TLEN_PRESENCE_EXT_AWAY;
	else if(g_strcasecmp(i, "6") == 0)
		status = TLEN_PRESENCE_INVISIBLE;
	else if(g_strcasecmp(i, "7") == 0)
		status = TLEN_PRESENCE_UNAVAILABLE;
	else if(g_strcasecmp(i, "8") == 0) /* ustaw stan opisowy */
		win_desc_show();
	else if(g_strcasecmp(i, "9") == 0) { /* koniec */
		win_main_quit();
	}

	if(status != -1)
		o2_set_status(status, NULL);
}
