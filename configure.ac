# This file is part of Germinal.
#
# Copyright 2011 Marc-Antoine Perennou <Marc-Antoine@Perennou.com>
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

AC_PREREQ([2.68])
AC_INIT([Germinal],[1],[Marc-Antoine@Perennou.com])

AC_SUBST([PACKAGE_NAME], ["$PACKAGE_NAME"])
AC_SUBST([PACKAGE_VERSION], ["$PACKAGE_VERSION"])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_SRCDIR([src/])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([.aux])

AM_INIT_AUTOMAKE([1.11 subdir-objects check-news foreign no-dist-gzip dist-xz tar-ustar -Wall])
AM_SILENT_RULES([yes])

AC_PROG_INSTALL
AC_PROG_CC
AM_PROG_CC_C_O

GETTEXT_PACKAGE=$PACKAGE_NAME
AC_DEFINE_UNQUOTED([GETTEXT_PACKAGE], ["$GETTEXT_PACKAGE"], [Gettext Package])
AC_SUBST(GETTEXT_PACKAGE)
IT_PROG_INTLTOOL([0.40.0])

PKG_CHECK_MODULES(GLIB, [glib-2.0 >= 2.28.0])
PKG_CHECK_MODULES(GIO, [gio-2.0 >= 2.28.0])
PKG_CHECK_MODULES(GTK, [gtk+-3.0 >= 3.0.0])
PKG_CHECK_MODULES(GDK, [gdk-3.0 >= 3.0.0])
PKG_CHECK_MODULES(VTE, [vte-2.90])

AC_CONFIG_FILES([
  Makefile
  po/Makefile.in
])
AC_OUTPUT
AC_MSG_RESULT([
        $PACKAGE_NAME $VERSION
])
