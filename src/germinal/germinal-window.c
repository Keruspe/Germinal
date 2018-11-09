/*
 * This file is part of Germinal.
 *
 * Copyright 2018 Marc-Antoine Perennou <Marc-Antoine@Perennou.com>
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

#include "germinal-window.h"

struct _GerminalWindow
{
    GtkApplicationWindow parent_instance;
};

typedef struct
{
    gboolean placeholder;
} GerminalWindowPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (GerminalWindow, germinal_window, GTK_TYPE_APPLICATION_WINDOW)

static void
germinal_window_finalize (GObject *object)
{
    GerminalWindowPrivate *priv = germinal_window_get_instance_private (GERMINAL_WINDOW (object));

    priv->placeholder = FALSE;

    G_OBJECT_CLASS (germinal_window_parent_class)->finalize (object);
}

static void
germinal_window_init (GerminalWindow *self)
{
    GerminalWindowPrivate *priv = germinal_window_get_instance_private (self);

    priv->placeholder = TRUE;
}

static void
germinal_window_class_init (GerminalWindowClass *klass)
{
    G_OBJECT_CLASS (klass)->finalize = germinal_window_finalize;
}

GtkWidget *
germinal_window_new (GtkApplication *application)
{
    return g_object_new (GERMINAL_TYPE_WINDOW,
                         "application", application,
                         "type",        GTK_WINDOW_TOPLEVEL,
                         NULL);
}
