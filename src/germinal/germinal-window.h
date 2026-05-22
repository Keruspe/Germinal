// SPDX-FileCopyrightText: 2018-2026 Marc-Antoine Perennou <Marc-Antoine@Perennou.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "germinal-terminal.h"

#include <adwaita.h>

G_BEGIN_DECLS

#define GERMINAL_TYPE_WINDOW germinal_window_get_type ()
G_DECLARE_FINAL_TYPE (GerminalWindow, germinal_window, GERMINAL, WINDOW, AdwApplicationWindow)

GtkWidget *germinal_window_new           (GtkApplication *application, GerminalTerminal *terminal);
void       germinal_window_present       (GerminalWindow *self);
void       germinal_window_spawn_command (GerminalWindow *self, GStrv command);

G_END_DECLS
