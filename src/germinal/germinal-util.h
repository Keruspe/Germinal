// SPDX-FileCopyrightText: 2013-2026 Marc-Antoine Perennou <Marc-Antoine@Perennou.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

/* Url Regexp */
#define CHARACTER          "[a-zA-Z]"
#define STRAIGHT_TEXT_ONLY "[^ \t\n\r()\\[\\]\"<>]*[^,' \t\n\r()\\[\\]\"<>]+"
#define QUOTED_TEXT        "\"[^\"\n\r]+\""
#define PAREN_TEXT         "\\([^()\n\r]+\\)"
#define SQUARE_BRACED_TEXT "\\[[^\n\r\\[\\]]+\\]"
#define DUMB_USERS_TEXT    "<[^\n\r<>]+>"
#define URL_REGEXP         CHARACTER "+://(" QUOTED_TEXT "|" PAREN_TEXT "|" SQUARE_BRACED_TEXT "|" DUMB_USERS_TEXT "|" STRAIGHT_TEXT_ONLY ")+"

G_END_DECLS
