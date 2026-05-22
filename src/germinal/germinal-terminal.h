/*
 * This file is part of Germinal.
 *
 * Copyright 2018-2026 Marc-Antoine Perennou <Marc-Antoine@Perennou.com>
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

#pragma once

#include <glib/gi18n-lib.h>

#include <vte/vte.h>

G_BEGIN_DECLS

#define GERMINAL_TYPE_TERMINAL germinal_terminal_get_type ()
G_DECLARE_FINAL_TYPE (GerminalTerminal, germinal_terminal, GERMINAL, TERMINAL, VteTerminal)

gboolean     germinal_terminal_is_zero     (GerminalTerminal *self, guint keycode);
const gchar *germinal_terminal_get_url     (GerminalTerminal *self);
void         germinal_terminal_update_url  (GerminalTerminal *self, gdouble x, gdouble y);
gboolean     germinal_terminal_open_url    (GerminalTerminal *self);
gboolean germinal_terminal_copy_url    (GerminalTerminal *self);
void     germinal_terminal_copy        (GerminalTerminal *self);
void     germinal_terminal_copy_html   (GerminalTerminal *self);
void     germinal_terminal_paste       (GerminalTerminal *self);
gboolean germinal_terminal_spawn       (GerminalTerminal *self, gchar **cmd, GError **error);
void     germinal_terminal_zoom        (GerminalTerminal *self);
void     germinal_terminal_dezoom      (GerminalTerminal *self);
void     germinal_terminal_reset_zoom  (GerminalTerminal *self);
void     germinal_terminal_update_font (GerminalTerminal *self, const gchar *font_str);

void     germinal_terminal_spawn_command (GerminalTerminal *self, GStrv command);

gboolean germinal_terminal_search      (GerminalTerminal *self, const gchar *text);
gboolean germinal_terminal_search_next (GerminalTerminal *self);
gboolean germinal_terminal_search_prev (GerminalTerminal *self);
void     germinal_terminal_search_stop (GerminalTerminal *self);

GtkWidget *germinal_terminal_new ();

G_END_DECLS
