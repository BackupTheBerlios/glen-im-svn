
#ifndef PREFS_H
#define PREFS_H

#include <gnome.h>
#include <glib.h>

#define PREFS_FILE_FMT "%s/.glen/prefs"

/* tlen */
extern gchar *pref_tlen_id;
extern gchar *pref_tlen_pass;

/* status */
extern gboolean pref_status_save_status;
extern guint pref_status_saved;
extern guint pref_status_on_start;
extern gboolean pref_status_save_desc;
extern gchar *pref_status_desc;
extern gboolean pref_status_auto_away;
extern gboolean pref_status_auto_away_desc; /* opis przy auto-away? */
extern guint pref_status_auto_away_time; /* sekundy */
extern guint pref_status_auto_bbl_time; /* dla 'wroce pozniej' */

/* chat */
extern gchar *pref_chat_my_name;
/* czy pokazywac id w czacie: Ja (sigsegv@o2.pl) */
extern gboolean pref_chat_show_ids;
extern gboolean pref_chat_use_emotes;
extern gchar *pref_chat_emote_set;
extern gboolean pref_chat_enter_sends;

/* userlista */
extern gboolean pref_userlist_autoexpand; /* rozwijanie grup */

/* rozne */
extern gchar *pref_misc_browser_cmd;

/* Okno glowne */
/* Pozycja okna */
extern guint pref_win_main_pos_x;
extern guint pref_win_main_pos_y;
/* Rozmiar okna glownego. */
extern guint pref_win_main_width;
extern guint pref_win_main_height;

gboolean prefs_load();
gboolean prefs_save();

#endif
