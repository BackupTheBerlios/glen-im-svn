#define GLEN_DUMP_ARCH
#include <stdlib.h>
#include <stdio.h>
#include <glib.h>
#include <time.h>
#include <string.h>

#include "arch.h"

static int arch_to_html(GSList *arch, char *id);

int
main(int argc, char **argv)
{
	GSList *arch;

	if (argc != 2) {
		fprintf(stderr, "Nie podales id uzytkownika: id@tlen.pl\n");
		exit(EXIT_FAILURE);
	}

	arch = arch_load(argv[1]);
	return arch_to_html(arch, argv[1]);
}

GSList *arch_load(const char *id)
{
	char buf[8096];
	FILE *fp;
	GSList *list = NULL;
	GSList *sub = NULL;
	GSList *messages = NULL;
	ArchEntry *e = NULL;
	ArchEntry entry;
	int ret;
	
	ret = snprintf(buf, sizeof(buf), "%s/.glen/archive/%s", getenv("HOME"), id);
	if (ret < 0 || ret >= sizeof(buf)) {
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
			//printf("fread failed #1, ret=%i\n", ret);
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
					//printf("fread failed, ret=%i\n", ret);
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

#if 0
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
#endif

	return list;
}

#define HTML_START \
	"<html><head><meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\"><title>Zrzut archiwum rozmów z %s</title></head><body>" \
	"<font face=Verdana size=11px><center>"
#define HTML_END \
	"</table></body></html>"
//#define HTML_CHAT_TO_ME "<tr><td><font size=-2><b>%s (%s)</b></font></td></tr><tr><td>%.*s</td></tr>"
#define HTML_CHAT_TO_ME "<table style=\"background-color: #fefefe;\" width=\"75%%\" border=\"0\" " \
		"cellspacing=\"2\" cellpadding=\"0\">" \
		"<tr><td><font size=-1><b>%s</b> (%s)</font></td></tr>" \
		"<tr><td>%.*s</td></tr>" \
		"</table>"
#define HTML_CHAT_FROM_ME "<table style=\"background-color: #f0f0f0;\" width=\"75%%\" border=\"0\" " \
		"cellspacing=\"2\" cellpadding=\"0\">" \
		"<tr><td><font size=-1><b>Ja</b> (%s)</font></td></tr>" \
		"<tr><td>%.*s</td></tr>" \
		"</table>"
#define HTML_CHAT_START "</br><table cellspacing=0 cellpadding=0><tr><td><i>Poczatek rozmowy (%s)</i></td></tr>"
#define HTML_CHAT_ENDING "</table>"

static void
entry_to_html(ArchEntry *e, char *id)
{
	switch (e->type) {
		case ARCH_TYPE_CHAT_START_BY_ME:
		case ARCH_TYPE_CHAT_START:
			printf(HTML_CHAT_START, ctime(&e->time));
			break;
		case ARCH_TYPE_CHAT_TO_ME:
			printf(HTML_CHAT_TO_ME, id, ctime(&e->time), e->textlen, e->text);
			break;
		case ARCH_TYPE_CHAT_FROM_ME:
			printf(HTML_CHAT_FROM_ME, ctime(&e->time), e->textlen, e->text);
			break;
	}
}

static int
arch_to_html(GSList *list, char *id)
{
	GSList *i, *sub;
	ArchEntry *e;

	printf(HTML_START, id);
	
	i = list;
	while (i != NULL) {
		sub = i->data;

		while (sub != NULL) {
			e = sub->data;
			entry_to_html(e, id);
			sub = sub->next;
		}

		i = i->next;
	}
			
	printf(HTML_END);

	return 0;
}
