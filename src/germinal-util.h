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

#include <gtk/gtk.h>

#define GERMINAL_CLEANUP(x) __attribute__((cleanup(x)))

#define GERMINAL_REGEX_CLEANUP    GERMINAL_CLEANUP (cleanup_regex)

#define CHARACTER          "[a-zA-Z]"
#define STRAIGHT_TEXT_ONLY "[^ \t\n\r()\\[\\]\"<>]*[^,' \t\n\r()\\[\\]\"<>]+"
#define QUOTED_TEXT        "\"[^\"\n\r]+\""
#define PAREN_TEXT         "\\([^()\n\r]+\\)"
#define SQUARE_BRACED_TEXT "\\[[^\n\r\\[\\]]+\\]"
#define DUMB_USERS_TEXT    "<[^\n\r<>]+>"
#define URL_REGEXP         CHARACTER "+://(" QUOTED_TEXT "|" PAREN_TEXT "|" SQUARE_BRACED_TEXT "|" DUMB_USERS_TEXT "|" STRAIGHT_TEXT_ONLY ")+"

#define BACKCOLOR_KEY            "backcolor"
#define FONT_KEY                 "font"
#define FORECOLOR_KEY            "forecolor"
#define PALETTE_KEY              "palette"
#define SCROLLBACK_KEY           "scrollback-lines"
#define STARTUP_COMMAND_KEY      "startup-command"
#define TERM_KEY                 "term"
#define WORD_CHAR_EXCEPTIONS_KEY "word-char-exceptions"

#define SETTING_SIGNAL(key, fn)                 \
    g_signal_connect (G_OBJECT (settings),      \
                      "changed::" key##_KEY,    \
                      G_CALLBACK (update_##fn), \
                      terminal)
#define SETTING(key, fn)                         \
    update_##fn (settings, key##_KEY, terminal); \
    SETTING_SIGNAL (key, fn)

#define MENU_ACTION(name, label)                                        \
    GtkWidget *name##_menu_item = gtk_menu_item_new_with_label (label); \
    g_signal_connect (G_OBJECT (name##_menu_item),                      \
                      "activate",                                       \
                      G_CALLBACK (do_##name),                           \
                      terminal);                                        \
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), name##_menu_item)

#define MENU_SEPARATOR \
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), gtk_separator_menu_item_new ())

#define CONNECT_SIGNAL(obj, signal, fn, data) \
    g_signal_connect (G_OBJECT (obj),         \
                      signal,                 \
                      G_CALLBACK (fn),        \
                      data)

static void
cleanup_regex (GRegex **regex)
{
    g_regex_unref (*regex);
}
