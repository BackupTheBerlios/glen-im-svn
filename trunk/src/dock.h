
#ifndef DOCK_H
#define DOCK_H

#include <gnome.h>
#include "eggtrayicon.h"

extern GtkWidget *dock_widget;

void dock_create();
void dock_set_status(guint status, const gchar *desc);

#endif

