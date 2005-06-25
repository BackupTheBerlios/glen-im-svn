/* vim: set tw=0: */

#include "misc.h"
#include "arch.h"
#include "user.h"

/* lista list z wpisami do zrzucenia na dysk */
static GSList *buffers;
/* tak czesto bedziemy zrzucac dane na dysk */
static const guint flush_timeout = 1000;

/* Zrzuc jeden wpis na dysk */
static int arch_flush_entry(User *u, ArchEntry *e);

int arch_append_chat(User *u, gchar from_me, const gchar *from, const gchar *msg)
{
	ArchEntry *a;

	a = g_malloc(sizeof(ArchEntry));
	if (a == NULL) {
		return -1;
	}

	if (from_me != 0) {
		a->type = ChatFromMe;
	} else {
		a->type = ChatToMe;
	}

	time(&a->time);
	a->data = c;

	c->from = strdup(from);
	c->fromlen = strlen(from);

	c->text = strdup(msg);
	c->textlen = strlen(msg);

	if (c->from == NULL || c->text == NULL) {
		free(c);
		free(a);
		free(c->from);
		free(c->text);
		
		return -1;
	}

	u->history = g_slist_append(u->history, c);

	return 0;
}

int arch_flush_user(User *u)
{
	GSList *l;

	l = u->history;

	while(l != NULL) {
		arch_flush_entry(u, (ArchEntry *)l);
		l = l->next;
	}

	return 0;
}

static int arch_flush_entry(User *u, ArchEntry *e)
{
	ChatEntry *c;
	InfoEntry *i;
	char buf[4096];
	ArchEntry *ba;
	ChatEntry *bc;
	InfoEntry *bi;

	size_t len;

	ba = (ArchEntry *)buf;

	ba->type = e->type;
	ba->time = e->time;

	len = sizeof(e->type) + sizeof(e->time);

	if (e->type != Info) {
		c = e->data;
		bc = e->data;
		bc->fromlen = c->fromlen;
		memcpy(bc->from, c->from, c->fromlen);
		bc->textlen = c->textlen;
		memcpy(bc->text, c->text, c->textlen);

		len += sizeof(c->fromlen) + sizeof(c->textlen) + c->textlen + c->fromlen;
	} else {
		i = e->data;

	}

	fwrite(buf, len, 1, u->fp);
}

