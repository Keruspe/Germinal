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

#include "germinal-settings.h"
#include "germinal-window.h"

struct _GerminalWindow
{
    GtkApplicationWindow parent_instance;
};

enum
{
    PROP_TERMINAL = 1,

    N_PROPS
};

enum
{
    C_DECORATED,

    C_LAST_SIGNAL,
};

typedef struct
{
    GSettings        *settings;
    GerminalTerminal *terminal;

    guint64           c_signals[C_LAST_SIGNAL];
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

static gboolean
on_button_press (GtkWidget      *widget,
                 GdkEventButton *button_event,
                 gpointer        user_data)
{
    if (button_event->type != GDK_BUTTON_PRESS)
        return FALSE;

    GerminalTerminal *self = GERMINAL_TERMINAL (widget);

    /* Shift + Left clic */
    if ((button_event->button == GDK_BUTTON_PRIMARY) &&
        (button_event->state & GDK_SHIFT_MASK))
            return germinal_terminal_open_url (self, button_event);
    else if (button_event->button == GDK_BUTTON_SECONDARY)
    {
        gchar *url = germinal_terminal_get_url (self, button_event);

        if (url) /* show url stuff */
            gtk_widget_show_all (GTK_WIDGET (user_data));
        else
        {
            /* hide url stuff */
            g_autoptr (GList) children = gtk_container_get_children (GTK_CONTAINER (user_data));
            gtk_widget_hide (GTK_WIDGET (children->data));
            gtk_widget_hide (GTK_WIDGET (children->next->data));
            gtk_widget_hide (GTK_WIDGET (children->next->next->data));
        }
        gtk_menu_popup_at_pointer (GTK_MENU (user_data), (GdkEvent *) button_event);
        return TRUE;
    }

    return FALSE;
}

static void
on_child_exited (VteTerminal *vteterminal G_GNUC_UNUSED,
                 gint         status,
                 gpointer     user_data)
{
    if (status)
        g_critical ("child exited with code %d", status);
    gtk_window_close (GTK_WINDOW (user_data));
}

static void
on_window_title_changed (VteTerminal *vteterminal,
                         gpointer     user_data)
{
    gtk_window_set_title (GTK_WINDOW (user_data), vte_terminal_get_window_title (vteterminal));
}

static gboolean
do_copy (GtkWidget *widget G_GNUC_UNUSED,
         gpointer   user_data)
{
    germinal_terminal_copy (GERMINAL_TERMINAL (user_data));
    return TRUE;
}

static gboolean
do_copy_html (GtkWidget *widget G_GNUC_UNUSED,
              gpointer   user_data)
{
    germinal_terminal_copy_html (GERMINAL_TERMINAL (user_data));
    return TRUE;
}

static gboolean
do_paste (GtkWidget *widget G_GNUC_UNUSED,
          gpointer   user_data)
{
    germinal_terminal_paste (GERMINAL_TERMINAL (user_data));
    return TRUE;
}

static gboolean
do_copy_url (GtkWidget *widget G_GNUC_UNUSED,
             gpointer   user_data)
{
    return germinal_terminal_copy_url (GERMINAL_TERMINAL (user_data));
}

static gboolean
do_open_url (GtkWidget *widget G_GNUC_UNUSED,
             gpointer   user_data)
{
    return germinal_terminal_open_url (GERMINAL_TERMINAL (user_data), NULL);
}

static gboolean
do_zoom (GtkWidget *widget G_GNUC_UNUSED,
         gpointer   user_data)
{
    germinal_terminal_zoom (GERMINAL_TERMINAL (user_data));
    return TRUE;
}

static gboolean
do_dezoom (GtkWidget *widget G_GNUC_UNUSED,
           gpointer   user_data)
{
    germinal_terminal_dezoom (GERMINAL_TERMINAL (user_data));
    return TRUE;
}

static gboolean
do_reset_zoom (GtkWidget *widget G_GNUC_UNUSED,
               gpointer   user_data)
{
    germinal_terminal_reset_zoom (GERMINAL_TERMINAL (user_data));
    return TRUE;
}

static gboolean
do_quit (GtkWidget *widget G_GNUC_UNUSED,
         gpointer   user_data)
{
    gtk_window_close (GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (user_data))));
    return TRUE;
}

static void
germinal_window_set_property (GObject      *object,
                              guint         prop_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
    GerminalWindowPrivate *priv = germinal_window_get_instance_private (GERMINAL_WINDOW (object));

    switch (prop_id)
    {
    case PROP_TERMINAL:
        priv->terminal = g_value_dup_object (value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
germinal_window_constructed (GObject *object)
{
    GerminalWindow *self = GERMINAL_WINDOW (object);
    GerminalWindowPrivate *priv = germinal_window_get_instance_private (self);
    GtkWidget *terminal = GTK_WIDGET (priv->terminal);

    G_OBJECT_CLASS (germinal_window_parent_class)->constructed (object);

    /* Fill window */
    gtk_container_add (GTK_CONTAINER (self), terminal);
    gtk_widget_grab_focus (terminal);

    /* Populate right click menu */
    GtkWidget *menu = gtk_menu_new ();

    MENU_ACTION (copy_url,   _("Copy url"));
    MENU_ACTION (open_url,   _("Open url"));

    MENU_SEPARATOR;

    MENU_ACTION (copy,       _("Copy"));
    MENU_ACTION (copy_html,  _("Copy as HTML"));
    MENU_ACTION (paste,      _("Paste"));

    MENU_SEPARATOR;

    MENU_ACTION (zoom,       _("Zoom"));
    MENU_ACTION (dezoom,     _("Dezoom"));
    MENU_ACTION (reset_zoom, _("Reset zoom"));

    MENU_SEPARATOR;

    MENU_ACTION (quit,       _("Quit"));

    /* Bind signals */
    CONNECT_SIGNAL (terminal, "button-press-event",   on_button_press,         menu);
    CONNECT_SIGNAL (terminal, "child-exited",         on_child_exited,         self);
    CONNECT_SIGNAL (terminal, "window-title-changed", on_window_title_changed, self);

    /* Initialize title */
    on_window_title_changed (VTE_TERMINAL (terminal), self);

    gtk_widget_show_all (menu);
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

    g_clear_object (&priv->terminal);

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
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->constructed  = germinal_window_constructed;
    object_class->dispose      = germinal_window_dispose;
    object_class->set_property = germinal_window_set_property;

    g_object_class_install_property (object_class, PROP_TERMINAL,
        g_param_spec_object ("terminal", NULL, NULL,
                             GERMINAL_TYPE_TERMINAL,
                             G_PARAM_CONSTRUCT_ONLY | G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS));
}

void
germinal_window_present (GerminalWindow *self)
{
    GtkWidget *window = GTK_WIDGET (self);

    gtk_widget_show_all (window);
    gtk_window_maximize (GTK_WINDOW (window));
}

GtkWidget *
germinal_window_new (GtkApplication   *application,
                     GerminalTerminal *terminal)
{
    return g_object_new (GERMINAL_TYPE_WINDOW,
                         "application", application,
                         "terminal",    terminal,
                         "type",        GTK_WINDOW_TOPLEVEL,
                         NULL);
}
