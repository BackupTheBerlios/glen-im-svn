
#include "prefs.h"
#include "misc.h"
#include "emotes.h"
#include "tlen.h"


gchar *toutf(const gchar *str)
{
	gsize a, b;

	warn_if_fail(str != NULL);
	
	if(str == NULL)
		return NULL;

	return g_convert(str, -1, "UTF-8", "iso-8859-2", &a, &b, NULL);
}

gchar *fromutf(const gchar *str)
{
	gsize a, b;
	
	warn_if_fail(str != NULL);
	
	if(str == NULL)
		return NULL;

	return g_convert(str, -1, "iso-8859-2", "UTF-8", &a, &b, NULL);
}

static const char *href[] = {
	"http://",
	"ftp://"
};

static const guint href_count = sizeof(href)/sizeof(href[0]);
static const gsize bufsize = 4096;

static inline gsize markup_link(gchar *str, gchar *buf);
static inline gsize markup_emote(gchar *str, gchar *buf);

gchar *markup_text(const gchar *str)
{
	gchar *msg, *s;
	gchar *buf;
	gsize add;
	gchar one[3];
	
	msg = g_markup_escape_text(str, -1);
	buf = g_malloc(bufsize);
	g_assert(buf != NULL);

	s = msg;
	buf[0] = '\0';

	one[2] = 0;
	one[1] = 0;
	
	while(*s != '\0') {
		if(pref_chat_use_emotes ==  TRUE) {
			add = markup_emote(s, buf);
			if(add != 0) {
				s += add;	
				continue;
			}
		}

		add = markup_link(s, buf);
		if(add != 0) {
			s += add;
			continue;
		}

		if(*s == '\n')
			g_strlcat(buf, "<BR>", bufsize);
		else {
			one[0] = *s;
			g_strlcat(buf, one, bufsize);
		}
		s++;
	}

	g_free(msg);

	return buf;
}

static inline gsize markup_link(gchar *str, gchar *buf)
{
	gsize len;
	guint j;
	gchar link[1024];
	gchar *con;
	
	len = 0;
	for(j = 0; j < href_count; j++) {
		if(g_str_has_prefix(str, href[j]) == TRUE) {
			while(*str != ' ' && *str != '\n' && *str != '\0'
				&& len < sizeof(link)) {
				link[len] = *str;
				str++;
				len++;
			}

			link[len] = 0;
			
			con = g_strconcat("<a href=\"", link, "\">", link,
					"</a>", NULL);
			g_strlcat(buf, con, bufsize);
			g_free(con);
			break;
		}
	}

	return len;
}

static const gchar *img_fmt = "<img src=\"%s\" alt=\"%s\" border=\"0\" "
	"align=\"middle\">";

static inline gsize markup_emote(gchar *str, gchar *buf)
{
	Emote *e;
	GSList *l, *t;
	gchar *tag;
	gchar tmp[512];
	gsize len;

	/* to lekko naciagane... */
#if 0
	/* mamy poczatek emotki? */
	if(*str != ':' && *str != '&' && *str != '[' && *str != ';') {
		return 0;
	}
#endif
	len = 0;

	for(l = emote_set->emotes; l != NULL; l = l->next) {
		e = (Emote *)l->data;
		if(e == NULL)
			continue;

		for(t = e->tags; t != NULL; t = t->next) {
			tag = (gchar *)t->data;
			if(tag == NULL)
				continue;

			if(g_str_has_prefix(str, tag) == TRUE) {
				//printf("MATCH: '%s' - '%s'\n", str, tag);
				snprintf(tmp, sizeof(tmp), img_fmt, 
					e->url, e->tooltip);
				g_strlcat(buf, tmp, bufsize);
				len = strlen(tag);
				return len;
			}
		}
	}

	return len;
}

const gchar *get_time(time_t *when)
{
	time_t t;
	static gchar buf[60];
	
	if(when == NULL)
		time(&t);
	else
		t = *when;

	strftime(buf, sizeof(buf), "%H:%M:%S", localtime(&t));
	
	return buf;
}

const gchar *get_date(time_t *when)
{
	time_t t;
	static gchar buf[60];
	
	if(when == NULL)
		time(&t);
	else
		t = *when;

	strftime(buf, sizeof(buf), "%d.%m", localtime(&t));

	return buf;
}

gchar *strip_user(const gchar *user)
{
	gchar *s, *p;
	gchar *buf;
	const gsize bufsize = 100;

	buf = g_malloc(bufsize);
	
	s = buf;
	p = (gchar *)user;
	while(*p != '@' && *p != '\0' && p - user < bufsize)
		*s++ = *p++;

	*s = '\0';

	return buf;
}

gchar *text_view_get_text(GtkTextView *v)
{
	GtkTextBuffer *buf;
	GtkTextIter start, end;

	buf = gtk_text_view_get_buffer(v);

	if(buf == NULL)
		return NULL;
	else {
		gtk_text_buffer_get_start_iter(buf, &start);
		gtk_text_buffer_get_end_iter(buf, &end);
		return gtk_text_buffer_get_text(buf, &start, &end, TRUE);
	}
}

void text_view_set_text(GtkTextView *v, const gchar *text)
{
	GtkTextBuffer *buf;
	
	buf = gtk_text_view_get_buffer(v);
	
	if(buf != NULL)
		gtk_text_buffer_set_text(buf, text, strlen(text));
}

void text_view_clear(GtkTextView *v)
{
	GtkTextBuffer *buf;
	GtkTextIter start, end;

	buf = gtk_text_view_get_buffer(v);
	g_assert(buf != NULL);

	gtk_text_buffer_get_start_iter(buf, &start);
	gtk_text_buffer_get_end_iter(buf, &end);
	gtk_text_buffer_delete(buf, &start, &end);
}

const gchar *get_status_desc(guint status)
{
	gchar *ret = "Nieznany stan - zg³o¶ b³±d";

	switch(status) {
	case TLEN_PRESENCE_AVAILABLE:
		ret = "Dostêpny";
		break;
	case TLEN_PRESENCE_CHATTY:
		ret = "Porozmawiajmy";
		break;
	case TLEN_PRESENCE_DND:
		ret = "Jestem zajêty";
		break;
	case TLEN_PRESENCE_AWAY:
		ret = "Zaraz wracam";
		break;
	case TLEN_PRESENCE_EXT_AWAY:
		ret = "Wrócê pó¼niej";
		break;
	case TLEN_PRESENCE_INVISIBLE:
		ret = "Niewidoczny";
		break;
	case TLEN_PRESENCE_UNAVAILABLE:
		ret = "Niedostêpny";
		break;
	}

	return toutf(ret);
}

guint get_status_num(const gchar *name)
{
	gchar *s = NULL;
	guint ret = 0;
	
	if(name == NULL)
		return TLEN_PRESENCE_UNAVAILABLE;
	
	s = fromutf(name);

	if(strcmp(s, "Dostêpny") == 0)
		ret = TLEN_PRESENCE_AVAILABLE;
	else if(strcmp(s, "Porozmawiajmy") == 0)
		ret = TLEN_PRESENCE_CHATTY;
	else if(strcmp(s, "Jestem zajêty") == 0)
		ret = TLEN_PRESENCE_DND;
	else if(strcmp(s, "Zaraz wracam") == 0)
		ret = TLEN_PRESENCE_AWAY;
	else if(strcmp(s, "Wrócê pó¼niej") == 0)
		ret = TLEN_PRESENCE_EXT_AWAY;
	else if(strcmp(s, "Niewidoczny") == 0)
		ret = TLEN_PRESENCE_INVISIBLE;
	else if(strcmp(s, "Niedostêpny") == 0)
		ret = TLEN_PRESENCE_UNAVAILABLE;

	g_assert(ret != 0);

	g_free(s);

	return ret;
}

GtkWidget *yes_no_dialog(GtkWidget *win, const gchar *m)
{
	GtkWidget *w;

	w = gtk_message_dialog_new_with_markup(GTK_WINDOW(win), 0,
		GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO, m);

	g_assert(w != NULL);

	return w;
}

int xsprintf(char *buf, size_t buflen, const char *fmt, ...)
{
	int ret;
	va_list vp;

	va_start(vp, fmt);
	ret = vsnprintf(buf, buflen, fmt, vp);
	va_end(vp);

	if (ret < 0 || ret >= buflen) {
		return -1;
	}

	return 0;
}

