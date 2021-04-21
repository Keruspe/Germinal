/*
 * This file is part of Germinal.
 *
 * Copyright 2011-2014 Marc-Antoine Perennou <Marc-Antoine@Perennou.com>
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

#include <stdlib.h>

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
            GList *children = gtk_container_get_children (GTK_CONTAINER (user_data));
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
on_child_exited (VteTerminal *vteterminal,
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
    vte_terminal_copy_clipboard_format (VTE_TERMINAL (user_data), VTE_FORMAT_TEXT);
    return TRUE;
}

static gboolean
do_copy_html (GtkWidget *widget G_GNUC_UNUSED,
              gpointer   user_data)
{
    vte_terminal_copy_clipboard_format (VTE_TERMINAL (user_data), VTE_FORMAT_HTML);
    return TRUE;
}

static gboolean
do_paste (GtkWidget *widget G_GNUC_UNUSED,
          gpointer   user_data)
{
    vte_terminal_paste_clipboard (VTE_TERMINAL (user_data));
    return TRUE;
}

static void
copy_text (GdkAtom      selection,
           const gchar *text)
{
    GtkClipboard *clip = gtk_clipboard_get (selection);

    gtk_clipboard_set_text (clip, text, -1);
}

static gboolean
do_copy_url (GtkWidget *widget G_GNUC_UNUSED,
             gpointer   user_data)
{
    gchar *url = germinal_terminal_get_url (GERMINAL_TERMINAL (user_data), NULL);

    if (!url)
        return FALSE;

    copy_text (GDK_SELECTION_CLIPBOARD, url);
    copy_text (GDK_SELECTION_PRIMARY, url);

    return TRUE;
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
do_quit (GtkWidget *widget,
         gpointer   user_data)
{
    gtk_window_close (GTK_WINDOW (user_data));

    return TRUE;
}

static gboolean
launch_cmd (GerminalTerminal *terminal,
            const gchar      *_cmd)
{
    g_auto (GStrv) cmd = NULL;

    if (!g_shell_parse_argv (_cmd, NULL, &cmd, NULL))
        return FALSE;

    return germinal_terminal_spawn (terminal, cmd, NULL);
}

static gboolean
on_key_press (GtkWidget   *widget,
              GdkEventKey *event,
              gpointer     user_data)
{
    if (event->type != GDK_KEY_PRESS)
        return FALSE;

    GerminalTerminal *terminal = GERMINAL_TERMINAL (user_data);

    /* Ctrl + foo */
    if (event->state & GDK_CONTROL_MASK)
    {
        switch (event->keyval)
        {
        /* Clipboard */
        case GDK_KEY_C:
            return do_copy (widget, user_data);
        case GDK_KEY_V:
            return do_paste (widget, user_data);
        /* Zoom */
        case GDK_KEY_KP_Add:
        case GDK_KEY_plus:
            return do_zoom (widget, user_data);
        case GDK_KEY_KP_Subtract:
        case GDK_KEY_minus:
            return do_dezoom (widget, user_data);
        case GDK_KEY_KP_0:
        case GDK_KEY_0:
            return do_reset_zoom (widget, user_data);
        /* Quit */
        case GDK_KEY_Q:
            return do_quit (widget, widget);
        /* Window split (inspired by terminator) */
        case GDK_KEY_O:
            return launch_cmd (terminal, "tmux split-window -v");
        case GDK_KEY_E:
            return launch_cmd (terminal, "tmux split-window -h");
        /* Next/Previous window (tab) */
        case GDK_KEY_Tab:
            return launch_cmd (terminal, "tmux next-window");
        case GDK_KEY_ISO_Left_Tab:
            return launch_cmd (terminal, "tmux previous-window");
        /* New window (tab) */
        case GDK_KEY_T:
            return launch_cmd (terminal, "tmux new-window");
        /* Next/Previous pane */
        case GDK_KEY_N:
            return launch_cmd (terminal, "tmux select-pane -t :.+");
        case GDK_KEY_P:
            return launch_cmd (terminal, "tmux select-pane -t :.-");
        /* Close current pane */
        case GDK_KEY_W:
            return launch_cmd (terminal, "tmux kill-pane");
        /* Resize current pane */
        case GDK_KEY_X:
            return launch_cmd (terminal, "tmux resize-pane -Z");
        }
        
        if (germinal_terminal_is_zero (terminal, event->hardware_keycode))
            return do_reset_zoom (widget, user_data);
    }

    return GTK_WIDGET_GET_CLASS (user_data)->key_press_event (user_data, event);
}

typedef struct {
    GtkWidget        *win;
    GerminalTerminal *term;
    GStrv             command;
} GerminalCommandData;

static gboolean
germinal_spawn_command (gpointer user_data)
{
    g_autofree GerminalCommandData *data = user_data;

    if (!gtk_widget_get_realized (data->win) || !gtk_widget_get_realized ((GtkWidget *) data->term))
    {
        data = NULL;
        return G_SOURCE_CONTINUE;
    }

    germinal_terminal_spawn_command (data->term, data->command);

    return G_SOURCE_REMOVE;
}

static int
germinal_create_window (GApplication *application,
                        GStrv         command)
{
    /* Create window */
    GtkWidget *window = germinal_window_new (GTK_APPLICATION (application));
    GtkWindow *win = GTK_WINDOW (window);
    GtkWidget *terminal = germinal_terminal_new ();
    VteTerminal *term = VTE_TERMINAL (terminal);

    /* Fill window */
    gtk_container_add (GTK_CONTAINER (window), terminal);
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
    CONNECT_SIGNAL (window,   "key-press-event",      on_key_press,            terminal);
    CONNECT_SIGNAL (terminal, "button-press-event",   on_button_press,         menu);
    CONNECT_SIGNAL (terminal, "child-exited",         on_child_exited,         win);
    CONNECT_SIGNAL (terminal, "window-title-changed", on_window_title_changed, win);

    /* Initialize title */
    on_window_title_changed (term, win);

    /* Show the window */
    gtk_widget_show_all (menu);
    gtk_widget_show_all (window);

    /* Window settings */
    gtk_window_maximize (win);

    /* Launch the command */
    GerminalCommandData *data = g_new0 (GerminalCommandData, 1);
    data->win = window;
    data->term = GERMINAL_TERMINAL (term);
    data->command = command;
    g_idle_add (germinal_spawn_command, data);

    return EXIT_SUCCESS;
}

static gint
germinal_command_line (GApplication            *application,
                       GApplicationCommandLine *command_line)
{
    GVariantDict *dict = g_application_command_line_get_options_dict (command_line);

    if (g_variant_dict_contains (dict, "version"))
    {
        g_application_command_line_print (command_line, PACKAGE_STRING "\n");
        return 0;
    }

    g_autoptr (GVariant) v = g_variant_dict_lookup_value (dict, G_OPTION_REMAINING, NULL);
    GStrv command = (v) ? g_variant_dup_strv (v, NULL) : NULL;

    return germinal_create_window (application, command);
}

static void
germinal_activate (GApplication *application)
{
    germinal_create_window (application, NULL);
}

gint
main (gint   argc,
      gchar *argv[])
{
    /* Gettext and gtk initialization */
    textdomain(GETTEXT_PACKAGE);
    bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);
    bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");

    gtk_init (&argc, &argv);
    g_object_set (gtk_settings_get_default (), "gtk-application-prefer-dark-theme", TRUE, NULL);

    /* GtkApplication initialization */
    GtkApplication *app = gtk_application_new ("org.gnome.Germinal", G_APPLICATION_HANDLES_COMMAND_LINE|G_APPLICATION_SEND_ENVIRONMENT);
    GApplication *gapp = G_APPLICATION (app);
    GApplicationClass *klass = G_APPLICATION_GET_CLASS (gapp);

    g_application_add_main_option (gapp, "version",          'v', 0, G_OPTION_ARG_NONE,         N_("display the version"),   NULL);
    g_application_add_main_option (gapp, G_OPTION_REMAINING, 'e', 0, G_OPTION_ARG_STRING_ARRAY, N_("the command to launch"), "command");

    klass->command_line = germinal_command_line;
    klass->activate = germinal_activate;

    /* Launch program */
    return g_application_run (gapp, argc, argv);
}
