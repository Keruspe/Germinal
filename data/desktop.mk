# This file is part of Germinal.
#
# Copyright 2015 Marc-Antoine Perennou <Marc-Antoine@Perennou.com>
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

applicationsdir = $(datadir)/applications

germinal_desktop_file = %D%/desktop/org.gnome.Germinal.desktop

nodist_applications_DATA =       \
	$(germinal_desktop_file) \
	$(NULL)

@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST +=                                            \
	$(germinal_desktop_file:.desktop=.desktop.in.in) \
	$(NULL)

CLEANFILES +=                                         \
	$(germinal_desktop_file)                      \
	$(germinal_desktop_file:.desktop=.desktop.in) \
	$(NULL)

SUFFIXES += .desktop.in.in .desktop.in
.desktop.in.in.desktop.in:
	@ $(MKDIR_P) %D%/desktop
	$(AM_V_GEN) $(SED)                  \
	    -e 's,[@]bindir[@],$(bindir),g' \
	    <$^ >$@

