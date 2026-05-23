// SPDX-FileCopyrightText: 2026 Marc-Antoine Perennou <Marc-Antoine@Perennou.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <gtk/gtk.h>
#include <gio/gio.h>

G_BEGIN_DECLS

#define GERMINAL_TYPE_PALETTE_EDITOR germinal_palette_editor_get_type ()
G_DECLARE_FINAL_TYPE (GerminalPaletteEditor, germinal_palette_editor, GERMINAL, PALETTE_EDITOR, GtkWidget)

GtkWidget     *germinal_palette_editor_new      (GSettings *settings);
const GdkRGBA *germinal_palette_editor_get_rgba (GerminalPaletteEditor *self, guint index);
void           germinal_palette_editor_set_rgba (GerminalPaletteEditor *self, guint index, const GdkRGBA *rgba);

G_END_DECLS
