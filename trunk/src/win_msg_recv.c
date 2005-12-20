
#include "win_msg_recv.h"
#include "glen_html.h"
#include "interface.h"
#include "support.h"
#include "win_main.h"
#include "win_msg_send.h"
#include "misc.h"

struct recv_msg {
	gchar *id;
	gchar *name;
	gchar *time;
	gchar *text;
};

static GtkWidget * win = NULL;
static GtkWidget * html = NULL;
static GList * msg_list = NULL;
static guint msg_count;
static guint msg_unread_count;
static GList *cur_msg = NULL;

static GtkWidget * win_msg_recv_create();

static void close_button_cb(GtkWidget *, gpointer);
static void next_button_cb(GtkWidget *, gpointer);
static void prev_button_cb(GtkWidget *, gpointer);
static void reply_button_cb(GtkWidget *, gpointer);

static void update_unread_label();
static void set_message(struct recv_msg *mesg);

void win_msg_recv_add(const gchar *id, const gchar *name, const gchar *text, const gchar *time)
{
	struct recv_msg *m;
	GtkWidget *w = NULL;
	
	if(win == NULL)
		win = win_msg_recv_create();

	m = (struct recv_msg *)g_malloc0(sizeof(struct recv_msg));

	m->id = g_strdup(id);
	if(name != NULL);
		m->name = g_strdup(name);
	m->time = g_strdup(time);
	m->text = g_strdup(text);

	msg_list = g_list_append(msg_list, m);
	msg_count++;
	
	if(msg_count == 1) {
		msg_unread_count = 0;
		cur_msg = msg_list;
		set_message(m);
	} else {
		w = lookup_widget(win, "button_next");
		gtk_widget_set_sensitive(w, TRUE);
		msg_unread_count++;
	}

	update_unread_label();

	gtk_widget_show_all(win);
}

static GtkWidget * win_msg_recv_create()
{
	GtkWidget *w;
	GtkWidget *ret;
	
	ret = create_win_msg_recv();
	g_assert(ret != NULL);

	w = lookup_widget(ret, "scroll_output");
	html = glen_html_new();
	gtk_container_add(GTK_CONTAINER(w), html);

	gtk_widget_show_all(ret);
	
	w = lookup_widget(ret, "button_next");
	g_signal_connect(G_OBJECT(w), "clicked", G_CALLBACK(next_button_cb), NULL);
	
	w = lookup_widget(ret, "button_prev");
	g_signal_connect(G_OBJECT(w), "clicked", G_CALLBACK(prev_button_cb), NULL);
	
	w = lookup_widget(ret, "button_reply");
	g_signal_connect(G_OBJECT(w), "clicked", G_CALLBACK(reply_button_cb), NULL);

	w = lookup_widget(ret, "button_close");
	g_signal_connect(G_OBJECT(w), "clicked", G_CALLBACK(close_button_cb), NULL);

	g_signal_connect(G_OBJECT(ret), "delete_event", G_CALLBACK(close_button_cb), NULL);

	msg_count = msg_unread_count = 0;
	cur_msg = NULL;

	return ret;
}

static void update_unread_label()
{
	GtkWidget *w;
	gchar *s;
	
	w = lookup_widget(win, "label_count");
	s = g_strdup_printf("<b>Nieprzeczytanych: %i/%i</b>",
		msg_unread_count, msg_count);

	gtk_label_set_markup(GTK_LABEL(w), s);

	g_free(s);
}

static void set_message(struct recv_msg *m)
{
	GtkWidget *w;
	gchar *s;
	GtkHTMLStream *stream;

	g_assert(win != NULL);
	g_assert(html != NULL);
	
	w = lookup_widget(win, "label_from");
	if(m->name != NULL)
		s = g_strdup_printf("<b>%s (%s)</b>", m->name, m->id);
	else
		s = g_strdup_printf("<b>%s</b>", m->id);

	gtk_label_set_markup(GTK_LABEL(w), s);
	g_free(s);

	w = lookup_widget(win, "label_time");
	s = g_strdup_printf("<b>%s</b>", m->time);
	gtk_label_set_markup(GTK_LABEL(w), s);
	g_free(s);

	stream = glen_html_begin(GTK_HTML(html));
	s = markup_text(m->text);
	gtk_html_write(GTK_HTML(html), stream, s, strlen(s));
	gtk_html_end(GTK_HTML(html), stream, GTK_HTML_STREAM_OK);
	g_free(s);
}

static void next_button_cb(GtkWidget *w, gpointer d)
{
	GtkWidget *prev;

	prev = lookup_widget(win, "button_prev");
	gtk_widget_set_sensitive(prev, TRUE);

	if(cur_msg->next != NULL) {
		cur_msg = cur_msg->next;
		set_message((struct recv_msg *)cur_msg->data);
	}

	if(cur_msg->next == NULL)
		gtk_widget_set_sensitive(w, FALSE);

	if(msg_unread_count != 0)
		msg_unread_count--;
	update_unread_label();
}

static void prev_button_cb(GtkWidget *w, gpointer d)
{
	GtkWidget *next;

	next = lookup_widget(win, "button_next");
	gtk_widget_set_sensitive(next, TRUE);

	if(cur_msg->prev != NULL) {
		cur_msg = cur_msg->prev;
		set_message((struct recv_msg *)cur_msg->data);
	}

	if(cur_msg->prev == NULL)
		gtk_widget_set_sensitive(w, FALSE);
}

static void reply_button_cb(GtkWidget *w, gpointer d)
{
	struct recv_msg *m = cur_msg->data;
	gchar *s, *p;
	
	s = g_strdup_printf("-- Twoja wiadomo¶æ: --\n%s\n---\n", m->text);
	p = toutf(s);
	win_msg_send_show(user_get(m->id), p);
	g_free(s);
	g_free(p);
}

static void close_button_cb(GtkWidget *w, gpointer d)
{
	struct recv_msg *m;
	
	for(cur_msg = msg_list; cur_msg != NULL; cur_msg = cur_msg->next) {
		m = (struct recv_msg *)cur_msg->data;
		g_free(m->id);
		g_free(m->text);
		g_free(m->time);
		g_free(m);
	}

	g_list_free(msg_list);

	gtk_widget_hide(win);
	gtk_widget_destroy(html);
	gtk_widget_destroy(win);

	win = NULL;
	html = NULL;
	msg_list = NULL;
}

