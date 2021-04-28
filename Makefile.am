SUBDIRS = \
	autopackage \
	data \
	doc \
	images \
	po \
	src

# ACLOCAL_AMFLAGS = -I m4

EXTRA_DIST = \
	autogen.sh \
	intltool-extract.in \
	intltool-merge.in \
	intltool-update.in

DISTCLEANFILES = \
	xpad.desktop \
	intltool-extract \
	intltool-merge \
	intltool-update \
	po/xpad.pot \
	Makefile.in \
	aclocal.m4 \
	autom4te.cache/* \
	compile \
	config.h.in \
	configure \
	depcomp \
	install-sh \
	intltool-extract.in \
	intltool-merge.in \
	intltool-update.in \
	missing \
	autopackage/Makefile.in \
	doc/Makefile.in \
	images/Makefile.in \
	po/Makefile.in \
	src/Makefile.in \
	*.in~ \
	*.~1~

# Distribute pot file
dist-hook:
	$(MAKE) -C "$(srcdir)/po" "$(GETTEXT_PACKAGE).pot"
	cp "$(srcdir)/po/$(GETTEXT_PACKAGE).pot" "$(distdir)/po/"
