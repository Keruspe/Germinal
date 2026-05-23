// SPDX-FileCopyrightText: 2026 Marc-Antoine Perennou <Marc-Antoine@Perennou.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "germinal-palette-editor.h"
#include "germinal-settings.h"

#include <glib/gi18n-lib.h>

#define N_PALETTE_COLORS 16
#define N_COLS            8

struct _GerminalPaletteEditor
{
    GtkWidget parent_instance;
};

typedef struct
{
    GSettings            *settings;
    GtkColorDialogButton *buttons[N_PALETTE_COLORS];
    gboolean              updating;
} GerminalPaletteEditorPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (GerminalPaletteEditor, germinal_palette_editor, GTK_TYPE_WIDGET)

static void
load_palette (GerminalPaletteEditor *self)
{
    GerminalPaletteEditorPrivate *priv = germinal_palette_editor_get_instance_private (self);

    gsize size = 0;
    g_autofree GdkRGBA *palette = germinal_settings_get_palette (priv->settings, &size);

    priv->updating = TRUE;
    for (guint i = 0; i < N_PALETTE_COLORS; ++i)
    {
        GdkRGBA color = { 0 };
        if (i < size)
            color = palette[i];
        gtk_color_dialog_button_set_rgba (priv->buttons[i], &color);
    }
    priv->updating = FALSE;
}

static void
save_palette (GerminalPaletteEditor *self)
{
    GerminalPaletteEditorPrivate *priv = germinal_palette_editor_get_instance_private (self);

    priv->updating = TRUE;
    g_auto (GStrv) strv = g_new0 (gchar *, N_PALETTE_COLORS + 1);
    for (guint i = 0; i < N_PALETTE_COLORS; ++i)
    {
        const GdkRGBA *rgba = gtk_color_dialog_button_get_rgba (priv->buttons[i]);
        strv[i] = rgba ? gdk_rgba_to_string (rgba) : g_strdup ("#000000");
    }
    g_settings_set_strv (priv->settings, PALETTE_KEY, (const gchar * const *) strv);
    priv->updating = FALSE;
}

static void
on_palette_changed (GSettings   *settings G_GNUC_UNUSED,
                    const gchar *key      G_GNUC_UNUSED,
                    gpointer     user_data)
{
    GerminalPaletteEditor *self = GERMINAL_PALETTE_EDITOR (user_data);
    GerminalPaletteEditorPrivate *priv = germinal_palette_editor_get_instance_private (self);

    if (!priv->updating)
        load_palette (self);
}

static void
on_button_rgba_changed (GtkColorDialogButton *button G_GNUC_UNUSED,
                        GParamSpec           *pspec  G_GNUC_UNUSED,
                        gpointer              user_data)
{
    GerminalPaletteEditor *self = GERMINAL_PALETTE_EDITOR (user_data);
    GerminalPaletteEditorPrivate *priv = germinal_palette_editor_get_instance_private (self);

    if (!priv->updating)
        save_palette (self);
}

static void
germinal_palette_editor_dispose (GObject *object)
{
    GerminalPaletteEditor *self = GERMINAL_PALETTE_EDITOR (object);
    GerminalPaletteEditorPrivate *priv = germinal_palette_editor_get_instance_private (self);

    g_clear_object (&priv->settings);

    for (GtkWidget *child = gtk_widget_get_first_child (GTK_WIDGET (self)); child != NULL; )
    {
        GtkWidget *next = gtk_widget_get_next_sibling (child);
        gtk_widget_unparent (child);
        child = next;
    }

    G_OBJECT_CLASS (germinal_palette_editor_parent_class)->dispose (object);
}

static void
germinal_palette_editor_init (GerminalPaletteEditor *self G_GNUC_UNUSED)
{
}

static void
germinal_palette_editor_class_init (GerminalPaletteEditorClass *klass)
{
    G_OBJECT_CLASS (klass)->dispose = germinal_palette_editor_dispose;

    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
    gtk_widget_class_set_layout_manager_type (widget_class, GTK_TYPE_BOX_LAYOUT);
    gtk_widget_class_set_css_name (widget_class, "germinal-palette-editor");
}

GtkWidget *
germinal_palette_editor_new (GSettings *settings)
{
    GerminalPaletteEditor *self = g_object_new (GERMINAL_TYPE_PALETTE_EDITOR, NULL);
    GerminalPaletteEditorPrivate *priv = germinal_palette_editor_get_instance_private (self);

    priv->settings = g_object_ref (settings);

    gtk_orientable_set_orientation (GTK_ORIENTABLE (gtk_widget_get_layout_manager (GTK_WIDGET (self))),
                                    GTK_ORIENTATION_VERTICAL);

    static const gchar * const row_labels[] = { N_("Normal"), N_("Bright") };

    for (guint row = 0; row < G_N_ELEMENTS (row_labels); ++row)
    {
        GtkWidget *box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
        gtk_widget_set_margin_start  (box, 12);
        gtk_widget_set_margin_end    (box, 12);
        gtk_widget_set_margin_top    (box, (row == 0) ? 6 : 0);
        gtk_widget_set_margin_bottom (box, 6);

        GtkWidget *label = gtk_label_new (_(row_labels[row]));
        gtk_label_set_xalign (GTK_LABEL (label), 0.0);
        gtk_widget_set_size_request (label, 48, -1);
        gtk_box_append (GTK_BOX (box), label);

        for (guint col = 0; col < N_COLS; ++col)
        {
            guint idx = row * N_COLS + col;
            GtkWidget *button = gtk_color_dialog_button_new (gtk_color_dialog_new ());
            priv->buttons[idx] = GTK_COLOR_DIALOG_BUTTON (button);
            g_signal_connect_object (button, "notify::rgba",
                                     G_CALLBACK (on_button_rgba_changed), self, 0);
            gtk_box_append (GTK_BOX (box), button);
        }

        gtk_widget_set_parent (box, GTK_WIDGET (self));
    }

    g_signal_connect_object (settings, "changed::" PALETTE_KEY,
                             G_CALLBACK (on_palette_changed), self, 0);

    load_palette (self);

    return GTK_WIDGET (self);
}

const GdkRGBA *
germinal_palette_editor_get_rgba (GerminalPaletteEditor *self,
                                  guint                  index)
{
    g_return_val_if_fail (GERMINAL_IS_PALETTE_EDITOR (self), NULL);
    g_return_val_if_fail (index < N_PALETTE_COLORS, NULL);

    GerminalPaletteEditorPrivate *priv = germinal_palette_editor_get_instance_private (self);
    return gtk_color_dialog_button_get_rgba (priv->buttons[index]);
}

void
germinal_palette_editor_set_rgba (GerminalPaletteEditor *self,
                                  guint                  index,
                                  const GdkRGBA         *rgba)
{
    g_return_if_fail (GERMINAL_IS_PALETTE_EDITOR (self));
    g_return_if_fail (index < N_PALETTE_COLORS);

    GerminalPaletteEditorPrivate *priv = germinal_palette_editor_get_instance_private (self);
    gtk_color_dialog_button_set_rgba (priv->buttons[index], rgba);
}
