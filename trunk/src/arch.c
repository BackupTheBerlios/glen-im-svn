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
static guint flush_timer_id = 0;

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

	flush_timer_id = g_timeout_add(flush_timeout, arch_flush_func, NULL);

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

/* Usuñ timer flushuj±cy dane z plików archiwum na dysk. Wcze¶niej
   wywo³aj funkcjê obs³ugiwan± przez timer, aby wszystko co nale¿y
   znalaz³o siê w plikach. */
void arch_disable_flushing(void)
{
	arch_flush_func(NULL);

	if (g_source_remove(flush_timer_id) == FALSE) {
		g_warning("Nie udalo sie usunac timera flushujacego pliki archiwum. Spodziewaj sie crasha.");
	}
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

			g_debug("arch_populate_userlist_model: dodaje %s", id);
			gtk_tree_store_append(model, &iter, NULL);
			gtk_tree_store_set(model, &iter, 0, id, -1);	/* 0, mamy tylko jedn± kolumnê */
		} 
	}

	g_dir_close(dir);

	return 0;
}

static char * dump_arch_entry_type(char type)
{
	static const char *types[] = { "CHAT_TO_ME", "CHAT_FROM_ME", "MSG_FROM_ME",
		"MSG_TO_ME", "CHAT_START_BY_ME", "CHAT_START" };
	size_t index;

	index = type;

	if (index >= sizeof(types) / sizeof(types[0])) {
		return "UNKNOWN";
	} else {
		return (char *)types[index];
	}
}

static void dump_arch_entry(ArchEntry *e)
{
	printf("ArchEntry\n");
	printf("  type =    %s\n", dump_arch_entry_type(e->type));
	printf("  time =    %s", ctime(&e->time));
	printf("  textlen = %i\n", e->textlen);
	if (e->text != NULL) {
		printf("  text =    '%s'\n", e->text);
	}
}

/* £aduje historiê dla podanego u¿ytkownika. Zwraca wska¼nik na listê list z
 * rozmowami/wiadomo¶ciami, lub NULL w przypadku b³êdu */
GSList *arch_load(const char *id)
{
	char buf[8096];
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
		/* Odczytaj podstawowe dane */
		ret = fread(&entry, sizeof(entry.type) + sizeof(entry.time) + sizeof(entry.textlen), 1, fp);
		if (ret != 1) {
			printf("fread failed #1, ret=%i\n", ret);
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
					printf("fread failed, ret=%i\n", ret);
					free(entry.text);
					goto bail_out;
				}
			}

			entry.text[entry.textlen] = '\0';
		} else {
			entry.text = NULL;
		}

		//dump_arch_entry(&entry);

		/* Skopiuj wpis */
		e = malloc(sizeof(ArchEntry));
		if (e == NULL) {
			goto bail_out;
		} else {
			memcpy(e, &entry, sizeof entry);
			if (entry.type == ARCH_TYPE_MSG_TO_ME || entry.type == ARCH_TYPE_MSG_FROM_ME) {
			/* Wiadomo¶ci wrzucamy na oddzieln± podlistê */
				messages = g_slist_append(messages, e);
			} else if (entry.type == ARCH_TYPE_CHAT_START_BY_ME || entry.type == ARCH_TYPE_CHAT_START) {
			/* Je¶li jest to pocz±tek rozmowy, robimy z niej oddzieln± podlistê */
				if (sub != NULL) {
					list = g_slist_append(list, sub);
					sub = NULL;
				}

				sub = g_slist_append(sub, e);
			} else if (entry.type == ARCH_TYPE_CHAT_FROM_ME || entry.type == ARCH_TYPE_CHAT_TO_ME) {
			/* Wypowiedzi w rozmowie wrzucamy na podlistê */
				sub = g_slist_append(sub, e);
			}
		}
	}
bail_out:
	/* Wrzuæ ostatni± podlistê */
	if (sub != NULL) {
		list = g_slist_append(list, sub);
	}

	/* Dodaj podlistê z wiadomo¶ciami. Dodamy j± na pocz±tku, ¿eby mieæ do
	 * niej ³atwy dostêp. */
	list = g_slist_prepend(list, messages);
	fclose(fp);

	{
		puts("DUMP ARCHIWUM");
		/* Omin wiadomosci */
		GSList *ii = list->next;

		while (ii != NULL) {
			puts("---");

			sub = ii->data;

			while (sub != NULL) {
				e = sub->data;
				dump_arch_entry(e);
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
			g_warning("arch_flush_func: user->arch_file == NULL");
		}
	}

	return TRUE;
}


