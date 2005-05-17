
#include "win_main.h"
#include "win_desc.h"
#include "win_msg_send.h"
#include "win_edit.h"
#include "win_prefs.h"
#include "interface.h"
#include "support.h"
#include "prefs.h"
#include "dock.h"
#include "emotes.h"
#include "tlen.h"
#include "misc.h"

GtkWidget *win_main = NULL;
/* model dla drzewka */
static GtkTreeStore *model = NULL;

static GtkMenu *status_menu = NULL;
static GtkMenu *userlist_menu = NULL;
static GtkMenu *main_menu = NULL;

static GtkTooltips *tips = NULL;

static gboolean main_win_iconified = FALSE;
static gboolean main_win_hidden = FALSE;

static GtkWidget **status_icons;

static void create_model();
static void create_view();
static gboolean status_icons_load();
static void create_status_menu();
static void create_userlist_menu();
static void create_main_menu();

/* XXX: wszystko to w opcje, albo z gtk theme braæ */
static const gchar *desc_row_color = "#f2f2f2";
static gchar *avail_color = "#000000";
static gchar *unavail_color = "#555555";
/* - kolor
   - nazwa
   - opis
   */
static const gchar *desc_fmt = 
" <span foreground=\"%s\">%s\n <i><span size=\"smaller\">%s</span></i></span>";
static const gchar *no_desc_fmt = 
" <span foreground=\"%s\">%s</span>";

static GtkWidget *expander_open;
static GtkWidget *expander_closed;
static GdkPixbuf *expander_open_pixbuf;
static GdkPixbuf *expander_closed_pixbuf;

static gchar *saved_desc = NULL;
static gboolean is_auto_away = FALSE;
static guint saved_status;
static gint autoaway_timer_id = -1;

/* funkcja sortujaca */
static gint sort_func(GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b,
	gpointer user_data);

/* callbacki */
static void tree_row_activated_cb(GtkTreeView *, GtkTreePath *,
	GtkTreeViewColumn *, gpointer);
static gboolean button_clicked_cb(GtkWidget *w, GdkEvent *ev);
static gboolean delete_win_cb(GtkWidget *w, gpointer data);
static gboolean window_state_cb(GtkWidget *w, GdkEvent *ev);
/* dla menu */
static gint popup_menu_cb(GtkWidget *menu, GdkEvent *event);
static void status_menu_item_cb(gchar *index);
static void userlist_menu_item_cb(gchar *index);
static void main_menu_item_cb(gchar *index);

/* cell renderer */
static void text_data_function(GtkTreeViewColumn *col, GtkCellRenderer *r,
	GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data);
static void pixbuf_data_function(GtkTreeViewColumn *col, GtkCellRenderer *r,
	GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data);

/* selection func */
static gboolean selection_func(GtkTreeSelection *selection, GtkTreeModel *m,
	GtkTreePath *path, gboolean selected, gpointer user_data);
/* ustawiane w powyzszym */
static gboolean is_user_selected = FALSE;

/* miganie ikonka statusu przy laczeniu */
static gint timeout_id;
static gboolean icon_blink(gpointer data);

static gboolean autoaway_timeout_cb(gpointer data);

static void quit_app_cb();

enum ColumnType {
	COLUMN_TEXT = 0,
	COLUMN_PTR,
	COLUMN_TYPE,
	NUM_COLS
};

#define TYPE_GROUP 0
#define TYPE_USER 1

GtkWidget * win_main_create()
{
	gchar *s;
	
	g_assert(win_main == NULL);
	win_main = create_win_main();
	g_assert(win_main != NULL);

	s = g_strdup_printf("glen (%s)", pref_tlen_id);
	gtk_window_set_title(GTK_WINDOW(win_main), s);
	g_free(s);
	
	tips = gtk_tooltips_new();

	create_model();
	create_view();

	/* stworz grupe "kontakty") */
	//(void)win_main_get_group(NULL);

	if(status_icons_load() == FALSE) {
		/* XXX: okienko */
		g_warning("Nie mozna zaladowac ikon. Blad instalacji?");
		return NULL;
	}

	create_status_menu();
	create_userlist_menu();
	create_main_menu();

	expander_open = create_pixmap(win_main, "glen/expander-open.png");
	expander_closed = create_pixmap(win_main, "glen/expander-closed.png");
	expander_open_pixbuf = gtk_image_get_pixbuf(GTK_IMAGE(expander_open));
	expander_closed_pixbuf =
		gtk_image_get_pixbuf(GTK_IMAGE(expander_closed));

	g_signal_connect(G_OBJECT(win_main), "delete_event",
		G_CALLBACK(delete_win_cb), NULL);
	g_signal_connect(G_OBJECT(win_main), "window_state_event",
		G_CALLBACK(window_state_cb), NULL);

	saved_status = o2_get_status(); 
	win_main_reset_autoaway_timer();
	
	return win_main;
}

void win_main_quit()
{
	quit_app_cb();
}

void win_main_user_add(User *user)
{
	Group *g;
	GtkTreePath *path, *parent_path;
	GtkTreeIter iter, parent;

	g_assert(user != NULL);
	
	user->icon_pixbuf = get_status_icon_pixbuf(user->status);

	g = user->group;
	/* jesli grupa nie jest dodana do drzewka, dodaj ja teraz */
	if(g->row == NULL) {
		g_debug("grupy %s nie ma w tree, dodaje", g->name);
		win_main_group_add(g);
	}
	
	parent_path = gtk_tree_row_reference_get_path(g->row);
	gtk_tree_model_get_iter(GTK_TREE_MODEL(model), &parent, parent_path);

	gtk_tree_store_append(model, &iter, &parent);
	gtk_tree_store_set(model, &iter, COLUMN_TEXT, user->name, COLUMN_PTR,
		user, COLUMN_TYPE, TYPE_USER, -1);
	path = gtk_tree_model_get_path(GTK_TREE_MODEL(model), &iter);

	user->row = gtk_tree_row_reference_new(GTK_TREE_MODEL(model), path);

	gtk_tree_path_free(path);
	gtk_tree_path_free(parent_path);

	/* update opisow grup itd */
	win_main_user_update(user);
}

void win_main_user_update(User *user)
{
	GtkTreePath *path;
	GtkTreeIter iter;
	gchar *color;
	Group *g;
	gchar *desc;
	
	g_assert(user != NULL);

	g = user->group;

	user->icon_pixbuf = get_status_icon_pixbuf(user->status);
	
	g_free(user->text);

	if(o2_status_is_avail(user->status))
		color = avail_color;
	else
		color = unavail_color;

	if(user->desc != NULL) {
		desc = g_markup_escape_text(user->desc, -1);
		user->text = g_strdup_printf(desc_fmt, color, user->name, desc);
		g_free(desc);
	} else {
		user->text = g_strdup_printf(no_desc_fmt, color, user->name);
	}

	path = gtk_tree_row_reference_get_path(user->row);
	gtk_tree_model_get_iter(GTK_TREE_MODEL(model), &iter, path);
	gtk_tree_model_row_changed(GTK_TREE_MODEL(model), path, &iter);
	gtk_tree_path_free(path);
	
	win_main_group_update(user->group);
	win_main_update_all();

	/* ustaw tytul okienka z rozmowa, jesli istnieje */
	win_chat_update_title(user);
}

void win_main_user_set_group(User *u, Group *g)
{
	Group *z;
	GtkTreeIter iter, parent;
	GtkTreePath *path, *parent_path;

	if(u->group == g) {
		printf("win_main_user_set_group: nie zmieniam dla %s\n",
			u->id);
		return;
	}

	win_main_user_remove(u);
	
	/* jesli usuniemy ostatniego usera w grupie trzeba usnac ja tez z
	 * modelu */
	z = u->group;
	if(z->child_count == 1/* && 
		g_strcasecmp(u->group->name, "Kontakty")*/ != 0) {
		printf("Usuwam grupe \"%s\"\n", z->name);
		g = u->group;
		win_main_group_remove(z);
		z = NULL;
	}

	if(g->row == NULL)
		win_main_group_add(g);

	parent_path = gtk_tree_row_reference_get_path(g->row);
	gtk_tree_model_get_iter(GTK_TREE_MODEL(model), &parent, parent_path);
	
	gtk_tree_store_append(model, &iter, &parent);
	gtk_tree_store_set(model, &iter, COLUMN_TEXT, u->name, COLUMN_PTR,
		u, COLUMN_TYPE, TYPE_USER, -1);
	path = gtk_tree_model_get_path(GTK_TREE_MODEL(model), &iter);
	
	u->row = gtk_tree_row_reference_new(GTK_TREE_MODEL(model), path);
	gtk_tree_path_free(path);
	gtk_tree_path_free(parent_path);

	if(z != NULL)
		win_main_group_update(z);

	win_main_group_update(g);
	win_main_update_all();
}

void win_main_user_remove(User *u)
{
	GtkTreePath *path;
	GtkTreeIter iter;

	g_assert(u->row != NULL);
	
	path = gtk_tree_row_reference_get_path(u->row);
	gtk_tree_model_get_iter(GTK_TREE_MODEL(model), &iter, path);
	gtk_tree_store_remove(GTK_TREE_STORE(model), &iter);

	gtk_tree_path_free(path);
	gtk_tree_row_reference_free(u->row);
	
	g_free(u->text);
	
	u->row = NULL;
	u->text = NULL;
}

void win_main_update_all()
{
	gtk_widget_queue_draw(win_main);
	/* robimy to tutaj zeby wymusic ponowne sortowanie */
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(model),
		COLUMN_TYPE, sort_func, NULL, NULL);
}

void win_main_expand_all()
{
	GtkWidget *treeview;

	treeview = lookup_widget(win_main, "user_tree");

	gtk_tree_view_expand_all(GTK_TREE_VIEW(treeview));
}

gboolean win_main_is_iconified()
{
	return main_win_iconified;
}

void win_main_toggle_hidden()
{
	if(main_win_hidden == TRUE) {
		gtk_widget_show(win_main);
		gtk_widget_grab_focus(win_main);
	} else {
		gtk_widget_hide(win_main);
	}

	main_win_hidden = !main_win_hidden;
}

void win_main_group_add(Group *group)
{
	GtkTreeIter top;
	GtkTreePath *path;

	g_assert(group->row == NULL);

	gtk_tree_store_append(model, &top, NULL);
	gtk_tree_store_set(model, &top, COLUMN_TEXT, group->name,
		COLUMN_PTR, group,
		COLUMN_TYPE, TYPE_GROUP, -1);

	path = gtk_tree_model_get_path(GTK_TREE_MODEL(model), &top);
	group->row = gtk_tree_row_reference_new(GTK_TREE_MODEL(model), path);
	gtk_tree_path_free(path);
}

void win_main_group_remove(Group *g)
{
	GtkTreeIter iter;
	GtkTreePath *path;

	g_assert(g->row != NULL);

	path = gtk_tree_row_reference_get_path(g->row);
	gtk_tree_model_get_iter(GTK_TREE_MODEL(model), &iter, path);
	gtk_tree_store_remove(GTK_TREE_STORE(model), &iter);
	
	gtk_tree_path_free(path);
	gtk_tree_row_reference_free(g->row);
	
	g_free(g->text);

	g->text = NULL;
	g->row = NULL;
}

void win_main_group_update(Group *g)
{
	if(g->text != NULL)
		g_free(g->text);

	g->text = g_strdup_printf("<b>%s <span color=\"#bbbbbb\">"
		" (%i/%i)</span></b>", g->name, g->child_active_count,
		g->child_count);
}

void win_main_set_status(guint status, const gchar *desc)
{
	GtkWidget *w, *img;
	GdkPixbuf *pb;
	gchar *p, *s;

	w = lookup_widget(win_main, "user_tree");
	if(status == TLEN_PRESENCE_UNAVAILABLE)// {
		win_main_clear();
//		gtk_tree_view_set_model(GTK_TREE_VIEW(w), NULL);
//	} else {
//		gtk_tree_view_set_model(GTK_TREE_VIEW(w),
//			GTK_TREE_MODEL(model));
//	}

	w = lookup_widget(win_main, "status_btn");
	img = lookup_widget(win_main, "image_status_button");
	pb = get_status_icon_pixbuf(status);
	
	gtk_window_set_icon(GTK_WINDOW(win_main), pb);
	gtk_image_set_from_pixbuf(GTK_IMAGE(img), pb);

	p = (gchar *)get_status_desc(status);

	if(desc != NULL && strlen(desc) > 0)
		s = g_strdup_printf("%s: %s", p, desc);
	else
		s = g_strdup_printf("%s", p);

	g_free(p);

	gtk_tooltips_set_tip(tips, w, s, NULL);
	g_free(s);

	dock_set_status(status, desc);
}

void win_main_start_icon_blinking(guint status)
{
	gchar *s;
	GtkWidget *w;

	timeout_id = g_timeout_add(750, icon_blink, (gpointer)status);

	w = lookup_widget(win_main, "status_btn");
	
	s = toutf("£±czenie...");
	gtk_tooltips_set_tip(tips, w, s, NULL);
	
	g_free(s);
}

void win_main_stop_icon_blinking()
{
	/*
	GtkWidget *w, *img;
	GdkPixbuf *pb;
	guint status;

	status = o2_get_status();
	pb = get_status_icon_pixbuf(status);

	w = lookup_widget(win_main, "status_btn");
	img = lookup_widget(w, "image_status_button");
	
	gtk_window_set_icon(GTK_WINDOW(win_main), pb);
	gtk_image_set_from_pixbuf(GTK_IMAGE(img), pb);

	g_time
	*/
}

void win_main_reset_autoaway_timer()
{
	guint status;
	
	if(autoaway_timer_id> 0) {
		gtk_timeout_remove(autoaway_timer_id);
		autoaway_timer_id = -1;
	}

	if(o2_is_connected() == FALSE) {
		is_auto_away = FALSE;
		return;
	}

	if(is_auto_away == TRUE) {
		win_main_set_status(saved_status, saved_desc);
		g_free(saved_desc);
		saved_desc = NULL;
		is_auto_away = FALSE;
	}
	
	status = o2_get_status();
	if(pref_status_auto_away == FALSE ||
		status == TLEN_PRESENCE_AWAY ||
		status == TLEN_PRESENCE_INVISIBLE ||
		status == TLEN_PRESENCE_EXT_AWAY ||
		is_auto_away == TRUE)
		return;

	autoaway_timer_id = gtk_timeout_add(pref_status_auto_away_time * 60 *
		1000,
		autoaway_timeout_cb, 0);
}

void win_main_clear()
{
	GSList *l;

	l = user_get_list();

	while(l != NULL) {
		printf("win_main_clear: removing user %s\n",
			((User *)l->data)->id);
		win_main_user_remove((User *)l->data);
		l = user_remove((User *)l->data);
	}

	l = group_get_list(); 
	
	while(l != NULL) {
		win_main_group_remove((Group *)l->data);
		l = group_remove((Group *)l->data);
	}
			
	gtk_tree_store_clear(GTK_TREE_STORE(model));
}

static gboolean autoaway_timeout_cb(gpointer data)
{
	gchar buf[1024];

	if(data == 0) {
		saved_status = o2_get_status();
		saved_desc = g_strdup(o2_get_desc());
		if(pref_status_auto_away_desc) {
			/* ugly */
			snprintf(buf, sizeof(buf), "%s%s[%s %s]",
				(saved_desc == NULL ? "" : saved_desc),
				(saved_desc == NULL ? "" : " "),
				get_time(NULL), get_date(NULL));
		} else
			strncpy(buf, saved_desc, strlen(saved_desc));
		
		win_main_set_status(TLEN_PRESENCE_AWAY, buf);
		o2_set_status(TLEN_PRESENCE_AWAY, buf);
		
		autoaway_timer_id = gtk_timeout_add(
			pref_status_auto_bbl_time * 60 * 1000, 
			autoaway_timeout_cb, (gpointer)1);
		is_auto_away = TRUE;
	} else {
		snprintf(buf, sizeof(buf), "%s", o2_get_desc());
		win_main_set_status(TLEN_PRESENCE_EXT_AWAY, buf);
		o2_set_status(TLEN_PRESENCE_EXT_AWAY, buf);
	}
		
	return FALSE;
}


GtkWidget *get_status_icon(guint status)
{
	guint i = 0;

	switch(status) {
	case TLEN_PRESENCE_AVAILABLE:
		i = 0;
		break;
	case TLEN_PRESENCE_AWAY:
		i = 1;
		break;
	case TLEN_PRESENCE_DND:
		i = 2;
		break;
	case TLEN_PRESENCE_EXT_AWAY:
		i = 3;
		break;
	case TLEN_PRESENCE_UNAVAILABLE:
		i = 4;
		break;
	case TLEN_PRESENCE_CHATTY:
		i = 5;
		break;
	case TLEN_PRESENCE_INVISIBLE:
		i = 6;
		break;
	}

	return status_icons[i];
}

GtkWidget *get_status_icon_new(guint status)
{
	return gtk_image_new_from_pixbuf(get_status_icon_pixbuf(status));
}

GdkPixbuf *get_status_icon_pixbuf(guint status)
{
	GtkWidget *i;

	i = get_status_icon(status);

	return gtk_image_get_pixbuf(GTK_IMAGE(i));
}

void quit_app_cb()
{
	if(pref_status_save_desc) {
		if(o2_get_desc() == NULL)
			pref_status_desc = "";
		else
			pref_status_desc = g_strdup(o2_get_desc());
	}

	if(pref_status_save_status == TRUE)
		pref_status_saved = o2_get_status();

	if(o2_is_connected())
		o2_set_status(TLEN_PRESENCE_UNAVAILABLE, NULL);

	prefs_save();
	emote_unload_set();
	gtk_main_quit();
}

static void create_model()
{
	model = gtk_tree_store_new(NUM_COLS, G_TYPE_STRING,
		G_TYPE_POINTER, G_TYPE_INT);
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(model),
		COLUMN_TYPE, GTK_SORT_ASCENDING);
}

static void create_view()
{
	GtkTreeView *treeview;
	GtkTreeViewColumn *col;
	GtkCellRenderer *renderer;
	GtkTreeSelection *sel;

	treeview = GTK_TREE_VIEW(lookup_widget(win_main, "user_tree"));
	g_assert(treeview != NULL);

	/* wspanialy sposob na schowanie expandera */
	col = gtk_tree_view_column_new();
	gtk_tree_view_append_column(treeview, col);
	gtk_tree_view_column_set_visible(col, FALSE);
	gtk_tree_view_set_expander_column(treeview, col);
	
	col = gtk_tree_view_column_new();
	gtk_tree_view_append_column(treeview, col);

	renderer = gtk_cell_renderer_pixbuf_new();
	gtk_tree_view_column_pack_start(col, renderer, FALSE);
	gtk_tree_view_column_set_cell_data_func(col, renderer,
		pixbuf_data_function, NULL, NULL);
	
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(col, renderer,
		text_data_function, NULL, NULL);

	gtk_tree_view_set_model(treeview, GTK_TREE_MODEL(model));
	g_object_unref(model);

	sel = gtk_tree_view_get_selection(treeview);
	/* zeby uniknac lookup_widget przekazujemy treeview w user_data */
	gtk_tree_selection_set_select_function(sel, selection_func,
		(gpointer)treeview, NULL);
	g_signal_connect(G_OBJECT(treeview), "button_release_event",
		G_CALLBACK(button_clicked_cb), NULL);

	g_signal_connect(G_OBJECT(treeview), "row-activated",
		G_CALLBACK(tree_row_activated_cb), NULL);
}

static void create_status_menu()
{
	GtkButton *b;
	GtkWidget *i;
	gchar *s;

	status_menu = GTK_MENU(gtk_menu_new());
	g_assert(status_menu != NULL);

	b = GTK_BUTTON(lookup_widget(win_main, "status_btn"));
	g_assert(b != NULL);

	g_signal_connect_swapped(G_OBJECT(b), "event",
		G_CALLBACK(popup_menu_cb), status_menu);

	s = toutf("_Dostêpny");
	i = gtk_image_menu_item_new_with_mnemonic(s);
	g_free(s);
	gtk_menu_shell_append(GTK_MENU_SHELL(status_menu), i);
	g_signal_connect_swapped(G_OBJECT(i), "activate",
		G_CALLBACK(status_menu_item_cb), "1");
	gtk_widget_show(i);
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(i),
		get_status_icon_new(TLEN_PRESENCE_AVAILABLE));
	
	s = toutf("_Porozmawiajmy");
	i = gtk_image_menu_item_new_with_mnemonic(s);
	g_free(s);
	gtk_menu_shell_append(GTK_MENU_SHELL(status_menu), i);
	g_signal_connect_swapped(G_OBJECT(i), "activate",
		G_CALLBACK(status_menu_item_cb), "2");
	gtk_widget_show(i);
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(i),
		get_status_icon_new(TLEN_PRESENCE_CHATTY));

	s = toutf("_Jestem zajêty");
	i = gtk_image_menu_item_new_with_mnemonic(s);
	g_free(s);
	gtk_menu_shell_append(GTK_MENU_SHELL(status_menu), i);
	g_signal_connect_swapped(G_OBJECT(i), "activate",
		G_CALLBACK(status_menu_item_cb), "3");
	gtk_widget_show(i);
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(i),
		get_status_icon_new(TLEN_PRESENCE_DND));

	s = toutf("_Zaraz wracam");
	i = gtk_image_menu_item_new_with_mnemonic(s);
	g_free(s);
	gtk_menu_shell_append(GTK_MENU_SHELL(status_menu), i);
	g_signal_connect_swapped(G_OBJECT(i), "activate",
		G_CALLBACK(status_menu_item_cb), "4");
	gtk_widget_show(i);
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(i),
		get_status_icon_new(TLEN_PRESENCE_AWAY));

	s = toutf("_Wrócê pó¼niej");
	i = gtk_image_menu_item_new_with_mnemonic(s);
	g_free(s);
	gtk_menu_shell_append(GTK_MENU_SHELL(status_menu), i);
	g_signal_connect_swapped(G_OBJECT(i), "activate",
		G_CALLBACK(status_menu_item_cb), "5");
	gtk_widget_show(i);
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(i),
		get_status_icon_new(TLEN_PRESENCE_EXT_AWAY));

	s = toutf("_Niewidoczny");
	i = gtk_image_menu_item_new_with_mnemonic(s);
	g_free(s);
	gtk_menu_shell_append(GTK_MENU_SHELL(status_menu), i);
	g_signal_connect_swapped(G_OBJECT(i), "activate",
		G_CALLBACK(status_menu_item_cb), "6");
	gtk_widget_show(i);
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(i),
		get_status_icon_new(TLEN_PRESENCE_INVISIBLE));

	s = toutf("Niedostêpn_y");
	i = gtk_image_menu_item_new_with_mnemonic(s);
	g_free(s);
	gtk_menu_shell_append(GTK_MENU_SHELL(status_menu), i);
	g_signal_connect_swapped(G_OBJECT(i), "activate",
		G_CALLBACK(status_menu_item_cb), "7");
	gtk_widget_show(i);
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(i),
		get_status_icon_new(TLEN_PRESENCE_UNAVAILABLE));

	/* separator */
	i = gtk_menu_item_new();
	gtk_container_add(GTK_CONTAINER(status_menu), i);
	gtk_widget_set_sensitive(i, FALSE);
	gtk_widget_show(i);

	s = toutf("_Ustaw stan opisowy");
	i = gtk_menu_item_new_with_mnemonic(s);
	g_free(s);
	gtk_menu_shell_append(GTK_MENU_SHELL(status_menu), i);
	g_signal_connect_swapped(G_OBJECT(i), "activate",
		G_CALLBACK(status_menu_item_cb), "8");
	gtk_widget_show(i);
	
}

static void create_userlist_menu()
{
	gchar *s;
	GtkWidget *i;
	GtkAccelGroup *accel;
	
	userlist_menu = GTK_MENU(gtk_menu_new());
	g_assert(userlist_menu != NULL);

	accel = gtk_accel_group_new();
	
	s = toutf("Wiadomo¶æ");
	i = gtk_image_menu_item_new_with_label(s);
	g_free(s);
	gtk_menu_shell_append(GTK_MENU_SHELL(userlist_menu), i);
	g_signal_connect_swapped(G_OBJECT(i), "activate",
		G_CALLBACK(userlist_menu_item_cb), "1");
	gtk_widget_add_accelerator(i, "activate", accel, GDK_M,
		GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
	gtk_widget_show(i);
	/* XXX: ikonki... */
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(i),
		gtk_image_new_from_stock("gnome-stock-mail-snd",
			GTK_ICON_SIZE_MENU));

	s = toutf("Rozmowa");
	i = gtk_image_menu_item_new_with_label(s);
	g_free(s);
	gtk_menu_shell_append(GTK_MENU_SHELL(userlist_menu), i);
	g_signal_connect_swapped(G_OBJECT(i), "activate",
		G_CALLBACK(userlist_menu_item_cb), "2");
	gtk_widget_add_accelerator(i, "activate", accel, GDK_C,
		GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
	gtk_widget_show(i);
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(i),
		gtk_image_new_from_stock("gnome-stock-mail",
			GTK_ICON_SIZE_MENU));

	/* separator */
	i = gtk_menu_item_new();
	gtk_container_add(GTK_CONTAINER(userlist_menu), i);
	gtk_widget_set_sensitive(i, FALSE);
	gtk_widget_show(i);

	s = toutf("Edytuj kontakt");
	i = gtk_image_menu_item_new_with_label(s);
	g_free(s);
	gtk_menu_shell_append(GTK_MENU_SHELL(userlist_menu), i);
	g_signal_connect_swapped(G_OBJECT(i), "activate",
		G_CALLBACK(userlist_menu_item_cb), "3");
	gtk_widget_add_accelerator(i, "activate", accel, GDK_E,
		GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
	gtk_widget_show(i);
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(i),
		gtk_image_new_from_stock("gtk-edit",
			GTK_ICON_SIZE_MENU));

	s = toutf("Usuñ kontakt");
	i = gtk_image_menu_item_new_with_label(s);
	g_free(s);
	gtk_menu_shell_append(GTK_MENU_SHELL(userlist_menu), i);
	g_signal_connect_swapped(G_OBJECT(i), "activate",
		G_CALLBACK(userlist_menu_item_cb), "4");
	gtk_widget_add_accelerator(i, "activate", accel, GDK_Delete, 0,
		GTK_ACCEL_VISIBLE);
	gtk_widget_show(i);
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(i),
		gtk_image_new_from_stock("gtk-remove",
			GTK_ICON_SIZE_MENU));
	/* separator */
	i = gtk_menu_item_new();
	gtk_container_add(GTK_CONTAINER(userlist_menu), i);
	gtk_widget_set_sensitive(i, FALSE);
	gtk_widget_show(i);

	s = toutf("Skopiuj opis do schowka");
	i = gtk_image_menu_item_new_with_label(s);
	g_free(s);
	gtk_menu_shell_append(GTK_MENU_SHELL(userlist_menu), i);
	g_signal_connect_swapped(G_OBJECT(i), "activate",
		G_CALLBACK(userlist_menu_item_cb), "5");
	gtk_widget_show(i);
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(i),
		gtk_image_new_from_stock("gtk-copy",
			GTK_ICON_SIZE_MENU));

	gtk_window_add_accel_group(GTK_WINDOW(win_main), accel);
}

static void create_main_menu()
{
	gchar *s;
	GtkWidget *i;
	GtkAccelGroup *accel;
	GtkButton *b;
	
	main_menu = GTK_MENU(gtk_menu_new());
	g_assert(main_menu != NULL);

	b = GTK_BUTTON(lookup_widget(win_main, "menu_btn"));
	g_assert(b != NULL);

	g_signal_connect_swapped(G_OBJECT(b), "event",
		G_CALLBACK(popup_menu_cb), main_menu);
	
	accel = gtk_accel_group_new();
	
	s = toutf("Dodaj kontakt");
	i = gtk_image_menu_item_new_with_label(s);
	g_free(s);
	gtk_menu_shell_append(GTK_MENU_SHELL(main_menu), i);
	g_signal_connect_swapped(G_OBJECT(i), "activate",
		G_CALLBACK(main_menu_item_cb), "4");
	gtk_widget_add_accelerator(i, "activate", accel, GDK_A,
		GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
	gtk_widget_show(i);
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(i),
		gtk_image_new_from_stock("gtk-add",
			GTK_ICON_SIZE_MENU));
	
	i = gtk_menu_item_new();
	gtk_container_add(GTK_CONTAINER(main_menu), i);
	gtk_widget_set_sensitive(i, FALSE);
	gtk_widget_show(i);

	s = toutf("Ustawienia");
	i = gtk_image_menu_item_new_with_label(s);
	g_free(s);
	gtk_menu_shell_append(GTK_MENU_SHELL(main_menu), i);
	g_signal_connect_swapped(G_OBJECT(i), "activate",
		G_CALLBACK(main_menu_item_cb), "1");
	gtk_widget_add_accelerator(i, "activate", accel, GDK_P,
		GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
	gtk_widget_show(i);
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(i),
		gtk_image_new_from_stock("gtk-preferences",
			GTK_ICON_SIZE_MENU));

	i = gtk_menu_item_new();
	gtk_container_add(GTK_CONTAINER(main_menu), i);
	gtk_widget_set_sensitive(i, FALSE);
	gtk_widget_show(i);

	s = toutf("O programie");
	i = gtk_image_menu_item_new_with_label(s);
	g_free(s);
	gtk_menu_shell_append(GTK_MENU_SHELL(main_menu), i);
	g_signal_connect_swapped(G_OBJECT(i), "activate",
		G_CALLBACK(main_menu_item_cb), "2");
	gtk_widget_show(i);
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(i),
		gtk_image_new_from_stock("gtk-about",
			GTK_ICON_SIZE_MENU));

	s = toutf("Zakoñcz");
	i = gtk_image_menu_item_new_with_label(s);
	g_free(s);
	gtk_menu_shell_append(GTK_MENU_SHELL(main_menu), i);
	g_signal_connect_swapped(G_OBJECT(i), "activate",
		G_CALLBACK(main_menu_item_cb), "3");
	gtk_widget_show(i);
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(i),
		gtk_image_new_from_stock("gtk-quit",
			GTK_ICON_SIZE_MENU));

	gtk_window_add_accel_group(GTK_WINDOW(win_main), accel);
}

static gboolean status_icons_load()
{
	const gchar *files[] = {
		"glen/status-avail.png", "glen/status-away.png",
		"glen/status-dnd.png", "glen/status-bbl.png",
		"glen/status-unavail.png", "glen/status-chatty.png",
		"glen/status-invisible.png" };

	const guint num_files = sizeof(files)/sizeof(files[0]);
	guint i;

	status_icons = (GtkWidget **)g_malloc(sizeof(GtkWidget *) *
		num_files);

	for(i = 0; i < num_files; i++) {
		status_icons[i] = create_pixmap(NULL, files[i]);
		gtk_widget_show(status_icons[i]);
	}

	return TRUE;
}

static void tree_row_activated_cb(GtkTreeView *tv, GtkTreePath *path,
	GtkTreeViewColumn *col, gpointer user_data)
{
	GtkTreeIter iter;
	guint type;
	User *u = NULL;

	if(gtk_tree_model_get_iter(GTK_TREE_MODEL(model), &iter,
		path) == FALSE)
		return;
		
	gtk_tree_model_get(GTK_TREE_MODEL(model), &iter, COLUMN_TYPE,
		&type, COLUMN_PTR, &u, -1);

		if(type == TYPE_USER) {
		win_chat_get(u->id, TRUE, TRUE);
	}
}

static gboolean delete_win_cb(GtkWidget *widget, gpointer data)
{
	gtk_widget_hide(widget);
	main_win_hidden = TRUE;

	return TRUE;
}

static gboolean window_state_cb(GtkWidget *w, GdkEvent *ev)
{
	GdkEventWindowState *ws = (GdkEventWindowState *)ev;

	if(ws->new_window_state & GDK_WINDOW_STATE_ICONIFIED)
		main_win_iconified = TRUE;
	else
		main_win_iconified = FALSE;

	return TRUE;
}

static void text_data_function(GtkTreeViewColumn *col, GtkCellRenderer *r,
	GtkTreeModel *mod, GtkTreeIter *iter, gpointer user_data)
{
	User *user;
	void *ptr;
	Group *group;
	guint type;

	gtk_tree_model_get(mod, iter, COLUMN_TYPE, &type, COLUMN_PTR, &ptr, -1);
	if(type == TYPE_USER) {
		user = (User *)ptr;
		g_object_set(r, 
			//"font", "normal",
			//"text", NULL,
			"cell-background", (user->desc ? desc_row_color: NULL),
			"markup", user->text,
			NULL);
	} else { /* grupa*/
		gtk_tree_model_get(mod, iter, COLUMN_PTR, &group, -1);
		group = (Group *)ptr;
		g_object_set(r, 
			//"font", "bold",
			"cell-background", NULL,
			//"text", NULL,
			"markup", group->text,
			NULL);
	}
}

static void pixbuf_data_function(GtkTreeViewColumn *col, GtkCellRenderer *r,
	GtkTreeModel *mod, GtkTreeIter *iter, gpointer user_data)
{
	guint type;
	User *u;
	Group *g;
	void *ptr;
	GdkPixbuf *pb;

	gtk_tree_model_get(mod, iter, COLUMN_TYPE, &type, COLUMN_PTR, &ptr,
		-1);

	if(type == TYPE_GROUP) {
		g = (Group *)ptr;
		if(g->expanded == TRUE)
			pb = expander_open_pixbuf;
		else
			pb = expander_closed_pixbuf;
		
		g_object_set(r, "pixbuf", pb,
				"cell-background", NULL,
				NULL);
	} else {
		u = (User *)ptr;
		//g_object_set(r, "visible", TRUE, NULL);
		g_object_set(r,
			"pixbuf", u->icon_pixbuf,
			"cell-background", (u->desc ? desc_row_color: NULL),
			NULL);
	}
}

static gint sort_func(GtkTreeModel *m, GtkTreeIter *a, GtkTreeIter *b,
	gpointer userdata)
{
	gint ret;
	void *ptr1, *ptr2;
	gint type1;
	gint type2;

	ret = 0;

	gtk_tree_model_get(m, a, COLUMN_TYPE, &type1, COLUMN_PTR, &ptr1, -1);
	gtk_tree_model_get(m, b, COLUMN_TYPE, &type2, COLUMN_PTR, &ptr2, -1);

	if(type1 == type2) {
		if(type1 == TYPE_GROUP) {
			Group *g1 = (Group *)ptr1;
			Group *g2 = (Group *)ptr2;

			/* Grupa "Kontakty" zawsze na gorze, bez
			 * autoryzacji/nieznani na dole */
			if(g_strcasecmp(g1->name, "Kontakty") == 0)
				ret = -1;
			else if(g_strcasecmp(g2->name, "Kontakty") == 0)
				ret = 1;
			else if(g_strcasecmp(g1->name,
					GROUP_NAME_WAIT_AUTH) == 0 ||
				g_strcasecmp(g1->name, 
					GROUP_NAME_NO_AUTH) == 0)
				ret = 1;
			else if(g_strcasecmp(g2->name,
					GROUP_NAME_NO_AUTH) == 0 ||
				g_strcasecmp(g2->name,
					GROUP_NAME_WAIT_AUTH) == 0)
				ret = -1;
			else
				ret = g_strcasecmp(g1->name, g2->name);
		} else if(type1 == TYPE_USER) {
			User *u1 = (User *)ptr1;
			User *u2 = (User *)ptr2;
			gboolean b1 = o2_status_is_avail(u1->status);
			gboolean b2 = o2_status_is_avail(u2->status);
			
			if(b1 == b2)
				ret = g_strcasecmp(u1->name,
					u2->name);
			else if(b1 == TRUE && b2 == FALSE)
				ret = -1;
			else ret = 1;
		}
	}

	return ret;
}

/* XXX: Trzeba bedzie zamienic to na button_clicked, bo przy zmianie wierszy
 * klawiatura grupy sie zamykaja, a to chyba zle :} */
static gboolean selection_func(GtkTreeSelection *selection, GtkTreeModel *m,
	GtkTreePath *path, gboolean selected, gpointer user_data)
{
	GtkTreeIter it;
	gchar *ptr;
	Group *g;
	gint type;
	GtkTreeView *tv = (GtkTreeView *)user_data;

	if(gtk_tree_model_get_iter(m, &it, path)) {
		gtk_tree_model_get(m, &it, COLUMN_PTR, &ptr, COLUMN_TYPE,
			&type, -1);

		if(type == TYPE_GROUP && selected == FALSE) {
			g = (Group *)ptr;
			if(g->expanded == TRUE) {
				gtk_tree_view_collapse_row(tv, path);
			} else {
				gtk_tree_view_expand_row(tv, path, TRUE);
			}

			g->expanded = !g->expanded;

			is_user_selected = FALSE;

			return FALSE;
		}
	}

	is_user_selected = TRUE;

	return TRUE;
}

static gboolean button_clicked_cb(GtkWidget *w, GdkEvent *ev)
{
	GdkEventButton *eb = (GdkEventButton *)ev;

	if(eb->button == 3 && is_user_selected == TRUE) {
		gtk_menu_popup(userlist_menu, NULL, NULL, NULL, NULL,
			eb->button, eb->time);
	}

	return FALSE;
}

static gint popup_menu_cb(GtkWidget *w, GdkEvent *ev)
{
	GdkEventButton *eb = (GdkEventButton *)ev;
	GtkMenu *menu = GTK_MENU(w);

	if(ev->type != GDK_BUTTON_PRESS)
		return FALSE;
	
	gtk_menu_popup(menu, NULL, NULL, NULL, NULL,
		eb->button, eb->time);

	return TRUE;
}

static void status_menu_item_cb(gchar * i)
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

	if(status != -1)
		o2_set_status(status, NULL);
}

static void userlist_menu_item_cb(gchar *i)
{
	User *u;
	Group *g;
	GtkTreeSelection *sel;
	GtkWidget *w;
	GList *l;
	GtkTreeIter it;
	GtkTreeModel *mod;
	GtkClipboard *clip;
	GdkAtom atom;
	gchar *s, *p;

	w = lookup_widget(win_main, "user_tree");
	sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(w));
	if(sel == NULL) {
		puts("no sel");
		return;
	}

	if(gtk_tree_selection_count_selected_rows(sel) != 1)
		return;

	mod = GTK_TREE_MODEL(model);
	l = gtk_tree_selection_get_selected_rows(sel, &mod);

	gtk_tree_model_get_iter(GTK_TREE_MODEL(model), &it, (GtkTreePath *)l->data);
	gtk_tree_model_get(GTK_TREE_MODEL(model), &it, COLUMN_PTR, &u, -1);

	g_list_foreach(l, (GFunc)gtk_tree_path_free, NULL);
	g_list_free(l);
	
	if(g_strcasecmp(i, "1") == 0) /* Wiadomo¶æ */
		win_msg_send_show(u, NULL);
	else if(g_strcasecmp(i, "2") == 0) /* rozmowa */
		win_chat_get(u->id, TRUE, TRUE);
	else if(g_strcasecmp(i, "3") == 0) /* edytuj */
		win_edit_create(u);
	else if(g_strcasecmp(i, "4") == 0) { /* usun */
		s = g_strdup_printf(
			"Czy na pewno chcesz usun±æ u¿ytkownika <b>%s</b>?",
			u->id);
		p = toutf(s);
		w = yes_no_dialog(win_main, p);
		if(gtk_dialog_run(GTK_DIALOG(w)) == GTK_RESPONSE_YES) {
			g = u->group;
			win_main_user_remove(u);
			o2_contact_remove(u->id);
			user_remove(u);
			g->child_count--;
			if(g->child_count == 0) {
				win_main_group_remove(g);
				group_remove(g);
			}
		}
		gtk_widget_destroy(w);
		g_free(s);
		g_free(p);
	} else if(g_strcasecmp(i, "5") == 0) { /* skopiuj opis */
		atom = gdk_atom_intern("CLIPBOARD", FALSE);
		clip = gtk_clipboard_get(atom);

		if(u->desc != NULL)
			gtk_clipboard_set_text(clip, u->desc, strlen(u->desc));
		else
			gtk_clipboard_set_text(clip, "", 1);
	}
}

static void main_menu_item_cb(gchar *i)
{
	if(g_strcasecmp(i, "1") == 0) /* ustawienia */
		win_prefs_show();
	else if(g_strcasecmp(i, "2") == 0) { /* o programie */
		GtkWidget *about;
		about = create_win_about();
		g_signal_connect(G_OBJECT(about), "delete_event",
			G_CALLBACK(gtk_widget_destroy), NULL);
		g_signal_connect(G_OBJECT(about), "close",
			G_CALLBACK(gtk_widget_destroy), NULL);
		gtk_widget_show(about);
	} else if(g_strcasecmp(i, "3") == 0) /* wyjscie */
		quit_app_cb();
	else if(g_strcasecmp(i, "4") == 0) /* dodaj kontakt */
		win_edit_create(NULL);
}

static gboolean icon_blink(gpointer data)
{
	static gboolean on = TRUE;
	gboolean ret;
	guint status = (guint)data;
	GdkPixbuf *pb;
	GtkWidget *w, *img;

	ret = TRUE;

	if(on == FALSE) {
		pb = get_status_icon_pixbuf(TLEN_PRESENCE_UNAVAILABLE);
	} else {
		pb = get_status_icon_pixbuf(status);
		if(o2_is_connected())
			ret = FALSE;
	}

	w = lookup_widget(win_main, "status_btn");
	img = lookup_widget(w, "image_status_button");
	
	dock_set_status((on ? status : TLEN_PRESENCE_UNAVAILABLE), NULL);
	gtk_window_set_icon(GTK_WINDOW(win_main), pb);
	gtk_image_set_from_pixbuf(GTK_IMAGE(img), pb);

	on = !on;
	
	return ret;
}



