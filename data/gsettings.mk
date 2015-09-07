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

germinal_gschema_file = %reldir%/gsettings/org.gnome.Germinal.gschema.xml
gschemas_compiled = %reldir%/gsettings/gschemas.compiled

gsettings_SCHEMAS =              \
	$(germinal_gschema_file) \
	$(NULL)

@GSETTINGS_RULES@

$(germinal_gschema_file:.xml=.valid): $(germinal_gschema_file)
	@ $(MKDIR_P) %reldir%/gsettings

$(gschemas_compiled): $(gsettings_SCHEMAS:.xml=.valid)
	$(AM_V_GEN) $(GLIB_COMPILE_SCHEMAS) --targetdir=$(srcdir) .

EXTRA_DIST +=                    \
	$(germinal_gschema_file) \
	$(NULL)

CLEANFILES +=                \
	$(gschemas_compiled) \
	$(NULL)
