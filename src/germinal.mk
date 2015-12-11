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

bin_PROGRAMS +=      \
	bin/germinal \
	$(NULL)

germinal_c_files =                     \
	%D%/germinal/germinal.c        \
	$(NULL)

bin_germinal_SOURCES =                  \
	%D%/germinal/germinal-cleanup.h \
	%D%/germinal/germinal-util.h    \
	$(germinal_c_files)             \
	$(NULL)

bin_germinal_LDADD = \
	$(AM_LIBS)   \
	$(NULL)

CLEANFILES +=                     \
	${germinal_c_files:.c=.o} \
	$(NULL)

