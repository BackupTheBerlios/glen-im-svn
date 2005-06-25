/* vim: set tw=0: */

#ifndef USER_H
#define USER_H

#include <gnome.h>

enum UserFlags {
	UserFlagNoName = 0x01, /* nie ma ustawionego imienia */
	UserFlagPresenceNotify = 0x02, /* powiadamiaj o dostepnosci */
	UserFlagSoundsOn = 0x04, /* dzwieki wlaczone */
	UserFlagNoSubscribtion = 0x08, /* nie mamy autoryzacji */
	UserFlagWantsSubscribtion = 0x10, /* ktos chce naszej autoryzacji */
	UserFlagBlocked = 0x20 /* zablokowany */
};

#define USER_FLAG_SET(user, flag) ((user)->flags |= flag)
#define USER_FLAG_CLEAR(user, flag) ((user)->flags &= ~flag)
#define USER_FLAG_ISSET(user, flag) ((user)->flags & flag)

struct structUser {
	/* ogolne */
	gchar *id;
	gchar *name;
	gchar *desc;
	gint status;
	struct structGroup *group;

	guint flags;
	
	/* do wyswietlania w treeview */
	gchar *text;
	GtkTreeRowReference *row;
	GdkPixbuf *icon_pixbuf;

	/* otwarte okno do wysylania wiadomosci */
	GtkWidget *win_msg_send;

	/* Lista ArchEntry do zrzucenia na dysk */
	GSList *history;
	/* Plik do którego leci historia */
	FILE *fp;
};

typedef struct structUser User;

struct structGroup {
	gchar *name;
	GtkTreeRowReference *row;
	guint child_count;
	guint child_active_count;
	gboolean expanded;
	gchar *text;
};

typedef struct structGroup Group;

/* znajdz usera */
User * user_get(const gchar *id);
/* nowy user */
User * user_new(const gchar *id);
User * user_new_full(const gchar *id, const gchar *name, const gchar *desc,
	guint status);
void user_update(User *, const gchar *name, const gchar *desc,
	guint status);
void user_set_group(User *u, Group *g);
void user_set_name(User *u, const gchar *name);
void user_set_status(User *u, const gchar *desc, guint status);
/* zwraca nowy wskaznik na liste userow, przydatne przy iteracji i usuwaniu */
GSList * user_remove(User *u);

/* szuka grupy, jesli nie istnieje i create == TRUE tworzy nowa, w przeciwnym
 * wypadku zwraca NULL */
Group * group_new(const gchar *name);
Group * group_get(const gchar *name);
GSList * group_remove(Group *);

GSList *group_get_list();
GSList *user_get_list();

// grupa dla kontaktow na autoryzacje od ktorych czekamy
#define GROUP_NAME_WAIT_AUTH "Oczekiwanie na autoryzacjÄ™"
// ci userzy nie dostali naszej autoryzacji
#define GROUP_NAME_NO_AUTH "Brak autoryzacji"
//#define GROUP_NAME_UNKNOWN "Kontakty spoza listy"

#endif

