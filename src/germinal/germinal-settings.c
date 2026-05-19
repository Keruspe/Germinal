/*
 * This file is part of Germinal.
 *
 * Copyright 2021-2026 Marc-Antoine Perennou <Marc-Antoine@Perennou.com>
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

#include "germinal-settings.h"

#define G_SETTINGS_ENABLE_BACKEND 1
#include <gio/gsettingsbackend.h>

GSettings *
germinal_settings_new (void)
{
    g_autofree gchar *config_file_path = g_build_filename (g_get_user_config_dir (), "germinal", "settings", NULL);
    g_autoptr (GFile) config_file = g_file_new_for_path (config_file_path);

    if (g_file_query_exists (config_file, NULL /* cancellable */))
    {
        g_autoptr (GSettingsBackend) backend = g_keyfile_settings_backend_new (config_file_path, "/org/gnome/Germinal/", "Germinal");

        return g_settings_new_with_backend ("org.gnome.Germinal", backend);
    }
    else
    {
        return g_settings_new ("org.gnome.Germinal");
    }
}

gchar *
germinal_settings_get_string (GSettings   *settings,
                              const gchar *key)
{
    return (key) ? g_settings_get_string (settings, key) : NULL;
}

GdkRGBA *
germinal_settings_get_palette (GSettings *settings,
                               gsize     *palette_size)
{
    g_auto (GStrv) colors = g_settings_get_strv (settings, PALETTE_KEY);
    guint size = g_strv_length (colors);

    if (!((size == 0) ||
          (size == 8) ||
          (size == 16) ||
          (size == 24) ||
          ((size >= 25) && (size <= 255))))
    {
        g_settings_reset (settings, PALETTE_KEY);
        return germinal_settings_get_palette (settings, palette_size);
    }

    GdkRGBA *palette = g_new (GdkRGBA, size);

    for (guint i = 0; i < size; ++i)
        gdk_rgba_parse (&palette[i], colors[i]);

    *palette_size = size;

    return palette;
}
