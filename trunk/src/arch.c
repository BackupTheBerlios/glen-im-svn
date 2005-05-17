
#include "misc.h"
#include "arch.h"

/* lista list z wpisami do zrzucenia na dysk */
static GSList *buffers;
/* tak czesto bedziemy zrzucac dane na dysk */
static const guint flush_timeout = 1000;

int arch_append(const gchar *id, const gchar *msg, guint len)
{
	if(len == 0)
		return 0;


}
	
