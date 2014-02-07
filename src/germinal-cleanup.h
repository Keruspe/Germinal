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

#include "germinal-util.h"

#include <vte/vte.h>

#define GERMINAL_ERROR_CLEANUP    GERMINAL_CLEANUP (cleanup_error)
#define GERMINAL_FONT_CLEANUP     GERMINAL_CLEANUP (cleanup_font)
#define GERMINAL_PTY_CLEANUP      GERMINAL_CLEANUP (cleanup_pty)
#define GERMINAL_SETTINGS_CLEANUP GERMINAL_CLEANUP (cleanup_settings)
#define GERMINAL_STR_CLEANUP      GERMINAL_CLEANUP (cleanup_str)
#define GERMINAL_STRV_CLEANUP     GERMINAL_CLEANUP (cleanup_strv)

static void
cleanup_error (GError **error)
{
    if (*error)
        g_error_free (*error);
}

static void
cleanup_font (PangoFontDescription **font)
{
    pango_font_description_free (*font);
}

static void
cleanup_pty (VtePty **pty)
{
    g_object_unref (*pty);
}

static void
cleanup_settings (GSettings **settings)
{
    g_object_unref (*settings);
}

static void
cleanup_str (gchar **str)
{
    g_free (*str);
}

static void
cleanup_strv (gchar ***strv)
{
    g_strfreev (*strv);
}
