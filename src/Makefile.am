bin_PROGRAMS = xpad

xpad_SOURCES = \
	fio.c fio.h \
	help.c help.h \
	prefix.c prefix.h \
	xpad-app.c xpad-app.h \
	xpad-grip-tool-item.c xpad-grip-tool-item.h \
	xpad-pad.c xpad-pad.h \
	xpad-pad-group.c xpad-pad-group.h \
	xpad-pad-properties.c xpad-pad-properties.h \
	xpad-periodic.c xpad-periodic.h \
	xpad-preferences.c xpad-preferences.h \
	xpad-search-bar.c xpad-search-bar.h \
	xpad-session-manager.c xpad-session-manager.h \
	xpad-settings.c xpad-settings.h \
	xpad-styling-helpers.c xpad-styling-helpers.h \
	xpad-text-buffer.c xpad-text-buffer.h \
	xpad-text-view.c xpad-text-view.h \
	xpad-toolbar.c xpad-toolbar.h \
	xpad-tray.c xpad-tray.h

AM_CFLAGS = @GTK_CFLAGS@ @DEBUG_CFLAGS@ @GTK_SOURCE_VIEW_CFLAGS@ -DDATADIR=\"$(datadir)\" -DHELPDIR=\"@HELP_DIR@\" -DHELPFILENAME=\"@HELP_FILE_NAME@\" -DLOCALEDIR=\"@LOCALE_DIR@\" -DTHEMEDIR=\"@THEME_DIR@\"
xpad_LDADD = @GTK_LIBS@ @BINRELOC_LIBS@ @GTK_SOURCE_VIEW_LIBS@

if !OS_WIN32
  AM_CFLAGS += @X_CFLAGS@
  xpad_LDADD += @X_PRE_LIBS@ @X_LIBS@ @X_EXTRA_LIBS@
endif

if HAVE_APP_INDICATOR
  AM_CFLAGS += -DHAVE_APP_INDICATOR @APP_INDICATOR_CFLAGS@
  xpad_LDADD += @APP_INDICATOR_LIBS@
endif
