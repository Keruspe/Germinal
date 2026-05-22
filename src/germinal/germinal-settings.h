// SPDX-FileCopyrightText: 2021-2026 Marc-Antoine Perennou <Marc-Antoine@Perennou.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

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
