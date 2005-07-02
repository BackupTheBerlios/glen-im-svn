/*
   arch.c - interfejs dysk-program do archiwum. Wy¶wietlaniem zajmuje siê
   win_arch. Tutaj jedynie ³adujemy/zapisujemy dane.
*/
#include "misc.h"
#include "arch.h"
#include "user.h"
#include <sys/stat.h>
#include <sys/types.h>

/* Lista userow ktorych pliki archiwum flushujemy */
static GSList *flush_list = NULL;
/* tak czesto bedziemy zrzucac dane na dysk (ms) */
static const guint flush_timeout = 8000;
static char *archive_basedir = NULL;

/* Funkcja flushujaca dane do plików z archiwum */
static gboolean arch_flush_func(gpointer data);

int arch_init(void)
{
	char *home;
	char buf[4096];
	int ret;

	home = getenv("HOME");
	if (home == NULL) {
		g_error("Nie moge znalezc $HOME");
		return -1;
	}

	ret = snprintf(buf, sizeof(buf), "%s/.glen/archive", home);
	if (ret < 0 || ret >= sizeof(buf)) {
		g_error("snprintf failed");
		return -1;
	}

	ret = mkdir(buf, 0700);
	if (ret < 0) {
		if (errno != EEXIST) {
			g_error("Nie moge utworzyc katalogu %s\n", buf);
			return -1;
		}
	}

	archive_basedir = strdup(buf);
	if (archive_basedir == NULL) {
		g_error("strdup failed");
		return -1;
	}

	g_timeout_add(flush_timeout, arch_flush_func, NULL);

	return 0;
}

int arch_add(char type, time_t now, User *user, const char *msg)
{
	char buf[4096];
	ArchEntry *e;

	if (user->arch_file == NULL) {
		if (xsprintf(buf, sizeof(buf), "%s/%s", archive_basedir, user->id) < 0) {
			return -1;
		}

		user->arch_file = fopen(buf, "a");
		if (user->arch_file == NULL) {
			g_error("nie mozna otworzyc %s", buf);
			return -1;
		}

		/* Wrzuc go na liste userow ktorych archiwum flushujemy */
		flush_list = g_slist_append(flush_list, user);
	}

	/* Przygotuj bufor */
	e = (ArchEntry *)buf;
	e->type = type;
	e->time = now;
	if (msg != NULL) {
		e->textlen = strlen(msg);
	} else {
		e->textlen = 0;
	}

	/* Zapisz 'naglowek' */
	if (fwrite(e, sizeof(e->type) + sizeof(e->time) + sizeof(e->textlen), 1, user->arch_file) < 1) {
		return -1;
	}

	/* I tekst, jesli jest */
	if (e->textlen != 0) {
		if (fwrite(msg, e->textlen, 1, user->arch_file) != 1) {
			return -1;
		}
	}

	return 0;
}

void arch_close(User *user)
{
	if (user->arch_file != NULL) {
		fclose(user->arch_file);
		user->arch_file = NULL;

		flush_list = g_slist_remove(flush_list, user);
	}
}

/* Wype³nia model list± userów, których archiwum jest na dysku. Zwraca -1 w
 * przypadku b³êdu, 0 gdy wszystko ok */
int arch_populate_userlist_model(GtkTreeStore *model)
{
	User *user = NULL;
	GtkTreeIter iter;
	GDir *dir = NULL;
	char *id = NULL;
	char *utf = NULL;
	char buf[4096];

	dir = g_dir_open(archive_basedir, 0, NULL);
	if (dir == NULL) {
		g_error("Nie mozna odczytac zawartosci katalogu %s", archive_basedir);
		
		return -1;
	}

	for (id = (char *)g_dir_read_name(dir); id != NULL; id = (char *)g_dir_read_name(dir)) {
		/* Sprawd¼, czy mamy zwyk³y plik */
		if (xsprintf(buf, sizeof(buf), "%s/%s", archive_basedir, id) < 0) {
			g_dir_close(dir);

			return -1;
		}

		if (g_file_test(buf, G_FILE_TEST_IS_REGULAR)) {
			/* Mamy plik, dodaj do modelu */
			/* Spróbuj znale¼æ usera i wstawiæ jego imiê zamiast id */
			user = user_get(id);
			if (user != NULL) {
				id = user->name;
			}

			utf = toutf(id);
			g_assert(utf != NULL);
			
			g_debug("dodaje %s", utf);
			gtk_tree_store_append(model, &iter, NULL);
			gtk_tree_store_set(model, &iter, 0, utf, -1);	/* 0, mamy tylko jedn± kolumnê */

			g_free(utf);
		} 
	}

	g_dir_close(dir);

	return 0;
}

/* £aduje historiê dla podanego u¿ytkownika. Zwraca wska¼nik na listê list z
 * rozmowami/wiadomo¶ciami, lub NULL w przypadku b³êdu */
GSList *arch_load(const char *id)
{
	char buf[4096];
	FILE *fp;
	GSList *list = NULL;
	GSList *sub = NULL;
	GSList *messages = NULL;
	ArchEntry *e = NULL;
	ArchEntry entry;
	size_t ret;
	
	if (xsprintf(buf, sizeof(buf), "%s/%s", archive_basedir, id) < 0) {
		return NULL;
	}

	fp = fopen(buf, "r");
	if (fp == NULL) {
		g_warning("Nie mozna otworzyc pliku archiwum %s", buf);
		/* XXX: msgbox */

		return NULL;
	}

	while (!feof(fp)) {
		printf(".\n");
		/* Odczytaj podstawowe dane */
		ret = fread(&entry, sizeof(entry.type) + sizeof(entry.time) + sizeof(entry.textlen), 1, fp);
		if (ret != 1) {
			puts("tutaj");
			goto bail_out;
		}

		/* Za³aduj tekst, je¶li potrzeba */
		if (entry.textlen != 0) {
			entry.text = malloc(entry.textlen + 1);	/* +1 na \0 */
			if (entry.text == NULL) {
				goto bail_out;
			} else {
				ret = fread(entry.text, entry.textlen, 1, fp);
				if (ret != 1) {
					puts("tutaj");
					free(entry.text);
					goto bail_out;
				}
			}

			entry.text[entry.textlen] = '\0';
		} else {
			entry.text = NULL;
		}

		e = malloc(sizeof(ArchEntry));
		if (e == NULL) {
			goto bail_out;
		} else {
			memcpy(e, &entry, sizeof entry);
			/* Wiadomo¶ci wrzucamy na oddzieln± podlistê */
			if (entry.type == ARCH_TYPE_MSG_TO_ME || entry.type == ARCH_TYPE_MSG_FROM_ME) {
				messages = g_slist_append(messages, e);
			} else {
				sub = g_slist_append(sub, e);
				printf("-> %s\n", e->text);
			}
		}

		/* Je¶li to jest pocz±tek rozmowy, wstawiamy podlistê. */
		if (sub != NULL && (entry.type == ARCH_TYPE_CHAT_START || entry.type == ARCH_TYPE_CHAT_START_BY_ME)) {
			/* Wstaw poprzedni± podlistê */
			list = g_slist_append(list, sub);
			/* Nastêpny element wyl±duje ju¿ w nowej li¶cie */
			sub = NULL;
		}

	}
bail_out:

	/* Dodaj podlistê z wiadomo¶ciami. Dodamy j± na pocz±tku, ¿eby mieæ do
	 * niej ³atwy dostêp. */
	list = g_slist_prepend(list, messages);
	fclose(fp);

	{
		/* Omin wiadomosci */
		GSList *ii = list->next;

		while (ii != NULL) {
			sub = ii->data;

			printf("Rozmowa\n");
			while (sub != NULL) {
				e = sub->data;
				printf("   %s\n", e->text);
				sub = sub->next;
			}

			ii = ii->next;
		}
	}

	return list;
#if 0
	g_warning("Blad ladowania pliku '%s'", buf);
	arch_list_free(list);
	fclose(fp);

	return NULL;
#endif
}

/* Zwalnia pamiêæ po li¶cie zwróconej przez arch_load */
void arch_list_free(GSList *list)
{
	GSList *i;
	ArchEntry *entry;
	
	while (list != NULL) {
		i = list->data;

		while (i != NULL) {
			entry = i->data;

			free(entry->text);
			free(i->data);

			i = i->next;
		}

		g_slist_free(i);

		list = list->next;
	}

	g_slist_free(list);
}

static gboolean arch_flush_func(gpointer data)
{
	GSList *i = NULL;
	User *user = NULL;

	for (i = flush_list; i != NULL; i = i->next) {
		user = i->data;
		if (user->arch_file != NULL) {
			fflush(user->arch_file);
		} else {
			g_error("arch_flush_func: user->arch_file == NULL");
		}
	}

	return TRUE;
}


