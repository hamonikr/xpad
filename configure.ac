# Process this file with autoconf to produce a configure script.

# Sets up autoconf.
AC_INIT([Xpad],[5.8.8],[xpad-hackers@lists.launchpad.net])
AC_CONFIG_SRCDIR(src/xpad-app.c)
AC_CONFIG_HEADERS([config.h:config.h.in])

# Checks for programs.
AC_USE_SYSTEM_EXTENSIONS
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_MAKE_SET

# Sets up automake.
AM_INIT_AUTOMAKE([1.9 gnu no-dist-gzip dist-bzip2])
AM_BINRELOC

AC_PATH_XTRA

# Checks for GTK+ 3.0
PKG_CHECK_MODULES(GTK, gtk+-3.0 >= 3.22 gio-2.0 >= 2.56)
AM_CONDITIONAL(GTK, test -n "$GTK_LIBS")
AC_SUBST(GTK_LIBS)
AC_SUBST(GTK_CFLAGS)

# GTK source view version 4.x is based on the version available in Debian stable: https://packages.debian.org/stable/libs/
PKG_CHECK_MODULES(GTK_SOURCE_VIEW, gtksourceview-4 >= 4.0.3)
AC_SUBST(GTK_SOURCE_VIEW_LIBS)
AC_SUBST(GTK_SOURCE_VIEW_CFLAGS)

# Glib version is based on the version available in Debian stable: https://packages.debian.org/stable/libs/
PKG_CHECK_MODULES(GLIB, glib-2.0 >= 2.58)
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

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_SIZE_T

# i18n / text translations
# The choice for version of gettext is only based on what was installed on the developers desktop (2021).
# If another version is more convenient, go for it.
AM_GNU_GETTEXT_VERSION([0.21])
AM_GNU_GETTEXT([external])

GETTEXT_PACKAGE="$PACKAGE"
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE", [gettext domain])

# App indicator / systray (replacement of the deprecated GtkStatusIcon)

# Version requirement copied from transmission-gtk
APPINDICATOR_REQUIRED=0.4.90

AC_ARG_ENABLE(appindicator,
                        AS_HELP_STRING([--enable-appindicator[=@<:@no/auto/yes@:>@]],[Build support for application indicators ]),
                        [enable_appindicator=$enableval],
                        [enable_appindicator="auto"])

if test x$enable_appindicator = xauto ; then
        PKG_CHECK_EXISTS([ayatana-appindicator3-0.1 >= $APPINDICATOR_REQUIRED],
                         enable_appindicator="yes",
                         enable_appindicator="no")
fi

if test x$enable_appindicator = xyes ; then
        PKG_CHECK_EXISTS([ayatana-appindicator3-0.1 >= $APPINDICATOR_REQUIRED],,
                         AC_MSG_ERROR([ayatana-appindicator3-0.1 is not installed]))
        PKG_CHECK_MODULES(APP_INDICATOR,
                        ayatana-appindicator3-0.1 >= $APPINDICATOR_REQUIRED)
        AC_SUBST(APP_INDICATOR_CFLAGS)
        AC_SUBST(APP_INDICATOR_LIBS)
        AC_DEFINE(HAVE_APP_INDICATOR, 1, [Have AppIndicator])
fi

AM_CONDITIONAL(HAVE_APP_INDICATOR, test x"$enable_appindicator" = xyes)

# Sets up a few prefix-relative directories
if test "x${prefix}" = "xNONE"; then
  PREFIX="${ac_default_prefix}"
else
  PREFIX="${prefix}"
fi

# Define the help path and filename
HELP_DIR="$PACKAGE/help"
HELP_FILE_NAME="xpad-user-help.txt"
AC_SUBST(HELP_DIR)
AC_SUBST(HELP_FILE_NAME)

#Define the locale path
LOCALE_DIR="locale"
AC_SUBST(LOCALE_DIR)

#Define the icon path
ICON_PATH="icons"
AC_SUBST(THEME_DIR)

# Determines if we're debugging.
AC_ARG_ENABLE(debug,
  AS_HELP_STRING([--enable-debug=[no/yes]],[turn on debugging [default=no]]),
  enable_debug="$enableval",
  enable_debug=no)
if test "$enable_debug" = "yes"; then
  DEBUG_CFLAGS="-std=c99 -Wall -Werror -g -O0 -DG_DISABLE_DEPRECATED -DGDK_DISABLE_DEPRECATED -DGDK_PIXBUF_DISABLE_DEPRECATED -DGTK_DISABLE_DEPRECATED -DGTK_MULTIHEAD_SAFE -DGTK_DISABLE_SINGLE_INCLUDES -DGSEAL_ENABLE"
else
  if test "$enable_debug" = "most"; then
    DEBUG_CFLAGS="-std=c99 -Wall -Werror -g -O0 -DG_DISABLE_DEPRECATED -DGDK_DISABLE_DEPRECATED -DGDK_PIXBUF_DISABLE_DEPRECATED -DGTK_MULTIHEAD_SAFE -DGTK_DISABLE_SINGLE_INCLUDES -DGSEAL_ENABLE -Wno-deprecated-declarations"
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
