/* vim: set tw=0: */

#include <gnome.h>
#include <gtkhtml/gtkhtml.h>
#include <gtkhtml/gtkhtml-stream.h>
#include <gtkhtml/htmlselection.h>
#include <string.h>
#include <stdio.h>
#include "win_main.h"
#include "glen_html.h"
#include "win_chat.h"
#include "interface.h"
#include "support.h"
#include "misc.h"
#include "emotes.h"
#include "tlen.h"
#include "prefs.h"
#include "ctype.h"

static GSList chat_list;

static GdkPixbuf *typing_on_pixbuf;
static GdkPixbuf *typing_off_pixbuf;
static GdkPixbuf *blink_pixbuf;

static GtkTooltips *tooltips;

/* przyciski */
static void clear_btn_clicked_cb(GtkButton *button, gpointer user_data);
static void close_btn_clicked_cb(GtkButton *button, gpointer user_data);
static void send_btn_clicked_cb(GtkButton *button, gpointer user_data);
static void enter_sends_btn_cb(GtkToggleButton *button, gpointer user_data);

/* reszta */
static gboolean input_key_press_cb(GtkWidget *widget, GdkEventKey *event,
	gpointer user_data);
static void input_copy_clipboard_cb(GtkTextView *textview, gpointer user_data);
static gboolean delete_win_cb(GtkWidget *, gpointer data);
//static gboolean window_state_cb(GtkWidget *w, GdkEvent *ev, gpointer data);
static gboolean visibility_changed_cb(GtkWidget *w, GdkEvent *ev,
	gpointer data);
static gboolean focus_in_cb(GtkWidget *w, GdkEventFocus *e, gpointer data);

/* miganie ikonka przy nowej wiadomosci */
static gboolean window_icon_blink(gpointer data);

/* po kolei parametry:
   - kolor tla
   - data i czas
   - nazwa rozmowcy
   - id rozmowcy
   - tekst wiadomosci
  */
static const gchar *append_fmt =
"<table style=\"background-color: %s;\" width=\"100%%\" border=\"0\" "
"cellspacing=\"2\" cellpadding=\"0\">"
"<tr><td><font size=-1>%s <b>%s</b> (%s)</font></td></tr>"
"<tr><td>%s</td></tr>"
"</table>";

/* XXX: wrzucic do opcji, a najlepiej z theme brac */
static const gchar *my_bg = "#f0f0f0";
static const gchar *their_bg = "#ffffff";

void chat_init()
{
	warn_if_fail(win_main != NULL);
	
/* wskazniki do tego co zwraca create_pixmap() mozna stracic,
   bo i tak sa potrzebne przez caly program */
	typing_off_pixbuf = gtk_image_get_pixbuf(
		GTK_IMAGE(create_pixmap(NULL, "glen/typing-off.png")));
	typing_on_pixbuf = gtk_image_get_pixbuf(
		GTK_IMAGE(create_pixmap(NULL, "glen/typing-on.png")));
	blink_pixbuf = gtk_image_get_pixbuf(GTK_IMAGE(create_pixmap(NULL,
				"glen/blink.png")));
	
	tooltips = gtk_tooltips_new();
	g_assert(tooltips != NULL);
}

Chat * win_chat_get(const gchar *id, gboolean create, gboolean show)
{
	GSList *l;
	Chat *c = NULL;
	gchar *u1;
	GtkWidget *w;

	g_assert(id != NULL);

	u1 = strip_user(id);

	/* znajdz rozmowe w liscie */
	for(l = &chat_list; l != NULL; l = l->next) {
		c = (Chat *)l->data;
		if(c != NULL) {
			if(g_str_has_prefix(c->id, u1) == TRUE) {
				g_free(u1);
				if(show == TRUE) {
					w = lookup_widget(GTK_WIDGET(c->win), "input");
					gtk_widget_grab_focus(w);
					gtk_widget_show_all(GTK_WIDGET(c->win));
				}
				return c;
			}
		}
	}

	g_free(u1);
	c = NULL;
	if(create == FALSE)
		return c;

	/* rozmowa z tym id nie istnieje, stworz nowa */
	c = win_chat_create(id);
	g_slist_append(&chat_list, c);

	if(show == TRUE) {
		/* Wyrzuc okno na pierwszy plan/aktualny desktop, je¶li jest
		 * ju¿ otwarte */
		gtk_window_present(GTK_WINDOW(c->win));
		gtk_widget_show_all(GTK_WIDGET(c->win));
	}

	win_chat_update_title(c->user);

	return c;
}

Chat * win_chat_create(const gchar *id)
{
	GtkWidget *scroll;
	Chat *c;
	GtkWidget *b;
	gchar *s;
	
	c = (Chat *)g_malloc(sizeof(Chat));
	c->id = g_strdup(id);
	c->enter_sends = pref_chat_enter_sends;
	
	c->blink_state = BlinkOff;
	c->need_blinking = FALSE;

	c->notify_sent = FALSE;

	c->user = user_get(id);

	c->win = GTK_WINDOW(create_win_chat());
	g_assert(c->win != NULL);
	
	scroll = lookup_widget(GTK_WIDGET(c->win), "scroll_output");
	g_assert(scroll != NULL);
	c->input = GTK_TEXT_VIEW(lookup_widget(GTK_WIDGET(c->win), "input"));
	g_assert(c->input != NULL);

	c->output = GTK_HTML(glen_html_new());
	g_assert(c->output != NULL);
	gtk_widget_set_name(GTK_WIDGET(c->output), "output");
	
	gtk_widget_set(GTK_WIDGET(c->output), "can-focus", FALSE, NULL);

	gtk_container_add(GTK_CONTAINER(scroll), GTK_WIDGET(c->output));

	gtk_widget_show(GTK_WIDGET(c->output));

	c->stream = glen_html_begin(c->output);
//	gtk_html_write(c->output, c->stream, header, strlen(header));

/* ustaw pixbuf dla 'lampki' */
	b = lookup_widget(GTK_WIDGET(c->win), "image_typing");
	g_assert(b != NULL);
	gtk_image_set_from_pixbuf(GTK_IMAGE(b), typing_off_pixbuf);

/* callbacki do przyciskow */
	b = lookup_widget(GTK_WIDGET(c->win), "clear_btn");
	g_assert(b != NULL);
	g_signal_connect(G_OBJECT(b), "clicked",
		G_CALLBACK(clear_btn_clicked_cb), (gpointer)c);

	b = lookup_widget(GTK_WIDGET(c->win), "send_btn");
	g_assert(b != NULL);
	g_signal_connect(G_OBJECT(b), "clicked",
		G_CALLBACK(send_btn_clicked_cb), (gpointer)c);

	b = lookup_widget(GTK_WIDGET(c->win), "close_btn");
	g_assert(b != NULL);
	g_signal_connect(G_OBJECT(b), "clicked",
		G_CALLBACK(close_btn_clicked_cb), (gpointer)c);

	b = lookup_widget(GTK_WIDGET(c->win), "enter_sends_btn");
	g_assert(b != NULL);
	g_signal_connect(G_OBJECT(b), "toggled",
		G_CALLBACK(enter_sends_btn_cb), (gpointer)c);
	
/* reszta */
	b = lookup_widget(GTK_WIDGET(c->win), "input");
	g_assert(b != NULL);
	g_signal_connect(G_OBJECT(b), "key_press_event",
		G_CALLBACK(input_key_press_cb), (gpointer)c);
	g_signal_connect(G_OBJECT(b), "copy_clipboard",
		G_CALLBACK(input_copy_clipboard_cb), (gpointer)c);
	
	gtk_widget_grab_focus(b);

	g_signal_connect(G_OBJECT(c->win), "delete_event",
		G_CALLBACK(delete_win_cb), c);
//	g_signal_connect(G_OBJECT(c->win), "window_state_event",
//		G_CALLBACK(window_state_cb), c);
	g_signal_connect_after(G_OBJECT(c->win), "visibility_notify_event",
		G_CALLBACK(visibility_changed_cb), c);
	g_signal_connect(G_OBJECT(c->win), "focus_in_event",
		G_CALLBACK(focus_in_cb), c);

	/* ustaw tooltip dla 'lampki */
	b = lookup_widget(GTK_WIDGET(c->win), "eventbox_typing");
	g_assert(b != NULL);
	s = toutf("Je¶li lampka zapali siê oznacza to, ¿e Twój "
		"rozmówca w³a¶nie pisze wiadomo¶æ");
	gtk_tooltips_set_tip(tooltips, b, s, NULL);
	g_free(s);
	
	return c;
}

void win_chat_set_typing(const gchar *id, gboolean on)
{
	Chat *c;
	GtkWidget *img;
	GdkPixbuf *pb;
	gchar *s;

	c = win_chat_get(id, FALSE, FALSE);
	if(c == NULL)
		return;

	img = lookup_widget(GTK_WIDGET(c->win), "image_typing");
	if(on == TRUE) {
		pb = typing_on_pixbuf;
		s = toutf("Twój rozmówca pisze wiadomo¶æ");
	} else {
		pb = typing_off_pixbuf;
		s = toutf("Je¶li lampka siê zapali oznacza to, ¿e Twój "
			"rozmówca w³a¶nie pisze wiadomo¶æ");
	}

	gtk_image_set_from_pixbuf(GTK_IMAGE(img), pb);
	gtk_widget_queue_draw(img);

	img = lookup_widget(GTK_WIDGET(c->win), "eventbox_typing");

	gtk_tooltips_set_tip(tooltips, img, s, NULL);
	g_free(s);
}

void win_chat_update_title(const User *u)
{
	Chat *c;
	gchar buf[1024];
	gchar *p = NULL;
	
	c = win_chat_get(u->id, FALSE, FALSE);

	if(c == NULL)
		return;

	if(u->desc == NULL)
		snprintf(buf, sizeof(buf), "%s", u->name);
	else {
		snprintf(buf, sizeof(buf), "%s (%s)", u->name, u->desc);
		p = buf;
		/* zastap newline spacjami */
		do {
			if(*p == '\n')
				*p = ' ';
			p++;
		} while(*p != '\0');
	}
	
	gtk_window_set_title(c->win, buf);
	gtk_window_set_icon(c->win, get_status_icon_pixbuf(c->user->status));
}

void win_chat_append(const gchar *id, const gchar *msg, gboolean me,
	const gchar *time)
{
	Chat *c;
	User *u;
	gchar *m = markup_text(msg);
	gchar *name = NULL;
	gchar *tim;

	c = win_chat_get(id, TRUE, TRUE);
	g_assert(c != NULL);

	if(time == NULL)
		tim = (gchar *)get_time(NULL);
	else
		tim = (gchar *)time;
	

	/* XXX: dodawanie usera do listy jako brak autoryzacji */
	u = user_get(id);
	if(u == NULL)
		name = (gchar *)id;
	else
		name = u->name;
	
	if(me)
		gtk_html_stream_printf(c->stream, append_fmt, my_bg,
				tim, pref_chat_my_name, pref_tlen_id, m);
	else
		gtk_html_stream_printf(c->stream, append_fmt, their_bg, 
				tim, name, id, m);

	g_free(m);

	/* scroll */
	gtk_html_flush(c->output);
	gtk_html_command(c->output, "scroll-eod");
	
	if(c->need_blinking == TRUE && c->blink_state == BlinkOff) {
		g_timeout_add(750, window_icon_blink, c);
	}
}

static void clear_btn_clicked_cb(GtkButton *button, gpointer user_data)
{
	Chat *c = (Chat *)user_data;
	gtk_html_end(c->output, c->stream, GTK_HTML_STREAM_OK);
	//gtk_html_stream_destroy(c->stream);
	c->stream = glen_html_begin(c->output);
	//c->stream = gtk_html_begin(c->output);
	//gtk_html_write(c->output, c->stream, header, strlen(header));
}

static void close_btn_clicked_cb(GtkButton *button, gpointer user_data)
{
	Chat *c = (Chat *)user_data;
	gtk_widget_hide_all(GTK_WIDGET(c->win));
}

static void send_btn_clicked_cb(GtkButton *button, gpointer user_data)
{
	gchar *text;
	Chat *c = (Chat *)user_data;
	
	text = text_view_get_text(GTK_TEXT_VIEW(c->input));
	if(strlen(text) != 0) {
		win_chat_append(c->id, text, TRUE, NULL);
		o2_send_chat(c->id, text);
	}
		
	g_free(text);
	text_view_clear(GTK_TEXT_VIEW(c->input));
}

static gboolean input_key_press_cb (GtkWidget *widget, GdkEventKey *event,
	gpointer user_data)
{
	Chat *c = user_data;
	gchar *text;

	switch(event->keyval) {
	case GDK_Return:
	case GDK_KP_Enter:
		if(o2_is_connected() == FALSE) {
			puts("NOT CONNECTED");
			return TRUE;
		}
		if(c->enter_sends == FALSE)
			break;

		text = text_view_get_text(GTK_TEXT_VIEW(widget));
		if(strlen(text) != 0) {
			win_chat_append(c->id, text, TRUE, NULL);
			o2_send_chat(c->id, text);
			win_main_reset_autoaway_timer();
		}
		
		g_free(text);
		text_view_clear(GTK_TEXT_VIEW(widget));
		c->need_blinking = FALSE;
		c->notify_sent = FALSE;
		return TRUE;
		break;
	default:
		if(isascii(event->keyval) && c->notify_sent == FALSE) {
			c->notify_sent = TRUE;
			o2_send_notify(c->id, TLEN_NOTIFY_TYPING);
			puts("sending notify");
		}
		break;
	}

	return FALSE;
}

static void enter_sends_btn_cb(GtkToggleButton *button, gpointer user_data)
{
	Chat *c = (Chat *)user_data;

	c->enter_sends = gtk_toggle_button_get_active(button);
}

static gboolean delete_win_cb(GtkWidget *widget, gpointer data)
{
	Chat *c = (Chat *)data;
	c->need_blinking = FALSE;

	gtk_widget_hide(widget);

	return TRUE;
}

static void input_copy_clipboard_cb(GtkTextView *textview, gpointer user_data)
{
	Chat *c = (Chat *)user_data;
	GtkTextBuffer *buf;
	GtkTextIter start, end;

	/* jesli jest zaznaczenie w input zostawiamy normalne zachowanie */
	buf = gtk_text_view_get_buffer(textview);
	if(gtk_text_buffer_get_selection_bounds(buf, &start, &end) == TRUE) {
		return;
	}

	/* Sprawdzamy, czy jest zaznaczenie w gtkhtml. Sposob jest 'dziwny', innego jak na
	   razie nie znalazlem. Sprawdzanie jest potrzebne, bo gtktml moze miec zaznaczenie
	   rowne "\n" nawet gdy nie jest NIC zaznaczone. W takim wypadku gdy user wciska
	   ctrl+C traci swoj stary schowek na rzecz pustego. A to jest ZLE. */

	if (html_engine_is_selection_active(c->output->engine)) {
		/* w tym miejscu zakladamy, ze skoro nie ma zaznaczenia w input user
		 * chce skopiowac to co jest w widgecie wyjsciowym */
		gtk_html_copy(c->output);
	}
}

static gboolean window_icon_blink(gpointer data)
{
	Chat *c = (Chat *)data;
	gboolean ret;
	GdkPixbuf *pb;

	if(c->need_blinking == FALSE) {
		pb = get_status_icon_pixbuf(c->user->status);
		c->blink_state = BlinkOff;
	//	puts("blink off");
		ret = FALSE;
	} else {
		if(c->blink_state == BlinkAlt) {
			pb = get_status_icon_pixbuf(c->user->status);
			c->blink_state = BlinkNormal;
		} else {
			pb = blink_pixbuf;
			c->blink_state = BlinkAlt;
		}
		
		ret = TRUE;
	}

	gtk_window_set_icon(GTK_WINDOW(c->win), pb);

	return ret;
}
/*
static gboolean window_state_cb(GtkWidget *w, GdkEvent *ev, gpointer data)
{
	GdkEventWindowState *ws = (GdkEventWindowState *)ev;
	Chat *c = (Chat *)data;

	if(ws->new_window_state & GDK_WINDOW_STATE_ICONIFIED ||
		ws->new_window_state & GDK_WINDOW_STATE_BELOW) {
		printf("%s needs blinking\n", c->id);
		c->need_blinking = TRUE;
	} else if(ws->new_window_state & GDK_WINDOW_STATE_ABOVE ||
		ws->new_window_state & ~GDK_WINDOW_STATE_ICONIFIED) {
		printf("%s DOESNT need blinking\n", c->id);
		c->need_blinking = FALSE;
	}

	return TRUE;
	}*/

static gboolean visibility_changed_cb(GtkWidget *w, GdkEvent *ev,
	gpointer data)
{
	GdkEventVisibility *v = (GdkEventVisibility *)ev;
	Chat *c = (Chat *)data;

	if(v->state == GDK_VISIBILITY_UNOBSCURED && 
		c->blink_state == BlinkOff) {
		c->need_blinking = FALSE;
	} else {
		c->need_blinking = TRUE;
	}
	
	return FALSE;
}

static gboolean focus_in_cb(GtkWidget *w, GdkEventFocus *e, gpointer data)
{
	Chat *c = (Chat *)data;
	
	//puts("IN");
	c->need_blinking = FALSE;
	
	return FALSE;
}
