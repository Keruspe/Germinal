# This file is part of Germinal.
#
# Copyright 2011-2015 Marc-Antoine Perennou <Marc-Antoine@Perennou.com>
#
# Germinal is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# Germinal is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Germinal.  If not, see <http://www.gnu.org/licenses/>.

AC_PREREQ([2.69])
AC_INIT([Germinal],[12],[Marc-Antoine@Perennou.com])

INTLTOOL_REQUIRED="0.50.0"
PKGCONFIG_REQUIRED="0.22"

GLIB_MIN_MAJOR="2"
GLIB_MIN_MINOR="44"
GLIB_API_REQUIRED="G_ENCODE_VERSION($GLIB_MIN_MAJOR, $GLIB_MIN_MINOR)"

GTK_MIN_MAJOR="3"
GTK_MIN_MINOR="0"
GDK_API_REQUIRED="G_ENCODE_VERSION($GTK_MIN_MAJOR, $GTK_MIN_MINOR)"

GLIB_REQUIRED="$GLIB_MIN_MAJOR.$GLIB_MIN_MINOR.0"
GTK_REQUIRED="$GTK_MIN_MAJOR.$GTK_MIN_MINOR.0"
VTE_REQUIRED="0.40.0"

AC_SUBST([PACKAGE_NAME], ["$PACKAGE_NAME"])
AC_SUBST([PACKAGE_VERSION], ["$PACKAGE_VERSION"])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_SRCDIR([src/])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([build-aux])
AC_PREFIX_DEFAULT([/usr])

AM_INIT_AUTOMAKE([1.11 subdir-objects check-news foreign no-dist-gzip dist-xz tar-ustar -Wall])
AM_SILENT_RULES([yes])

AC_PROG_AWK
AC_PROG_SED
AC_PROG_MKDIR_P
AC_PROG_LN_S
AC_PROG_INSTALL

AC_PROG_MAKE_SET

AC_PROG_CPP
AC_PROG_CC
AC_PROG_CC_C99
AM_PROG_CC_C_O

AC_C_INLINE

AC_DEFINE_UNQUOTED([GLIB_VERSION_MIN_REQUIRED], [$GLIB_API_REQUIRED], [The lower GLib API version supported])
AC_DEFINE_UNQUOTED([GLIB_VERSION_MAX_ALLOWED], [$GLIB_API_REQUIRED], [The higher GLib API version supported])
AC_DEFINE_UNQUOTED([GDK_VERSION_MIN_REQUIRED], [$GDK_API_REQUIRED], [The lower Gdk API version supported])
AC_DEFINE_UNQUOTED([GDK_VERSION_MAX_ALLOWED], [$GDK_API_REQUIRED], [The higher Gdk API version supported])

GETTEXT_PACKAGE=$PACKAGE_NAME
AC_DEFINE_UNQUOTED([GETTEXT_PACKAGE], ["$GETTEXT_PACKAGE"], [Gettext Package])
AC_SUBST(GETTEXT_PACKAGE)
IT_PROG_INTLTOOL([$INTLTOOL_REQUIRED])

GLIB_GSETTINGS

PKG_PROG_PKG_CONFIG([$PKGCONFIG_REQUIRED])

PKG_CHECK_MODULES(GLIB,  [glib-2.0 >= $GLIB_REQUIRED gio-2.0 >= $GLIB_REQUIRED])
PKG_CHECK_MODULES(GTK,   [gtk+-3.0 >= $GTK_REQUIRED  gdk-3.0 >= $GTK_REQUIRED])
PKG_CHECK_MODULES(VTE,   [vte-2.91 >= $VTE_REQUIRED])
PKG_CHECK_MODULES(PANGO, [pango])

AC_CONFIG_FILES([
  Makefile
  po/Makefile.in
  data/org.gnome.Germinal.gschema.xml.in
])
AC_OUTPUT
AC_MSG_RESULT([
    $PACKAGE_NAME $VERSION
    ===========

    prefix:                 ${prefix}
    exec_prefix:            ${exec_prefix}
    sysconfdir:             ${sysconfdir}
    libdir:                 ${libdir}
    includedir:             ${includedir}

    compiler:               ${CC}
    cflags:                 ${CFLAGS}
    ldflags:                ${LDFLAGS}
])
