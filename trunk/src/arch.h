/* vim: set tw=0: */

#ifndef ARCH_H
#define ARCH_H

enum ArchEntryType
{
	ChatFromMe,		/* Tekst w rozmowie kt�ry ja wys�a�em */
	ChatToMe,		/* Tekst od rozm�wcy */
	Info,			/* Info wstawione do okienka przez program */
	ConversationStart,	/* Pocz�tek rozmowy */
	MessageFromMe,		/* Wiadomo�� ode mnie */
	MessageToMe		/* Wiadomo�� do mnie */
};

/* Jeden wpis w pliku z histori� */
struct structArchEntry {
	enum ArchEntryType type;	/* Typ. Od tego pola zale�y jak interpretujemy reszt� */
	time_t time;			/* Czas wyst�pienia zdarzenia */
	guint textlen;			/* D�ugo�� ewentualnej wiadomo�ci */
	gchar *text;			/* Wiadomo�� */
};

#endif

