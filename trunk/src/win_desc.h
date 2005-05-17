
#ifndef WIN_DESC_H
#define WIN_DESC_H

#include <gnome.h>

/* ustawiane po zamknieciu okna */
extern GtkWidget *win_desc;
extern gint win_desc_status;
extern gchar *win_desc_desc;

void win_desc_show();

#endif

