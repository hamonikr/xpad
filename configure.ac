# Process this file with autoconf to produce a configure script.

# Sets up autoconf.
AC_INIT([Xpad],[5.4],[xpad-hackers@lists.launchpad.net])
AC_CONFIG_SRCDIR(src/xpad-app.c)
AC_CONFIG_HEADERS([config.h:config.h.in])

# Sets up automake.
AM_INIT_AUTOMAKE([1.9 gnu no-dist-gzip dist-bzip2])
AM_BINRELOC

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_MAKE_SET
AC_PROG_INTLTOOL([0.31], [no-xml])

AC_PATH_XTRA

# Checks for GTK+ 3.0
PKG_CHECK_MODULES(GTK, gtk+-3.0 >= 3.22 gio-2.0 >= 2.56)
AM_CONDITIONAL(GTK, test -n "$GTK_LIBS")
AC_SUBST(GTK_LIBS)
AC_SUBST(GTK_CFLAGS)

PKG_CHECK_MODULES(GTK_SOURCE_VIEW, gtksourceview-3.0 >= 3.10)
AC_SUBST(GTK_SOURCE_VIEW_LIBS)
AC_SUBST(GTK_SOURCE_VIEW_CFLAGS)

PKG_CHECK_MODULES(GLIB, glib-2.0 >= 2.40)
AC_SUBST(GLIB_LIBS)
AC_SUBST(GLIB_CFLAGS)

PKG_CHECK_MODULES(PANGO, pango >= 1.32)
AC_SUBST(PANGO_LIBS)
AC_SUBST(PANGO_CFLAGS)

PKG_CHECK_MODULES(PIXBUF, gdk-pixbuf-2.0 >= 2.28)
AC_SUBST(PIXBUF_LIBS)
AC_SUBST(PIXBUF_CFLAGS)

PKG_CHECK_MODULES(ATK, atk >= 2.8 atk-bridge-2.0)
AC_SUBST(ATK_LIBS)
AC_SUBST(ATK_CFLAGS)

# Force the use of GNU libraries, for the special case of the function readlink
AC_GNU_SOURCE

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_SIZE_T

# Sets up gettext.
GETTEXT_PACKAGE="$PACKAGE"
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE", [gettext domain])
AM_GLIB_GNU_GETTEXT

# Sets up a few prefix-relative directories
if test "x${prefix}" = "xNONE"; then
  PREFIX="${ac_default_prefix}"
else
  PREFIX="${prefix}"
fi
AC_DEFINE_UNQUOTED(LOCALE_DIR, "${PREFIX}/${DATADIRNAME}/locale",[Locale directory])
AC_DEFINE_UNQUOTED(THEME_DIR, "${PREFIX}/${DATADIRNAME}/icons",[Icon directory])
AC_DEFINE_UNQUOTED(HELP_FILE, "${PREFIX}/${DATADIRNAME}/xpad/help/xpad-user-help.txt",[Help file])
AC_DEFINE_UNQUOTED(BASE_DIR, "${PREFIX}",[Base directory])

# Determines if we're debugging.
AC_ARG_ENABLE(debug,
  AC_HELP_STRING([--enable-debug=[no/yes]],[turn on debugging [default=no]]),
  enable_debug="$enableval",
  enable_debug=no)
if test "$enable_debug" = "yes"; then
  DEBUG_CFLAGS="-ansi -Wall -Werror -g -O0 -DG_DISABLE_DEPRECATED -DGDK_DISABLE_DEPRECATED -DGDK_PIXBUF_DISABLE_DEPRECATED -DGTK_DISABLE_DEPRECATED -DGTK_MULTIHEAD_SAFE -DGTK_DISABLE_SINGLE_INCLUDES -DGSEAL_ENABLE"
else
  if test "$enable_debug" = "most"; then
    DEBUG_CFLAGS="-ansi -Wall -Werror -g -O0 -DG_DISABLE_DEPRECATED -DGDK_DISABLE_DEPRECATED -DGDK_PIXBUF_DISABLE_DEPRECATED -DGTK_MULTIHEAD_SAFE -DGTK_DISABLE_SINGLE_INCLUDES -DGSEAL_ENABLE -Wno-deprecated-declarations"
  else
    DEBUG_CFLAGS="-Wno-deprecated-declarations"
  fi
fi
AC_SUBST(DEBUG_CFLAGS)

# Sets up output files.
AC_CONFIG_FILES([
Makefile 
autopackage/Makefile 
autopackage/default.apspec
data/Makefile
doc/Makefile 
images/Makefile 
po/Makefile.in 
src/Makefile 
])

AC_OUTPUT
