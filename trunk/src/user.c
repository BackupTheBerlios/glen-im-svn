
#include "user.h"
#include "misc.h"
#include "tlen.h"


/* usuwaniem rzeczy specyficznych dla win_main (tzn: text, path itd) zajmuje
 * sie okno glowne */

static GSList *user_list = NULL;
static GSList *group_list = NULL;

User * user_get(const gchar *id)
{
	GSList *l;
	User *u, *ui;
	gchar *u1, *u2;

	u1 = strip_user(id);
	u2 = NULL;
	u = NULL;

	for(l = user_list; l != NULL; l = l->next) {
		ui = (User *)l->data;
		if(ui != NULL) {
			u2 = strip_user(ui->id);
			if(g_strcasecmp(u1, u2) == 0) {
				u = ui;
				break;
			}
			g_free(u2);
		}
	}

	g_free(u1);

	return u;
}

void user_set_group(User *u, Group *gnew)
{
	Group *gold;
	
	gold = u->group;
	/* zmiana grupy */
	if(gnew != gold) {
		if(gold != NULL)
			gold->child_count--;
		gnew->child_count++;
		if(o2_status_is_avail(u->status) == TRUE) {
			if(gold != NULL)
				gold->child_active_count--;
			gnew->child_active_count++;
		}
		
		u->group = gnew;
	}
}

void user_set_name(User *u, const gchar *name)
{
	g_free(u->name);
	
	if(name != NULL) {
		USER_FLAG_CLEAR(u, UserFlagNoName);
		u->name = g_strdup(name);
	} else {
		USER_FLAG_SET(u, UserFlagNoName);
		u->name = g_strdup(u->id);
	}
}

void user_set_status(User *u, const gchar *desc, guint status)
{
	g_free(u->desc);
	if(desc == NULL)
		u->desc = NULL;
	else
		u->desc = g_strdup(desc);

	u->status = status;
}

User * user_new(const gchar *id)
{
	User *u;
	
	u = g_malloc0(sizeof(User));
	u->id = g_strdup(id);
	user_list = g_slist_append(user_list, u);

	USER_FLAG_SET(u, UserFlagNoSubscribtion);
	USER_FLAG_SET(u, UserFlagNoName);

	return u;
}

User * user_new_full(const gchar *id, const gchar *name, const gchar *desc,
	guint status)
{
	User *u;

	u = user_new(id);
	user_update(u, name, desc, status);

	return u;
}

void user_update(User *u, const gchar *name, const gchar *desc,
	guint status)
{
	g_assert(u != NULL);

	u->status = status;

	g_free(u->name);
	g_free(u->desc);
	
	if(name != NULL && strlen(name) != 0) {
		u->name = g_strdup(name);
		USER_FLAG_CLEAR(u, UserFlagNoName);
	} else {
		u->name = g_strdup(u->id);
		USER_FLAG_SET(u, UserFlagNoName);
	}

	u->desc = g_strdup(desc);
}

GSList * user_remove(User *u)
{
	printf("user_remove: %s\n", u->id);
	g_free(u->id);
	g_free(u->name);
	g_free(u->desc);
	
	user_list = g_slist_remove(user_list, u);
	g_free(u);

	return user_list;
}

GSList * user_get_list()
{
	return user_list;
}

Group * group_new(const gchar *name)
{
	Group *g;

	g = (Group *)g_malloc0(sizeof(Group));
	if(name != NULL)
		g->name = g_strdup(name);
	else
		g->name = g_strdup("Kontakty");
	g->text = g_strdup_printf("%s (0/0)", g->name);
	/* XXX cos tu sie krzaczy */
	//g->expanded = pref_userlist_autoexpand;
	g->expanded = FALSE;
//	if(name == NULL) /* obejscie na razie tego, ze automatycznie
//			    zaznacza sie pierwszy row */
//		g->expanded = !g->expanded;
	group_list = g_slist_append(group_list, g);

	return g;
}

Group * group_get(const gchar *name)
{
	GSList *l;
	Group *gi;
	gchar *id;

	if(name == NULL)
		id = "Kontakty";
	else
		id = (gchar *)name;

	for(l = group_list; l != NULL; l = l->next) {
		gi = (Group *)l->data;
		if(g_strcasecmp(id, gi->name) == 0) {
			return gi;
		}
	}

	return NULL;
}

GSList * group_remove(Group *g)
{
	printf("group_remove: %s\n", g->name);
	g_free(g->name);
	g_free(g);

	group_list = g_slist_remove(group_list, g);

	return group_list;
}

GSList *group_get_list()
{
	return group_list;
}
