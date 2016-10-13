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

germinal_gschema_file = %D%/gsettings/org.gnome.Germinal.gschema.xml
gschemas_compiled = %D%/gsettings/gschemas.compiled

gsettings_SCHEMAS =              \
	$(germinal_gschema_file) \
	$(NULL)

@GSETTINGS_RULES@

$(germinal_gschema_file:.xml=.valid): $(germinal_gschema_file)
	@ $(MKDIR_P) $(@D)

$(gschemas_compiled): $(gsettings_SCHEMAS:.xml=.valid)
	$(AM_V_GEN) $(GLIB_COMPILE_SCHEMAS) --targetdir=$(srcdir) .

SUFFIXES += .gschema.xml.in .gschema.xml
.gschema.xml.in.gschema.xml:
	@ $(MKDIR_P) $(@D)
	$(AM_V_GEN) $(SED) -e 's,[@]GETTEXT_PACKAGE[@],$(GETTEXT_PACKAGE),g' < $< > $@

EXTRA_DIST +=                                 \
	$(germinal_gschema_file:.xml=.xml.in) \
	$(NULL)

CLEANFILES +=                    \
	$(germinal_gschema_file) \
	$(gschemas_compiled)     \
	$(NULL)
