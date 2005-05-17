
#include "emotes.h"
#include "prefs.h"
#include "glen_html.h"

static void url_requested_cb(GtkHTML *, const gchar *url,
	GtkHTMLStream *stream);
static void link_clicked_cb(GtkHTML *, const gchar *url,
	gpointer data);

static const gchar *header = 
"<html><head><meta http-equiv=\"Content-Type\" content=\"text/html;"
" charset=\"UTF-8\"></head>"
"<body leftmargin=\"2px\" rightmargin=\"2px\" topmargin=\"2px\" "
"bottommargin=\"2px\" link=\"#0000ff\" alink=\"#ff0000\" vlink=\"#3333ff\">";

GtkWidget *glen_html_new()
{
	GtkWidget *ret;

	ret = gtk_html_new();
	g_assert(ret != NULL);
	
	g_signal_connect(G_OBJECT(ret), "url_requested",
		G_CALLBACK(url_requested_cb), NULL);
	g_signal_connect(G_OBJECT(ret), "link_clicked",
		G_CALLBACK(link_clicked_cb), NULL);

	gtk_html_set_animate(GTK_HTML(ret), TRUE);
	gtk_html_set_images_blocking(GTK_HTML(ret), FALSE);

	return ret;
}

GtkHTMLStream * glen_html_begin(GtkHTML *html)
{
	GtkHTMLStream *stream;
	
	stream = gtk_html_begin(html);
	gtk_html_write(html, stream, header, strlen(header));

	return stream;
}

static void url_requested_cb(GtkHTML *html, const char *url,
	GtkHTMLStream *stream)
{
	Emote *e;

	e = emote_get_by_url(url);
	g_assert(e != NULL);
	
	gtk_html_write(html, stream, e->data, e->size);
	gtk_html_flush(html);
}

static void link_clicked_cb(GtkHTML *html, const gchar *url,
	gpointer data)
{
	gchar buf[2048];

	snprintf(buf, sizeof(buf), pref_misc_browser_cmd, url);

	g_spawn_command_line_async(buf, NULL);
}

