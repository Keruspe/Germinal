# This file is part of Germinal.
#
# Copyright 2011-2019 Marc-Antoine Perennou <Marc-Antoine@Perennou.com>
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

m4_define([germinal_version],    [24])

# Build system requirements
m4_define([autoconf_required],   [2.69])
m4_define([automake_required],   [1.15])
m4_define([pkgconfig_required],  [0.29])

# Glib and Gtk macros and dependencies
m4_define([glib_major_required], [2])
m4_define([glib_minor_required], [58])
m4_define([glib_api_required],   [G_ENCODE_VERSION(glib_major_required, glib_minor_required)])
m4_define([glib_required],       [glib_major_required.glib_minor_required.0])
m4_define([gtk_major_required],  [3])
m4_define([gtk_minor_required],  [24])
m4_define([gtk_api_required],    [G_ENCODE_VERSION(gtk_major_required, gtk_minor_required)])
m4_define([gtk_required],        [gtk_major_required.gtk_minor_required.0])

# Other dependencies
m4_define([vte_required],        [0.54.2])
m4_define([gds_required],        [3.28.1])

AC_PREREQ(autoconf_required)
AC_INIT([Germinal], [germinal_version], [Marc-Antoine@Perennou.com], [germinal])

AC_SUBST([PACKAGE_NAME])
AC_SUBST([PACKAGE_VERSION])

AC_DEFINE_UNQUOTED([GLIB_VERSION_MIN_REQUIRED], [glib_api_required], [The lower GLib API version supported])
AC_DEFINE_UNQUOTED([GLIB_VERSION_MAX_ALLOWED],  [glib_api_required], [The higher GLib API version supported])
AC_DEFINE_UNQUOTED([GDK_VERSION_MIN_REQUIRED],  [gtk_api_required],  [The lower Gdk API version supported])
AC_DEFINE_UNQUOTED([GDK_VERSION_MAX_ALLOWED],   [gtk_api_required],  [The higher Gdk API version supported])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_SRCDIR([src/])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([build-aux])
AC_PREFIX_DEFAULT([/usr])

AC_USE_SYSTEM_EXTENSIONS

AM_INIT_AUTOMAKE(automake_required [subdir-objects check-news foreign no-dist-gzip dist-xz tar-ustar -Wall])
AM_MAINTAINER_MODE([enable])
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

GETTEXT_PACKAGE=${PACKAGE_NAME}
AC_DEFINE_UNQUOTED([GETTEXT_PACKAGE], ["${GETTEXT_PACKAGE}"], [Gettext Package])
AC_SUBST(GETTEXT_PACKAGE)
AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION
AM_GNU_GETTEXT_REQUIRE_VERSION([0.19.7])

PKG_PREREQ(pkgconfig_required)
PKG_PROG_PKG_CONFIG([pkgconfig_required])

PKG_CHECK_MODULES(GLIB,      [glib-2.0 >= glib_required gio-2.0 >= glib_required])
PKG_CHECK_MODULES(GTK,       [gtk+-3.0 >= gtk_required  gdk-3.0 >= gtk_required])
PKG_CHECK_MODULES(VTE,       [vte-2.91 >= vte_required])
PKG_CHECK_MODULES(SCHEMAS,   [gsettings-desktop-schemas >= gds_required])
PKG_CHECK_MODULES(PCRE2,     [libpcre2-8])
PKG_CHECK_MODULES(PANGO,     [pango])
PKG_CHECK_MODULES(APPSTREAM, [appstream-glib])

GLIB_GSETTINGS

APPSTREAM_XML

PKG_CHECK_MODULES(DBUS,      [dbus-1])
AC_ARG_WITH([dbusservicesdir],
            AS_HELP_STRING([--with-dbusservicesdir=DIR], [D-Bus system service directory]),
            [],
            [with_dbusservicesdir=`${PKG_CONFIG} --variable=session_bus_services_dir dbus-1`])
AC_SUBST([dbusservicesdir], [$with_dbusservicesdir])

AC_ARG_WITH([systemduserunitdir],
            AS_HELP_STRING([--with-systemduserunitdir=DIR], [systemd user units directory]),
            [],
            [with_systemduserunitdir=`${PKG_CONFIG} --variable systemduserunitdir systemd`])
AC_SUBST([systemduserunitdir], [$with_systemduserunitdir])

AC_CONFIG_FILES([
  Makefile
  po/Makefile.in
])
AC_OUTPUT
AC_MSG_RESULT([
    ${PACKAGE_NAME} ${VERSION}
    ===========

    prefix:                 ${prefix}
    exec_prefix:            ${exec_prefix}
    sysconfdir:             ${sysconfdir}
    libdir:                 ${libdir}
    includedir:             ${includedir}

    compiler:               ${CC}
    cflags:                 ${CFLAGS}
    ldflags:                ${LDFLAGS}

    DBus user services dir: ${dbusservicesdir}
    systemd user units dir: ${systemduserunitdir}
])
