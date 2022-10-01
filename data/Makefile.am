desktopdir = $(datadir)/applications
desktop_in_files = xpad.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)

appdatadir = $(datadir)/metainfo
appdata_in_files = xpad.appdata.xml.in
appdata_DATA = $(appdata_in_files:.xml.in=.xml)

CLEANFILES = \
	$(appdata_DATA) \
	$(desktop_DATA)

EXTRA_DIST = \
	$(appdata_in_files) \
	$(desktop_in_files)

$(desktop_DATA): $(desktop_in_files)
	$(AM_V_GEN)$(MSGFMT) --desktop --template $< -d $(top_srcdir)/po -o $@

$(appdata_DATA): $(appdata_in_files)
	$(AM_V_GEN)$(MSGFMT) --xml --template $< -d $(top_srcdir)/po -o $@
