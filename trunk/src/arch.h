
#ifndef ARCH_H
#define ARCH_H

enum ArchEntryType
{
	FromMe,
	ToMe,
	Info
};

struct structArchEntry
{
	enum ArchEntryType type;
	time_t time;
	void *data;
};

struct structChatEntry
{
	guint fromlen;
	gchar *from;
	guint textlen;
	gchar *text;
}

struct structInfoEntry
{
	guint textlen;
	gchar *text;
}
	
typedef struct structChatEntry ChatEntry;
typedef struct structInfoEntry InfoEntry;
typedef struct structArchEntry ArchEntry;

#endif

