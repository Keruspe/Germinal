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
    AdwApplicationWindow parent_instance;
};

enum
{
    PROP_TERMINAL = 1,

    N_PROPS
};

typedef struct
{
    GSettings        *settings;
    GerminalTerminal *terminal;

    GSignalGroup     *settings_signals;
    GSignalGroup     *terminal_signals;

    GtkWidget        *popover;
    GMenu            *url_section;
} GerminalWindowPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (GerminalWindow, germinal_window, ADW_TYPE_APPLICATION_WINDOW)

static void
update_decorated (GSettings   *settings,
                  const gchar *key,
                  gpointer     user_data)
{
    gtk_window_set_decorated (GTK_WINDOW (user_data), g_settings_get_boolean (settings, key));
}

static void
on_click_pressed (GtkGestureClick *gesture,
                  gint             n_press G_GNUC_UNUSED,
                  gdouble          x,
                  gdouble          y,
                  gpointer         user_data)
{
    GerminalWindow *self = GERMINAL_WINDOW (user_data);
    GerminalWindowPrivate *priv = germinal_window_get_instance_private (self);
    guint button = gtk_gesture_single_get_current_button (GTK_GESTURE_SINGLE (gesture));
    GdkModifierType state = gtk_event_controller_get_current_event_state (GTK_EVENT_CONTROLLER (gesture));

    /* Shift + Left click */
    if (button == GDK_BUTTON_PRIMARY && (state & GDK_SHIFT_MASK))
    {
        germinal_terminal_update_url (priv->terminal, x, y);
        germinal_terminal_open_url (priv->terminal);
    }
    else if (button == GDK_BUTTON_SECONDARY)
    {
        germinal_terminal_update_url (priv->terminal, x, y);
        gboolean has_url = germinal_terminal_get_url (priv->terminal) != NULL;

        g_menu_remove_all (priv->url_section);
        if (has_url)
        {
            g_menu_append (priv->url_section, _("Copy url"), "ctx.copy-url");
            g_menu_append (priv->url_section, _("Open url"), "ctx.open-url");
        }

        GdkRectangle rect = { (gint) x, (gint) y, 1, 1 };
        gtk_popover_set_pointing_to (GTK_POPOVER (priv->popover), &rect);
        gtk_popover_popup (GTK_POPOVER (priv->popover));
    }
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
                         const gchar *prop G_GNUC_UNUSED,
                         gpointer     user_data)
{
    g_autofree gchar *title = vte_terminal_dup_termprop_string_by_id (vteterminal, VTE_PROPERTY_ID_XTERM_TITLE, NULL);
    gtk_window_set_title (GTK_WINDOW (user_data), title);
}

static void
action_copy (GSimpleAction *action G_GNUC_UNUSED,
             GVariant      *param G_GNUC_UNUSED,
             gpointer       user_data)
{
    GerminalWindowPrivate *priv = germinal_window_get_instance_private (GERMINAL_WINDOW (user_data));
    germinal_terminal_copy (priv->terminal);
}

static void
action_copy_html (GSimpleAction *action G_GNUC_UNUSED,
                  GVariant      *param G_GNUC_UNUSED,
                  gpointer       user_data)
{
    GerminalWindowPrivate *priv = germinal_window_get_instance_private (GERMINAL_WINDOW (user_data));
    germinal_terminal_copy_html (priv->terminal);
}

static void
action_paste (GSimpleAction *action G_GNUC_UNUSED,
              GVariant      *param G_GNUC_UNUSED,
              gpointer       user_data)
{
    GerminalWindowPrivate *priv = germinal_window_get_instance_private (GERMINAL_WINDOW (user_data));
    germinal_terminal_paste (priv->terminal);
}

static void
action_copy_url (GSimpleAction *action G_GNUC_UNUSED,
                 GVariant      *param G_GNUC_UNUSED,
                 gpointer       user_data)
{
    GerminalWindowPrivate *priv = germinal_window_get_instance_private (GERMINAL_WINDOW (user_data));
    germinal_terminal_copy_url (priv->terminal);
}

static void
action_open_url (GSimpleAction *action G_GNUC_UNUSED,
                 GVariant      *param G_GNUC_UNUSED,
                 gpointer       user_data)
{
    GerminalWindowPrivate *priv = germinal_window_get_instance_private (GERMINAL_WINDOW (user_data));
    germinal_terminal_open_url (priv->terminal);
}

static void
action_zoom (GSimpleAction *action G_GNUC_UNUSED,
             GVariant      *param G_GNUC_UNUSED,
             gpointer       user_data)
{
    GerminalWindowPrivate *priv = germinal_window_get_instance_private (GERMINAL_WINDOW (user_data));
    germinal_terminal_zoom (priv->terminal);
}

static void
action_dezoom (GSimpleAction *action G_GNUC_UNUSED,
               GVariant      *param G_GNUC_UNUSED,
               gpointer       user_data)
{
    GerminalWindowPrivate *priv = germinal_window_get_instance_private (GERMINAL_WINDOW (user_data));
    germinal_terminal_dezoom (priv->terminal);
}

static void
action_reset_zoom (GSimpleAction *action G_GNUC_UNUSED,
                   GVariant      *param G_GNUC_UNUSED,
                   gpointer       user_data)
{
    GerminalWindowPrivate *priv = germinal_window_get_instance_private (GERMINAL_WINDOW (user_data));
    germinal_terminal_reset_zoom (priv->terminal);
}

static void
action_quit (GSimpleAction *action G_GNUC_UNUSED,
             GVariant      *param G_GNUC_UNUSED,
             gpointer       user_data)
{
    gtk_window_close (GTK_WINDOW (user_data));
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
    adw_application_window_set_content (ADW_APPLICATION_WINDOW (self), terminal);
    gtk_widget_grab_focus (terminal);

    /* Action group for right-click menu */
    g_autoptr (GSimpleActionGroup) ag = g_simple_action_group_new ();

#define ADD_ACTION(name_str, cb) G_STMT_START {                                 \
        g_autoptr (GSimpleAction) _a = g_simple_action_new (name_str, NULL);    \
        g_signal_connect (_a, "activate", G_CALLBACK (cb), self);               \
        g_action_map_add_action (G_ACTION_MAP (ag), G_ACTION (_a));             \
    } G_STMT_END

    ADD_ACTION ("copy-url",   action_copy_url);
    ADD_ACTION ("open-url",   action_open_url);
    ADD_ACTION ("copy",       action_copy);
    ADD_ACTION ("copy-html",  action_copy_html);
    ADD_ACTION ("paste",      action_paste);
    ADD_ACTION ("zoom",       action_zoom);
    ADD_ACTION ("dezoom",     action_dezoom);
    ADD_ACTION ("reset-zoom", action_reset_zoom);
    ADD_ACTION ("quit",       action_quit);

#undef ADD_ACTION

    gtk_widget_insert_action_group (GTK_WIDGET (self), "ctx", G_ACTION_GROUP (ag));

    /* Build right-click menu model */
    priv->url_section = g_menu_new ();

    g_autoptr (GMenu) menu = g_menu_new ();
    g_menu_append_section (menu, NULL, G_MENU_MODEL (priv->url_section));

    g_autoptr (GMenu) clipboard_section = g_menu_new ();
    g_menu_append (clipboard_section, _("Copy"),         "ctx.copy");
    g_menu_append (clipboard_section, _("Copy as HTML"), "ctx.copy-html");
    g_menu_append (clipboard_section, _("Paste"),        "ctx.paste");
    g_menu_append_section (menu, NULL, G_MENU_MODEL (clipboard_section));

    g_autoptr (GMenu) zoom_section = g_menu_new ();
    g_menu_append (zoom_section, _("Zoom"),       "ctx.zoom");
    g_menu_append (zoom_section, _("Dezoom"),     "ctx.dezoom");
    g_menu_append (zoom_section, _("Reset zoom"), "ctx.reset-zoom");
    g_menu_append_section (menu, NULL, G_MENU_MODEL (zoom_section));

    g_autoptr (GMenu) quit_section = g_menu_new ();
    g_menu_append (quit_section, _("Quit"), "ctx.quit");
    g_menu_append_section (menu, NULL, G_MENU_MODEL (quit_section));

    priv->popover = gtk_popover_menu_new_from_model (G_MENU_MODEL (menu));
    gtk_popover_set_has_arrow (GTK_POPOVER (priv->popover), FALSE);
    gtk_widget_set_parent (priv->popover, terminal);

    /* Click gesture for right-click menu and shift+click URL */
    GtkGesture *gesture = gtk_gesture_click_new ();
    gtk_gesture_single_set_button (GTK_GESTURE_SINGLE (gesture), 0);
    g_signal_connect (gesture, "pressed", G_CALLBACK (on_click_pressed), self);
    gtk_widget_add_controller (terminal, GTK_EVENT_CONTROLLER (gesture));

    /* VTE signals */
    priv->terminal_signals = g_signal_group_new (VTE_TYPE_TERMINAL);
    g_signal_group_connect (priv->terminal_signals, "child-exited",                                G_CALLBACK (on_child_exited),         self);
    g_signal_group_connect (priv->terminal_signals, "termprop-changed::" VTE_TERMPROP_XTERM_TITLE, G_CALLBACK (on_window_title_changed), self);
    g_signal_group_set_target (priv->terminal_signals, priv->terminal);

    /* Initialize title */
    on_window_title_changed (VTE_TERMINAL (terminal), NULL, self);
}

static void
germinal_window_dispose (GObject *object)
{
    GerminalWindowPrivate *priv = germinal_window_get_instance_private (GERMINAL_WINDOW (object));

    g_clear_pointer (&priv->popover, gtk_widget_unparent);
    g_clear_object (&priv->url_section);
    g_clear_object (&priv->settings_signals);
    g_clear_object (&priv->settings);
    g_clear_object (&priv->terminal_signals);
    g_clear_object (&priv->terminal);

    G_OBJECT_CLASS (germinal_window_parent_class)->dispose (object);
}

static void
germinal_window_init (GerminalWindow *self)
{
    GerminalWindowPrivate *priv = germinal_window_get_instance_private (self);

    GSettings *settings = priv->settings = germinal_settings_new ();

    priv->settings_signals = g_signal_group_new (G_TYPE_SETTINGS);
    g_signal_group_connect (priv->settings_signals, "changed::" DECORATED_KEY, G_CALLBACK (update_decorated), self);
    g_signal_group_set_target (priv->settings_signals, settings);

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
    gtk_window_maximize (GTK_WINDOW (self));
    gtk_window_present (GTK_WINDOW (self));
}

GtkWidget *
germinal_window_new (GtkApplication   *application,
                     GerminalTerminal *terminal)
{
    return g_object_new (GERMINAL_TYPE_WINDOW,
                         "application", application,
                         "terminal",    terminal,
                         NULL);
}
