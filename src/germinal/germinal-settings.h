/*
 * This file is part of Germinal.
 *
 * Copyright 2021 Marc-Antoine Perennou <Marc-Antoine@Perennou.com>
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

#ifndef __GERMINAL_SETTINGS_H__
#define __GERMINAL_SETTINGS_H__

#include <gdk/gdk.h>
#include <gio/gio.h>

G_BEGIN_DECLS

/* Settings keys */
#define AUDIBLE_BELL_KEY         "audible-bell"
#define BACKCOLOR_KEY            "backcolor"
#define DECORATED_KEY            "decorated"
#define FONT_KEY                 "font"
#define FORECOLOR_KEY            "forecolor"
#define PALETTE_KEY              "palette"
#define SCROLLBACK_KEY           "scrollback-lines"
#define STARTUP_COMMAND_KEY      "startup-command"
#define TERM_KEY                 "term"
#define WORD_CHAR_EXCEPTIONS_KEY "word-char-exceptions"

GSettings *germinal_settings_new         (void);
gchar     *germinal_settings_get_string  (GSettings *settings, const gchar *key);
GdkRGBA   *germinal_settings_get_palette (GSettings *settings, gsize *palette_size);

G_END_DECLS

#endif /* __GERMINAL_SETTINGS_H__ */
