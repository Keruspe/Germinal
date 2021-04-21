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

#include "germinal-settings.h"
#include "germinal-window.h"

struct _GerminalWindow
{
    GtkApplicationWindow parent_instance;
};

enum
{
    C_DECORATED,

    C_LAST_SIGNAL,
};

typedef struct
{
    GSettings *settings;

    guint64    c_signals[C_LAST_SIGNAL];
} GerminalWindowPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (GerminalWindow, germinal_window, GTK_TYPE_APPLICATION_WINDOW)

static void
update_decorated (GSettings   *settings,
                  const gchar *key,
                  gpointer     user_data)
{
    gboolean decorated = g_settings_get_boolean (settings, key);
    GtkWindow *win = GTK_WINDOW (user_data);

    gtk_window_set_decorated (win, decorated);
    gtk_window_set_hide_titlebar_when_maximized (win, !decorated);
}

static void
germinal_window_dispose (GObject *object)
{
    GerminalWindowPrivate *priv = germinal_window_get_instance_private (GERMINAL_WINDOW (object));

    if (priv->settings)
    {
        g_signal_handler_disconnect (priv->settings, priv->c_signals[C_DECORATED]);
        priv->c_signals[C_DECORATED] = -1;
        g_clear_object (&priv->settings);
    }

    G_OBJECT_CLASS (germinal_window_parent_class)->dispose (object);
}

static void
germinal_window_init (GerminalWindow *self)
{
    GerminalWindowPrivate *priv = germinal_window_get_instance_private (self);

    GSettings *settings = priv->settings = germinal_settings_new ();

    priv->c_signals[C_DECORATED] = g_signal_connect (G_OBJECT (settings),
                                                     "changed::" DECORATED_KEY,
                                                     G_CALLBACK (update_decorated),
                                                     self);

    update_decorated (settings, DECORATED_KEY, self);
}

static void
germinal_window_class_init (GerminalWindowClass *klass)
{
    G_OBJECT_CLASS (klass)->dispose = germinal_window_dispose;
}

GtkWidget *
germinal_window_new (GtkApplication *application)
{
    return g_object_new (GERMINAL_TYPE_WINDOW,
                         "application", application,
                         "type",        GTK_WINDOW_TOPLEVEL,
                         NULL);
}
