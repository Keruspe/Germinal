/*
 * This file is part of Germinal.
 *
 * Copyright 2013 Marc-Antoine Perennou <Marc-Antoine@Perennou.com>
 *
 * Germinal is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Germinal is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Germinal.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __GERMINAL_CLEANUP_H__
#define __GERMINAL_CLEANUP_H__

#include "germinal-util.h"

#include <vte/vte.h>

G_BEGIN_DECLS

#define GERMINAL_CLEANUP(x) __attribute__((cleanup(x)))

#define GERMINAL_FONT_CLEANUP GERMINAL_CLEANUP (cleanup_font)

static void
cleanup_font (PangoFontDescription **font)
{
    pango_font_description_free (*font);
}

G_END_DECLS

#endif /* __GERMINAL_CLEANUP_H__ */
