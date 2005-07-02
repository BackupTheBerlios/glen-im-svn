## Process this file with automake to produce Makefile.in

INCLUDES = \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	@PACKAGE_CFLAGS@ \
	-I/usr/local/include/libtlen2 -Wall
	
bin_PROGRAMS = glen

glen_SOURCES = \
	arch.c arch.h \
	callbacks.c callbacks.h \
	dialog_auth.c dialog_auth.h \
	dock.c dock.h \
	eggtrayicon.c eggtrayicon.h \
	emotes.c emotes.h \
	glen_html.c glen_html.h \
	interface.c interface.h \
	main.c \
	misc.h misc.c \
	prefs.c prefs.h \
	support.c support.h \
	tlen.c tlen.h \
	user.c user.h \
	win_arch.c win_arch.h \
	win_chat.c win_chat.h \
	win_conf_wizard.c win_conf_wizard.h \
	win_desc.c win_desc.h \
	win_edit.c win_edit.h \
	win_main.c win_main.h \
	win_msg_recv.c win_msg_recv.h \
	win_msg_send.c win_msg_send.h \
	win_prefs.c win_prefs.h


glen_LDADD = @PACKAGE_LIBS@ $(INTLLIBS) -ltlen2 -L/usr/local/lib/

tags: $(glen_SOURCES)
	ctags $(glen_SOURCES)
	cscope -bqu
