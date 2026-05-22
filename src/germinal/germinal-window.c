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
    GSignalGroup     *search_entry_signals;

    GtkWidget        *popover;
    GMenu            *url_section;

    GtkWidget        *search_bar;
    GtkWidget        *search_entry;

    guint             spawn_source_id;
} GerminalWindowPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (GerminalWindow, germinal_window, ADW_TYPE_APPLICATION_WINDOW)

typedef struct {
    GerminalWindow   *win;
    GerminalTerminal *term;
    GStrv             command;
} GerminalCommandData;

static void
germinal_command_data_free (gpointer user_data)
{
    g_autofree GerminalCommandData *data = user_data;
    g_strfreev (data->command);
}

static gboolean
germinal_spawn_command (gpointer user_data)
{
    GerminalCommandData *data = user_data;
    GerminalWindowPrivate *priv = germinal_window_get_instance_private (data->win);

    if (!gtk_widget_get_realized (GTK_WIDGET (data->win)) || !gtk_widget_get_realized (GTK_WIDGET (data->term)))
        return G_SOURCE_CONTINUE;

    priv->spawn_source_id = 0;
    germinal_terminal_spawn_command (data->term, g_steal_pointer (&data->command));

    return G_SOURCE_REMOVE;
}

void
germinal_window_spawn_command (GerminalWindow *self,
                               GStrv           command)
{
    GerminalWindowPrivate *priv = germinal_window_get_instance_private (self);

    GerminalCommandData *data = g_new0 (GerminalCommandData, 1);
    data->win = self;
    data->term = priv->terminal;
    data->command = command;

    priv->spawn_source_id = g_idle_add_full (G_PRIORITY_DEFAULT_IDLE, germinal_spawn_command, data, germinal_command_data_free);
    g_source_set_name_by_id (priv->spawn_source_id, "[germinal] spawn-command");
}

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
update_search_state (GerminalWindow *self)
{
    GerminalWindowPrivate *priv = germinal_window_get_instance_private (self);
    const gchar *text = gtk_editable_get_text (GTK_EDITABLE (priv->search_entry));

    if (text && *text)
        germinal_terminal_search (priv->terminal, text);
    else
        germinal_terminal_search_stop (priv->terminal);
}

static void
on_search_changed (GtkSearchEntry *entry G_GNUC_UNUSED,
                   gpointer        user_data)
{
    update_search_state (GERMINAL_WINDOW (user_data));
}

static void
on_search_entry_next_match (GtkSearchEntry *entry G_GNUC_UNUSED,
                            gpointer        user_data)
{
    GerminalWindowPrivate *priv = germinal_window_get_instance_private (GERMINAL_WINDOW (user_data));
    germinal_terminal_search_next (priv->terminal);
}

static void
on_search_entry_prev_match (GtkSearchEntry *entry G_GNUC_UNUSED,
                            gpointer        user_data)
{
    GerminalWindowPrivate *priv = germinal_window_get_instance_private (GERMINAL_WINDOW (user_data));
    germinal_terminal_search_prev (priv->terminal);
}

static void
on_stop_search (GtkSearchEntry *entry G_GNUC_UNUSED,
                gpointer        user_data)
{
    GerminalWindow *self = GERMINAL_WINDOW (user_data);
    GerminalWindowPrivate *priv = germinal_window_get_instance_private (self);

    gtk_revealer_set_reveal_child (GTK_REVEALER (priv->search_bar), FALSE);
    germinal_terminal_search_stop (priv->terminal);
    gtk_widget_grab_focus (GTK_WIDGET (priv->terminal));
}

static gboolean
on_window_key_pressed (GtkEventControllerKey *controller G_GNUC_UNUSED,
                       guint                  keyval,
                       guint                  keycode G_GNUC_UNUSED,
                       GdkModifierType        state,
                       gpointer               user_data)
{
    GerminalWindow *self = GERMINAL_WINDOW (user_data);
    GerminalWindowPrivate *priv = germinal_window_get_instance_private (self);

    if ((state & GDK_CONTROL_MASK) && keyval == GDK_KEY_f)
    {
        gtk_revealer_set_reveal_child (GTK_REVEALER (priv->search_bar), TRUE);
        gtk_widget_grab_focus (priv->search_entry);
        update_search_state (self);
        return GDK_EVENT_STOP;
    }

    return GDK_EVENT_PROPAGATE;
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

    /* Search bar */
    GtkWidget *search_entry = priv->search_entry = gtk_search_entry_new ();
    gtk_widget_set_hexpand (search_entry, TRUE);

    GtkWidget *search_bar = priv->search_bar = gtk_revealer_new ();
    gtk_revealer_set_transition_type (GTK_REVEALER (search_bar), GTK_REVEALER_TRANSITION_TYPE_SLIDE_DOWN);
    gtk_revealer_set_child (GTK_REVEALER (search_bar), search_entry);

    priv->search_entry_signals = g_signal_group_new (GTK_TYPE_SEARCH_ENTRY);
    g_signal_group_connect (priv->search_entry_signals, "search-changed", G_CALLBACK (on_search_changed),         self);
    g_signal_group_connect (priv->search_entry_signals, "activate",       G_CALLBACK (on_search_entry_next_match), self);
    g_signal_group_connect (priv->search_entry_signals, "next-match",     G_CALLBACK (on_search_entry_next_match), self);
    g_signal_group_connect (priv->search_entry_signals, "previous-match", G_CALLBACK (on_search_entry_prev_match), self);
    g_signal_group_connect (priv->search_entry_signals, "stop-search",    G_CALLBACK (on_stop_search),             self);
    g_signal_group_set_target (priv->search_entry_signals, search_entry);

    GtkEventController *window_key_ctrl = gtk_event_controller_key_new ();
    gtk_event_controller_set_propagation_phase (window_key_ctrl, GTK_PHASE_CAPTURE);
    g_signal_connect (window_key_ctrl, "key-pressed", G_CALLBACK (on_window_key_pressed), self);
    gtk_widget_add_controller (GTK_WIDGET (self), window_key_ctrl);

    /* Fill window */
    GtkWidget *box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_append (GTK_BOX (box), search_bar);
    gtk_box_append (GTK_BOX (box), terminal);
    gtk_widget_set_vexpand (terminal, TRUE);

    adw_application_window_set_content (ADW_APPLICATION_WINDOW (self), box);
    gtk_widget_grab_focus (terminal);

    /* Action group for right-click menu */
    static const GActionEntry ctx_actions[] = {
        { .name = "copy-url",   .activate = action_copy_url   },
        { .name = "open-url",   .activate = action_open_url   },
        { .name = "copy",       .activate = action_copy       },
        { .name = "copy-html",  .activate = action_copy_html  },
        { .name = "paste",      .activate = action_paste      },
        { .name = "zoom",       .activate = action_zoom       },
        { .name = "dezoom",     .activate = action_dezoom     },
        { .name = "reset-zoom", .activate = action_reset_zoom },
        { .name = "quit",       .activate = action_quit       },
    };

    g_autoptr (GSimpleActionGroup) ag = g_simple_action_group_new ();
    g_action_map_add_action_entries (G_ACTION_MAP (ag), ctx_actions, G_N_ELEMENTS (ctx_actions), self);
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

    g_clear_handle_id (&priv->spawn_source_id, g_source_remove);
    g_clear_pointer (&priv->popover, gtk_widget_unparent);
    g_clear_object (&priv->url_section);
    g_clear_object (&priv->search_entry_signals);
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
