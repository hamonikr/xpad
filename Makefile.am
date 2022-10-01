SUBDIRS = \
	autopackage \
	data \
	doc \
	images \
	po \
	src

# ACLOCAL_AMFLAGS = -I m4

EXTRA_DIST = \
	autogen.sh

DISTCLEANFILES = \
	xpad.desktop \
	po/xpad.pot \
	Makefile.in \
	aclocal.m4 \
	autom4te.cache/* \
	compile \
	config.h.in \
	configure \
	depcomp \
	install-sh \
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
