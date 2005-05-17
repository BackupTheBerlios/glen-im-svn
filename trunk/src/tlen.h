
#include <libtlen2/libtlen2.h>

gboolean o2_login();
gboolean o2_send_chat(const gchar *id, const gchar *msg);
gboolean o2_send_msg(const gchar *id, const gchar *msg);
gboolean o2_is_connected();
gboolean o2_send_notify(const gchar *id, TlenNotifyType t);
gboolean o2_set_status(guint status, const gchar *desc);
guint o2_get_status();
const gchar *o2_get_desc();
/* TRUE jesli przeszedl z niedostepny/niewidoczny na inny */
gboolean o2_changed_to_avail(guint new_status, guint old_status);
gboolean o2_changed_to_unavail(guint new_status, guint old_status);
gboolean o2_status_is_avail(guint status);
gboolean o2_contact_add(const gchar *id, const gchar *name,
	const gchar *group);
gboolean o2_contact_remove(const gchar *id);

gboolean o2_subscribe_accept(const gchar *id);
gboolean o2_subscribe_request(const gchar *id);
gboolean o2_unsubscribe_request(const gchar *id);

guint o2_presence_to_index(guint status);

