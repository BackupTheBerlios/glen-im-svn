/* vim: set tw=0: */

#ifndef ARCH_H
#define ARCH_H

#ifndef GLEN_DUMP_ARCH
#include "user.h"
#else
#define GLEN_DUMP_ARCH_DEFINED
#include <gtk/gtk.h>
#endif

/* Typ wpisu w archiwum */
#define ARCH_TYPE_CHAT_TO_ME		0	/* Tekst do mnie */
#define ARCH_TYPE_CHAT_FROM_ME		1	/* Tekst kt�ry wys�a�em */
#define ARCH_TYPE_MSG_FROM_ME		2	/* Wiadomo�� kt�r� wys�a�em */
#define ARCH_TYPE_MSG_TO_ME		3	/* Wiadomo�� do mnie */
#define ARCH_TYPE_CHAT_START_BY_ME	4	/* rozmowa rozpocz�ta przez nas */
#define ARCH_TYPE_CHAT_START		5	/* rozmowa rozpocz�ta przez drug� stron� */

/* Jeden wpis w pliku z histori� */
struct structArchEntry {
	char type;		/* Typ. Od tego pola zale�y jak interpretujemy reszt� */
	time_t time;		/* Czas wyst�pienia zdarzenia */
	guint textlen;		/* D�ugo�� ewentualnej wiadomo�ci */
	gchar *text;		/* Wiadomo�� */
} __attribute__((__packed__));

typedef struct structArchEntry ArchEntry;

GSList *arch_load(const char *id);
void arch_list_free(GSList *list);
#ifndef GLEN_DUMP_ARCH
/* Odpalenie podsystemu archiwum */
int arch_init(void);
/* Dodanie wpisu do archiwum */
int arch_add(char type, time_t now, User *user, const char *msg);
/* Zamkniecie sesji zapisywania do archiwum dla usera */
void arch_close(User *user);
int arch_populate_userlist_model(GtkTreeStore *model);

/* Wylacza flushowanie archiwum. Potrzebne przy wyjsciu z programu. */
void arch_disable_flushing(void);
#endif

#endif

