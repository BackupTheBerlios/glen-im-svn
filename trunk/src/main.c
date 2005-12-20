
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gnome.h>

#include "win_main.h"
#include "arch.h"
#include "tlen.h"
#include "support.h"
#include "emotes.h"
#include "misc.h"
#include "dock.h"
#include "prefs.h"

// XXX
#include "interface.h"
#include "dialog_auth.h"
GtkWindow * tester_create();

int main(int argc, char *argv[])
{
	gchar *desc;
#ifdef ENABLE_NLS
	bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);
#endif

	gnome_program_init (PACKAGE, VERSION, LIBGNOMEUI_MODULE,
	                    argc, argv,
	                    GNOME_PARAM_APP_DATADIR, PACKAGE_DATA_DIR,
	                    NULL);

	printf("GNOME_PARAM_APP_DATADIR=%s, PACKAGE_DATA_DIR=%s\n", GNOME_PARAM_APP_DATADIR, PACKAGE_DATA_DIR);

	/* XXX: wizard konfiguracji gdy nie ma ustawien -- pierwsze
	 * uruchomienie */
	if(prefs_load() == FALSE)
		printf("prefs_load() == FALSE\n");
	
	arch_init();

	win_main_create();
	chat_init();
	gtk_widget_show(win_main);

	if(pref_chat_use_emotes == TRUE)
		pref_chat_use_emotes = emote_load_set(pref_chat_emote_set);


	dock_create();

//	tlen_debug_set(TLEN_DEBUG_HIGH);

	desc = NULL;

	if(pref_status_save_desc == TRUE)
		if(pref_status_desc != NULL && strlen(pref_status_desc) != 0)
			desc = g_strdup(pref_status_desc);

	if(pref_status_save_status == TRUE)
		o2_set_status(pref_status_saved, desc);
	else
		o2_set_status(pref_status_on_start, desc);

	if(desc != NULL)
		g_free(desc);

	gtk_main ();
	return 0;
}

