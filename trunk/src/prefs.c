
#include <libtlen2/libtlen2.h>
#include "prefs.h"
#include "tlen.h"
#include "misc.h"

/* tlen */
gchar *pref_tlen_id = "twoj_login@o2.pl";
gchar *pref_tlen_pass = "twoje_haslo";

/* status */
gboolean pref_status_save_status = FALSE;
guint pref_status_saved = TLEN_PRESENCE_AVAILABLE;
guint pref_status_on_start = TLEN_PRESENCE_AVAILABLE; /* dostepny */
gboolean pref_status_save_desc = FALSE;
gchar *pref_status_desc = NULL;
gboolean pref_status_auto_away = TRUE;
gboolean pref_status_auto_away_desc = TRUE; /* opis przy auto-away? */
guint pref_status_auto_away_time = 2;
guint pref_status_auto_bbl_time = 10;

/* userlist */
gboolean pref_userlist_autoexpand = TRUE;

/* chat */
gchar *pref_chat_my_name = "Ja";
gboolean pref_chat_show_ids = TRUE;
gboolean pref_chat_use_emotes = TRUE;
gchar *pref_chat_emote_set = "standardowy";
gboolean pref_chat_enter_sends = TRUE;

/* misc */
gchar *pref_misc_browser_cmd = "gnome-moz-remote --newwin %s";

static GKeyFile *pref_key_file = NULL;

#define pref_get_val(group, var, type) \
if(g_key_file_has_key(pref_key_file, group, #var, NULL)) \
	var = g_key_file_get_ ##type(pref_key_file, group, #var, NULL);

#define pref_set_val(group, var, type) \
	g_key_file_set_ ##type(pref_key_file, group, #var, var);

gboolean prefs_load()
{
	GKeyFileFlags flags;
	gchar *home;
	gchar path[1024];

	home = (gchar *)g_get_home_dir();
	if(home == NULL) {
		g_warning("Nie mozna odnalezc katalogu domowego.\n"
			"Ustawienia nie beda zaladowane.\n");
		return FALSE;
	}

	snprintf(path, sizeof(path), PREFS_FILE_FMT, home);

	if(pref_key_file == NULL)
		pref_key_file = g_key_file_new();

	flags = 0;
	if(g_key_file_load_from_file(pref_key_file, path, flags, NULL) == FALSE) {
		return FALSE;
	}

	/* pierwsza grupa musi byc 'tlen' */
	if(g_strcasecmp(g_key_file_get_start_group(pref_key_file), "tlen") != 0) {
		return FALSE;
	}

	pref_get_val("tlen", pref_tlen_id, string);
	pref_get_val("tlen", pref_tlen_pass, string);

	pref_get_val("status", pref_status_save_status, boolean);
	pref_get_val("status", pref_status_saved, integer);
	pref_get_val("status", pref_status_on_start, integer);
	pref_get_val("status", pref_status_save_desc, boolean);
	pref_get_val("status", pref_status_desc, string);
	printf("%s\n\n", pref_status_desc);
	//home = pref_status_desc;
	//pref_status_desc = toutf(pref_status_desc);
	//if(pref_status_desc == NULL)
	//	pref_status_desc = home;
	//else
	//	g_free(home);
	pref_get_val("status", pref_status_auto_away, boolean);
	pref_get_val("status", pref_status_auto_away_desc, boolean);
	pref_get_val("status", pref_status_auto_away_time, integer);
	pref_get_val("status", pref_status_auto_bbl_time, integer);
	
	pref_get_val("userlist", pref_userlist_autoexpand, boolean);

	pref_get_val("chat", pref_chat_my_name, string);
	pref_get_val("chat", pref_chat_show_ids, boolean);
	pref_get_val("chat", pref_chat_use_emotes, boolean);
	pref_get_val("chat", pref_chat_emote_set, string);
	pref_get_val("chat", pref_chat_enter_sends, boolean);

	pref_get_val("misc", pref_misc_browser_cmd, string);

	return TRUE;
}

gboolean prefs_save()
{
	gchar *home;
	gchar path[1024];
	gsize len;
	FILE *fp;

	home = (gchar *)g_get_home_dir();
	if(home == NULL) {
		/* XXX: okienko */
		g_warning("Nie mozna odnalezc katalogu domowego.\n"
			"Ustawienia nie beda zachowane.");
		return FALSE;
	}

	snprintf(path, sizeof(path), "%s/.glen", home);
	/* na chama ;] */
	mkdir(path, 0700);

	snprintf(path, sizeof(path), PREFS_FILE_FMT, home);

	if(pref_key_file == NULL)
		pref_key_file = g_key_file_new();
	g_assert(pref_key_file != NULL);

	g_key_file_set_comment(pref_key_file, NULL, NULL,
		" Zmieniaj plik na wlasna odpowiedzialnosc", NULL);

	pref_set_val("tlen", pref_tlen_id, string);
	pref_set_val("tlen", pref_tlen_pass, string);

	pref_set_val("status", pref_status_save_status, boolean);
	pref_set_val("status", pref_status_saved, integer);
	pref_set_val("status", pref_status_on_start, integer);
	pref_set_val("status", pref_status_save_desc, boolean);
	if(pref_status_desc != NULL) {
		home = pref_status_desc;
		pref_status_desc = toutf(pref_status_desc);
		pref_set_val("status", pref_status_desc, string);
		g_free(home);
	}
	pref_set_val("status", pref_status_auto_away, boolean);
	pref_set_val("status", pref_status_auto_away_desc, boolean);
	pref_set_val("status", pref_status_auto_away_time, integer);
	pref_set_val("status", pref_status_auto_bbl_time, integer);

	pref_set_val("userlist", pref_userlist_autoexpand, boolean);

	pref_set_val("chat", pref_chat_my_name, string);
	pref_set_val("chat", pref_chat_emote_set, string);
	pref_set_val("chat", pref_chat_show_ids, boolean);
	pref_set_val("chat", pref_chat_use_emotes, boolean);
	pref_set_val("chat", pref_chat_enter_sends, boolean);

	pref_set_val("misc", pref_misc_browser_cmd, string);
	
	fp = fopen(path, "w");
	if(fp == NULL) {
		/* XXX: okienko */
		g_warning("fopen()");
		g_key_file_free(pref_key_file);
		return FALSE;
	}

	home = g_key_file_to_data(pref_key_file, &len, NULL);
	fwrite(home, len, 1, fp);
	fclose(fp);
	g_free(home);
	chmod(path, 0600);
	
	return TRUE;
}

