
#include "win_main.h"
#include "win_msg_recv.h"
#include "win_chat.h"
#include "tlen.h"
#include "prefs.h"
#include "misc.h"
#include "dock.h"
#include "dialog_auth.h"

static guint o2_status = TLEN_PRESENCE_UNAVAILABLE;
static gchar *o2_desc = NULL;

static TlenConnection *tc = NULL;
static gboolean is_connected;

static gboolean user_goes_unavail = FALSE;

static void parse_event(TlenConnection *);
static void event_roster(TlenEvent *e);
static void event_presence(TlenEvent *e);
static void event_message(TlenEvent *e);
static void event_notify(TlenEvent *e);

static void event_subscribe(TlenEvent *);
static void event_subscribed(TlenEvent *e);
static void event_unsubscribe(TlenEvent *e);
static void event_unsubscribed(TlenEvent *e);

gboolean o2_login()
{
	gchar *strip;
	gboolean ret;
	
	if(tc != NULL)
		tlen_connection_delete(tc);

	is_connected = FALSE;

	strip = strip_user(pref_tlen_id);

	tc = tlen_connection_new(strip, pref_tlen_pass);
	g_assert(tc != NULL);

	ret = tlen_connection_open(tc, parse_event);
	win_main_start_icon_blinking(o2_status);

	g_free(strip);

	return ret;
}

gboolean o2_is_connected()
{
	return is_connected;
}

static void parse_event(TlenConnection *c)
{
	TlenEvent *e;

	g_assert(c != NULL);

	while((e = tlen_event_get(c))) {
		switch(e->type) {
		case TLEN_EVENT_MESSAGE:
			event_message(e);
			break;
		case TLEN_EVENT_PRESENCE:
			switch(tlen_presence_type(e)) {
			case TLEN_PRESENCE_STATUS:
				event_presence(e);
				break;
			case TLEN_PRESENCE_SUBSCRIBED:
				event_subscribed(e);
				break;
			case TLEN_PRESENCE_SUBSCRIBE:
				event_subscribe(e);
				break;
			case TLEN_PRESENCE_UNSUBSCRIBE:
				event_unsubscribe(e);
				break;
			case TLEN_PRESENCE_UNSUBSCRIBED:
				event_unsubscribed(e);
				break;
			default:
				break;
			}
			break;
		case TLEN_EVENT_NOTIFY:
			event_notify(e);
			break;
		case TLEN_EVENT_ROSTER:
			tlen_presence(tc, o2_status, o2_desc);
			event_roster(e);
			break;
		case TLEN_EVENT_LOGGED:
			tlen_roster_get(c);
			is_connected = TRUE;
			break;
		case TLEN_EVENT_UNAUTHORIZED:
			g_warning("TLEN_EVENT_UNAUTHORIZED");
			g_warning("Zle haslo znaczy sie");
			prefs_save();
			exit(EXIT_FAILURE);
			break;
		case TLEN_EVENT_CONNECTED:
			puts("Connected.");
			win_main_set_status(o2_status, o2_desc);
			break;
		case TLEN_EVENT_DISCONNECTED:
			g_warning("TLEN_EVENT_DISCONNECTED");
			is_connected = FALSE;
			if(user_goes_unavail == FALSE)
				o2_login();
			break;
		case TLEN_EVENT_CONNECTION_FAILED:
			is_connected = FALSE;
			g_warning("TLEN_EVENT_CONNECTION_FAILED");
			o2_login();
			break;
			default:
				g_warning("EVENT: %i", e->type);
				break;
		}

		tlen_event_free(e);
	}
}

static void event_roster(TlenEvent *e)
{
	GSList *l;
	TlenRosterItem *i;
	gchar *name, *id, *group, *p;
	User *u;
	Group *g;

	for(l = TLEN_EVENT_ROSTER(e)->list; l != NULL; l = l->next) {
		i = (TlenRosterItem *)l->data;
		name = group = NULL;
		if(i == NULL || i->jid == NULL)
			continue;

		id = toutf(i->jid);
		if(user_get(id) != NULL) {
			g_free(id);
			continue;
		}

		if(i->name != NULL) {
			p = toutf(i->name);
			name = g_markup_escape_text(p, -1);
			g_free(p);
		}
			
		u = user_new_full(id, name, NULL, TLEN_PRESENCE_UNAVAILABLE);

		if(i->subscription) { 
			if(g_strcasecmp(i->subscription, "none") == 0 ||
				g_strcasecmp(i->subscription, "to") == 0) {
				group = g_strdup(GROUP_NAME_WAIT_AUTH);
				USER_FLAG_SET(u, UserFlagNoSubscribtion);
			} else if(g_strcasecmp(i->subscription,
					"remove") == 0) {
				continue;
			} else if(g_strcasecmp(i->subscription, "from") == 0) {
				group = g_strdup(GROUP_NAME_NO_AUTH);
				USER_FLAG_SET(u, UserFlagWantsSubscribtion);
			} else if(g_strcasecmp(i->subscription, "both") == 0) {
				USER_FLAG_CLEAR(u, UserFlagNoSubscribtion);
				USER_FLAG_CLEAR(u, UserFlagWantsSubscribtion);
			}
		}
		
		if(i->group != NULL && group == NULL) {
			p = toutf(i->group);
			group = g_markup_escape_text(p, -1);
			g_free(p);
		}

		g = group_get(group);
		if(g == NULL)
			g = group_new(group);

		user_set_group(u, g);

		win_main_user_add(u);

		printf("roster from:%s subscribtion:%s ask:%s\n", 
			id, i->subscription, i->ask);
		
		g_free(name);
		g_free(id);
		g_free(group);
	}
}

static gchar *parse_time(const gchar *time)
{
	static gchar buf[40];
	gchar y[5], m[3], d[3], t[15];
	
	if(time == NULL)
		return NULL;

	y[4] = m[2] = d[2] = t[15] = 0;

	sscanf(time, "%4s%2s%2sT%8s", y, m, d, t);
	snprintf(buf, sizeof(buf), "%s/%s/%s %s", d, m, y, t);

	return buf;
}

static void event_message(TlenEvent *e)
{
	gchar *from, *body, *time;
	gchar *parsed_time = NULL;
	guint type;
	TlenEventMessage *m;
	User *u;
	
	m = TLEN_EVENT_MESSAGE(e);
	time = body = from = NULL;

	if(m->body == NULL)
		printf("message from '%s': '%s'\n", m->from, m->body);

	from = toutf(m->from);
	win_chat_set_typing(from, FALSE);
	if(m->body == NULL) {
		g_free(from);
		return;
	}

	body = toutf(m->body);
	if(m->stamp) {
		time = toutf(m->stamp);
		parsed_time = parse_time(time);
	}
	type = m->type;

	if(type == TLEN_MESSAGE_TYPE_CHAT) {
		win_chat_append(from, body, FALSE, parsed_time);
	} else {
		u = user_get(from);
		win_msg_recv_add(from, (u == NULL ? NULL : u->name), body, 
			(parsed_time == NULL ? get_time(NULL) : parsed_time));
	}

	g_free(from);
	g_free(body);
	g_free(time);
}

static void event_presence(TlenEvent *e)
{
	TlenEventPresence *ev;
	gchar *desc, *type, *from;
	gchar *p;
	User *u;
	Group *g;

	p = desc = from = type = NULL;

	ev = TLEN_EVENT_PRESENCE(e);

	from = toutf(ev->from);

	//printf("presence from: %s:%s\n", from, type);

	if(ev->description != NULL) {
		desc = toutf(ev->description);
	}

	if(ev->type != NULL)
		type = toutf(ev->type);

	printf("presence from '%s', desc: '%s', type: '%s'\n", from, desc,
		type);

	u = user_get(from);

	/* jesli nie znaleziono usera, to znaczy ze ktos ma nas na liscie,
	 * ale my jego nie. Zaskakujace. */
	if(u != NULL) {
		g = u->group;
		
		if(ev->status != u->status) {
			if(o2_changed_to_avail(ev->status, u->status)) {
				g->child_active_count++;
			} else if(o2_changed_to_unavail(ev->status, u->status)) {
				if(ev->status == TLEN_PRESENCE_UNAVAILABLE)
					win_chat_set_typing(from, FALSE);
				g->child_active_count--;
			}
		}

		user_set_status(u, desc, ev->status);
		win_main_user_update(u);
	}

	g_free(from);
	g_free(desc);
	g_free(type);
}

static void event_notify(TlenEvent *e)
{
	TlenEventNotify *ev;
	gchar *f;

	ev = TLEN_EVENT_NOTIFY(e);

	f = toutf(ev->from);

	if(ev->type != TLEN_NOTIFY_ALERT)
		win_chat_set_typing(f, ev->type == TLEN_NOTIFY_TYPING);

	g_free(f);
}

static void event_subscribe(TlenEvent *event)
{
	gchar *from, *type;
	TlenEventPresence *e;

	e = TLEN_EVENT_PRESENCE(event);

	from = toutf(e->from);
	type = toutf(e->type);

	printf("event_subscribe: from=%s, type=%s\n", from, type);

	if(from != NULL && type != NULL) {
		User *u = user_get(from);
		
		o2_subscribe_accept(from);
		if(u == NULL) {
		/* jesli user byl na liscie, wysylamy bezwarunkowo
		 * potwierdzenie, inaczej wyswietlamy dialog, ktory
		 * ewentualnie wysle odmowe */
			dialog_auth_create(from);
		}
	}
	
	g_free(from);
	g_free(type);
}

static void event_subscribed(TlenEvent *event)
{
	gchar *from, *type;
	TlenEventPresence *e;
	User *u = NULL;
	Group *g = NULL;

	e = TLEN_EVENT_PRESENCE(event);

	from = toutf(e->from);
	type = toutf(e->type);

	printf("event_subscribed: from=%s, type=%s\n", from, type);

	/*
	if(from != NULL && type != NULL) {
		if(user_get(from) != NULL) {
		}
	}
	*/

	u = user_get(from);
	g = group_get(NULL);
	USER_FLAG_CLEAR(u, UserFlagNoSubscribtion);
	win_main_user_set_group(u, g);
	user_set_group(u, g);

	g_free(from);
	g_free(type);
}

static void event_unsubscribe(TlenEvent *event)
{
	gchar *from, *type;
	TlenEventPresence *e;

	e = TLEN_EVENT_PRESENCE(event);

	from = toutf(e->from);
	type = toutf(e->type);

	printf("event_unsubscribe: from=%s, type=%s\n", from, type);

	if(from != NULL && type != NULL) {
		if(user_get(from) != NULL) {
		}
	}

	g_free(from);
	g_free(type);
}

static void event_unsubscribed(TlenEvent *event)
{
	gchar *from, *type;
	TlenEventPresence *e;

	e = TLEN_EVENT_PRESENCE(event);

	from = toutf(e->from);
	type = toutf(e->type);

	printf("event_unsubscribed: from=%s, type=%s\n", from, type);

	if(from != NULL && type != NULL) {
		if(user_get(from) != NULL) {
		}
	}

	g_free(from);
	g_free(type);
}

gboolean o2_set_status(guint status, const gchar *desc)
{
	gchar *s = NULL;
	gboolean ret;
	
	if(status != TLEN_PRESENCE_UNAVAILABLE) {
		o2_status = status;
		user_goes_unavail = FALSE;
		if(is_connected == FALSE)
			o2_login();
	} else {
		user_goes_unavail = TRUE;
	}

	if(o2_desc != NULL)
		g_free(o2_desc);
	
	if(desc != NULL) {
		o2_desc = g_strdup(desc);
		s = fromutf(desc);
	} else {
		o2_desc = NULL;
	}

	printf("desc='%s', o2_desc='%s'\n", desc, o2_desc);

	win_main_set_status(status, desc);
	dock_set_status(status, desc);

	ret = tlen_presence(tc, status, s);

	g_free(s);

	return ret;
}

guint o2_get_status()
{
	if(tc != NULL)
		return tc->status;
	else
		return TLEN_PRESENCE_UNAVAILABLE;
}

const gchar *o2_get_desc()
{
	if(tc == NULL)
		return NULL;
	else
		return tc->description;
}

gboolean o2_subscribe_accept(const gchar *id)
{
	gchar *s;
	gboolean ret;

	s = fromutf(id);

	ret = tlen_subscribe_accept(tc, s);

	g_free(s);

	return ret;
}

gboolean o2_subscribe_request(const gchar *id)
{
	gchar *s;
	gboolean ret;

	s = fromutf(id);

	ret = tlen_subscribe_request(tc, s);

	g_free(s);

	return ret;
}

gboolean o2_unsubscribe_request(const gchar *id)
{
	gchar *s;
	gboolean ret;

	s = fromutf(id);

	ret = tlen_unsubscribe_request(tc, s);

	g_free(s);

	return ret;
}

gboolean o2_send_chat(const gchar *id, const gchar *msg)
{
	gchar *m = fromutf(msg);
	gboolean ret;

	ret = tlen_message(tc, id, m, TLEN_MESSAGE_TYPE_CHAT);

	g_free(m);

	return ret;
}

gboolean o2_send_msg(const gchar *id, const gchar *msg)
{
	gchar *m = fromutf(msg);
	gboolean ret;

	ret = tlen_message(tc, id, m, TLEN_MESSAGE_TYPE_NORMAL);

	g_free(m);

	return ret;
}

gboolean o2_send_notify(const gchar *id, TlenNotifyType type)
{
	return tlen_notify(tc, id, type);
}

gboolean o2_changed_to_avail(guint new, guint old)
{
	gboolean ret = FALSE;

	if(new == old)
		return ret;

	if(old == TLEN_PRESENCE_UNAVAILABLE ||
		old == TLEN_PRESENCE_INVISIBLE) {
		if(new != TLEN_PRESENCE_UNAVAILABLE &&
			new != TLEN_PRESENCE_INVISIBLE)
			ret = TRUE;
	}
	
	return ret;
}

gboolean o2_changed_to_unavail(guint new, guint old)
{
	gboolean ret = FALSE;

	if(new == old)
		return ret;

	if(old != TLEN_PRESENCE_INVISIBLE && 
		old != TLEN_PRESENCE_UNAVAILABLE) {
		if(new == TLEN_PRESENCE_INVISIBLE ||
			new == TLEN_PRESENCE_UNAVAILABLE)
			ret = TRUE;
	}

	return ret;
}

gboolean o2_status_is_avail(guint s)
{
	if(s != TLEN_PRESENCE_UNAVAILABLE &&
		s != TLEN_PRESENCE_INVISIBLE)
		return TRUE;
	
	return FALSE;
}

gboolean o2_contact_add(const gchar *id, const gchar *name,
	const gchar *group)
{
	gchar *i, *n, *g;
	User *u;
	gboolean ret;

	i = fromutf(id);
	u = user_get(id);
	
	if(USER_FLAG_ISSET(u, UserFlagNoName))
		n = g_strdup("");
	else
		n = fromutf(name);
	
	if(group == NULL)// || g_strcasecmp(group, "Kontakty") == 0)
		g = g_strdup("Kontakty");
	else
		g = fromutf(group);

	ret = tlen_contact_add(tc, i, n, g);

	g_free(i);
	g_free(n);
	g_free(g);

	return ret;
}

gboolean o2_contact_remove(const gchar *id)
{
	gboolean ret;
	gchar *i;

	i = fromutf(id);

	ret = tlen_contact_remove(tc, i);

	g_free(i);

	return ret;
}

guint o2_presence_to_index(guint status)
{
	guint ret = 0;

	switch(status) {
	case TLEN_PRESENCE_AVAILABLE:
		ret = 0;
		break;
	case TLEN_PRESENCE_CHATTY:
		ret = 1;
		break;
	case TLEN_PRESENCE_DND:
		ret = 2;
		break;
	case TLEN_PRESENCE_AWAY:
		ret = 3;
		break;
	case TLEN_PRESENCE_EXT_AWAY:
		ret = 4;
		break;
	case TLEN_PRESENCE_INVISIBLE:
		ret = 5;
		break;
	case TLEN_PRESENCE_UNAVAILABLE:
		ret = 6;
		break;
	}

	return ret;
}

