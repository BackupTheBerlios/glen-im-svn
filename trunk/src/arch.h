/* vim: set tw=0: */

#ifndef ARCH_H
#define ARCH_H

enum ArchEntryType
{
	ChatFromMe,		/* Tekst w rozmowie który ja wys³a³em */
	ChatToMe,		/* Tekst od rozmówcy */
	Info,			/* Info wstawione do okienka przez program */
	ConversationStart,	/* Pocz±tek rozmowy */
	MessageFromMe,		/* Wiadomo¶æ ode mnie */
	MessageToMe		/* Wiadomo¶æ do mnie */
};

/* Jeden wpis w pliku z histori± */
struct structArchEntry {
	enum ArchEntryType type;	/* Typ. Od tego pola zale¿y jak interpretujemy resztê */
	time_t time;			/* Czas wyst±pienia zdarzenia */
	guint textlen;			/* D³ugo¶æ ewentualnej wiadomo¶ci */
	gchar *text;			/* Wiadomo¶æ */
};

#endif

