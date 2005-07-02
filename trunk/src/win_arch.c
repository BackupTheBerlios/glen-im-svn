
#include <gnome.h>
#include "arch.h"
#include "misc.h"
#include "support.h"
#include "glen_html.h"
#include "win_arch.h"
#include "interface.h"

static GtkWidget *win_arch = NULL;
static GtkTreeStore *users_model = NULL;
static GtkTreeStore *list_model = NULL;

/* Ustawia wyswietlana historie na danego usera */
static void win_arch_set_user(const char *id);

static void create_models(void);

void win_arch_show(const char *id)
{
	GtkWidget *scroll = NULL;
	GtkWidget *html = NULL;
	GtkWidget *tree = NULL;
	GtkTreeViewColumn *column = NULL;
	GtkCellRenderer *renderer = NULL;

	if (win_arch != NULL) {
		gtk_window_present(GTK_WINDOW(win_arch));

		if (id != NULL) {
			win_arch_set_user(id);
		}

		return;
	}

	win_arch = create_win_arch();
	g_assert(win_arch != NULL);

	/* Wstaw GtkHTML do scrollview na dole */
	scroll = lookup_widget(GTK_WIDGET(win_arch), "scroll");
	g_assert(scroll != NULL);

	html = glen_html_new();
	g_assert(html != NULL);

	gtk_widget_set_name(GTK_WIDGET(html), "html");
	gtk_widget_set(GTK_WIDGET(html), "can-focus", FALSE, NULL);
	
	gtk_container_add(GTK_CONTAINER(scroll), GTK_WIDGET(html));

	/* Teraz zajmiemy siê drzewkami */
	tree = lookup_widget(GTK_WIDGET(win_arch), "tree_users");
	g_assert(tree != NULL);

	/* Jedna kolumna, string */
	users_model = gtk_tree_store_new(1, G_TYPE_STRING);
	g_assert(users_model != NULL);

	/* Dodaj kolumnê z rendererem wy¶wietlaj±cym nazwê usera */
	column = gtk_tree_view_column_new();
	g_assert(column != NULL);

	/* Ustaw kolumnê na fixed height mode, tak mamy ustawione drzewko */
	gtk_tree_view_column_set_sizing(GTK_TREE_VIEW_COLUMN(column), GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);

	renderer = gtk_cell_renderer_text_new();
	g_assert(renderer != NULL);

	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_add_attribute(column, renderer, "text", 0);
	
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(users_model), 0, GTK_SORT_ASCENDING);

	if (arch_populate_userlist_model(users_model) < 0) {
		gtk_widget_destroy(win_arch);
		gtk_tree_view_set_model(GTK_TREE_VIEW(tree), NULL);
		g_object_unref(users_model);

		win_arch = NULL;
		users_model = NULL;

		g_error("arch_populate_userlist_model failed");

		/* XXX: msgbox */
		return;
	}

	gtk_tree_view_set_model(GTK_TREE_VIEW(tree), GTK_TREE_MODEL(users_model));

	gtk_widget_show_all(win_arch);
}

static void win_arch_set_user(const char *id)
{
}

