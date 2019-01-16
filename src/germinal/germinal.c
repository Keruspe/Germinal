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

#define PCRE2_CODE_UNIT_WIDTH 0
#include <pcre2.h>

#include <stdlib.h>

typedef void (*GerminalSettingsFunc) (GSettings   *settings,
                                      const gchar *key,
                                      gpointer     user_data);

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

static gchar *
get_setting (GSettings   *settings,
             const gchar *name)
{
    return (name) ? g_settings_get_string (settings, name) : NULL;
}

static GdkRGBA *
get_palette (GSettings   *settings,
             const gchar *name,
             gsize       *palette_size)
{
    g_auto (GStrv) colors = NULL;
    guint size, i;
    GdkRGBA *palette;

    colors = g_settings_get_strv (settings, name);
    size = g_strv_length (colors);

    if (!((size == 0) ||
          (size == 8) ||
          (size == 16) ||
          (size == 24) ||
          ((size >= 25) && (size <= 255))))
    {
        g_settings_reset (settings, name);
        return get_palette (settings, name, palette_size);
    }

    palette = g_new(GdkRGBA, size);

    for (i = 0 ; i < size ; ++i)
        gdk_rgba_parse (&palette[i], colors[i]);

    *palette_size = size;

    return palette;
}

static void
update_scrollback (GSettings   *settings,
                   const gchar *key,
                   gpointer     user_data)
{
    vte_terminal_set_scrollback_lines (VTE_TERMINAL (user_data), g_settings_get_int (settings, key));
}

static void
update_word_char_exceptions (GSettings   *settings,
                             const gchar *key,
                             gpointer     user_data)
{
    g_autofree gchar *setting = get_setting (settings, key);

    vte_terminal_set_word_char_exceptions (VTE_TERMINAL (user_data), setting);
}

static void
update_bell (GSettings   *settings,
             const gchar *key,
             gpointer     user_data)
{
    gboolean bell = g_settings_get_boolean (settings, key);
    VteTerminal *term = user_data;

    vte_terminal_set_audible_bell (term, bell);
}

static void
update_allow_bold (GSettings   *settings,
                   const gchar *key,
                   gpointer     user_data)
{
    gboolean allow_bold = g_settings_get_boolean (settings, key);
    VteTerminal *term = user_data;

    vte_terminal_set_allow_bold (term, allow_bold);
}

static void
update_decorated (GSettings   *settings,
                  const gchar *key,
                  gpointer     user_data)
{
    gboolean decorated = g_settings_get_boolean (settings, key);
    GtkWindow *win = GTK_WINDOW (gtk_widget_get_parent (user_data));

    gtk_window_set_decorated (win, decorated);
    gtk_window_set_hide_titlebar_when_maximized (win, !decorated);
}

static void
update_font (GSettings   *settings,
             const gchar *key,
             gpointer     user_data)
{
    g_autofree gchar *setting = get_setting (settings, key);

    germinal_terminal_update_font (GERMINAL_TERMINAL (user_data), setting);
}

static GdkRGBA *
update_colors (GSettings   *settings,
               const gchar *key,       /* NULL for initialization */
               gpointer     user_data) /* NULL to get the palette to free it */
{
    static GdkRGBA forecolor, backcolor;
    static GdkRGBA *palette = NULL;
    static gsize palette_size;

    if (!user_data)
        return palette;

    if (!key)
    {
        g_autofree gchar *backcolor_str = get_setting (settings, BACKCOLOR_KEY);
        g_autofree gchar *forecolor_str = get_setting (settings, FORECOLOR_KEY);

        gdk_rgba_parse (&backcolor, backcolor_str);
        gdk_rgba_parse (&forecolor, forecolor_str);

        palette = get_palette(settings, PALETTE_KEY, &palette_size);
    }
    else if (strcmp (key, BACKCOLOR_KEY) == 0)
    {
        g_autofree gchar *backcolor_str = get_setting (settings, BACKCOLOR_KEY);

        gdk_rgba_parse (&backcolor, backcolor_str);
    }
    else if (strcmp (key, FORECOLOR_KEY) == 0)
    {
        g_autofree gchar *forecolor_str = get_setting (settings, FORECOLOR_KEY);

        gdk_rgba_parse (&forecolor, forecolor_str);
    }
    else if (strcmp (key, PALETTE_KEY) == 0)
    {
        g_free(palette);
        palette = get_palette(settings, PALETTE_KEY, &palette_size);
    }

    vte_terminal_set_colors (VTE_TERMINAL (user_data),
                             &forecolor,
                             &backcolor,
                             palette,
                             palette_size);

    return NULL;
}

static void
on_temrinal_command_spawned (VteTerminal *terminal G_GNUC_UNUSED,
                             GPid         pid      G_GNUC_UNUSED,
                             GError      *error,
                             gpointer     user_data)
{
    if (error)
    {
        g_critical ("%s", error->message);
        g_error_free (error);
        exit (EXIT_FAILURE);
    }
    gtk_widget_show_all (user_data);
}

static int
germinal_create_window (GApplication *application,
                        GStrv         command)
{
    g_autoptr (GError) error = NULL;
    g_auto (GStrv) _free_command = command;

    /* Create window */
    GtkWidget *window = germinal_window_new (GTK_APPLICATION (application));
    GtkWindow *win = GTK_WINDOW (window);

    /* Vte settings */
    GtkWidget *terminal = germinal_terminal_new ();
    VteTerminal *term = VTE_TERMINAL (terminal);

    vte_terminal_set_mouse_autohide      (term, TRUE);
    vte_terminal_set_scroll_on_output    (term, FALSE);
    vte_terminal_set_scroll_on_keystroke (term, TRUE);

    /* Window settings */
    gtk_window_maximize (win);

    /* Fill window */
    gtk_container_add (GTK_CONTAINER (window), terminal);
    gtk_widget_grab_focus (terminal);

    /* Url matching stuff */
    g_autoptr (VteRegex) url_regexp = vte_regex_new_for_match (URL_REGEXP,
                                                              strlen (URL_REGEXP),
                                                              PCRE2_CASELESS | PCRE2_NOTEMPTY | PCRE2_MULTILINE,
                                                              NULL); /* error */

    vte_terminal_match_add_regex (term, url_regexp, 0);

    /* Apply user settings */
    GSettings *settings = g_object_get_data (G_OBJECT (application), "germinal-settings");

    update_bell                 (settings, AUDIBLE_BELL_KEY,         terminal);
    update_allow_bold           (settings, ALLOW_BOLD_KEY,           terminal);
    update_decorated            (settings, DECORATED_KEY,            terminal);
    update_colors               (settings, NULL,                     terminal);
    update_font                 (settings, FONT_KEY,                 terminal);
    update_scrollback           (settings, SCROLLBACK_KEY,           terminal);
    update_word_char_exceptions (settings, WORD_CHAR_EXCEPTIONS_KEY, terminal);

    /* Launch base command */
    if (G_LIKELY (!command))
    {
        g_autofree gchar *setting = get_setting (settings, STARTUP_COMMAND_KEY);
        command = g_strsplit (setting , " ", 0);
    }

    /* Override TERM */
    g_auto (GStrv) envp = g_environ_setenv (g_get_environ (), "TERM", get_setting (settings, TERM_KEY), TRUE);

    /* Spawn our command */
    vte_terminal_spawn_async (term, VTE_PTY_DEFAULT, g_get_home_dir (), command, envp, G_SPAWN_SEARCH_PATH,
                              NULL,  /* child_setup */
                              NULL,  /* child_setup_data */
                              NULL,  /* child_setup_data_destroy */
                              -1,    /* timeout */
                              NULL,  /* cancellable */
                              on_temrinal_command_spawned,
                              window);

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

static void
germinal_windows_foreach (GtkApplication      *application,
                          GerminalSettingsFunc fn,
                          GSettings           *settings,
                          const gchar         *key)
{
    for (GList *w = gtk_application_get_windows (application); w; w = w->next)
        fn (settings, key, gtk_container_get_children (GTK_CONTAINER (w->data))->data);
}

SETTING_UPDATE_FUNC (bell);
SETTING_UPDATE_FUNC (allow_bold);
SETTING_UPDATE_FUNC (colors);
SETTING_UPDATE_FUNC (decorated);
SETTING_UPDATE_FUNC (font);
SETTING_UPDATE_FUNC (scrollback);
SETTING_UPDATE_FUNC (word_char_exceptions);

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

    /* track user settings */
    g_autoptr (GSettings) settings = g_settings_new ("org.gnome.Germinal");

    SETTING_SIGNAL (AUDIBLE_BELL,         bell);
    SETTING_SIGNAL (ALLOW_BOLD,           allow_bold);
    SETTING_SIGNAL (BACKCOLOR,            colors);
    SETTING_SIGNAL (FORECOLOR,            colors);
    SETTING_SIGNAL (PALETTE,              colors);
    SETTING_SIGNAL (DECORATED,            decorated);
    SETTING_SIGNAL (FONT,                 font);
    SETTING_SIGNAL (SCROLLBACK,           scrollback);
    SETTING_SIGNAL (WORD_CHAR_EXCEPTIONS, word_char_exceptions);

    g_object_set_data (G_OBJECT (app), "germinal-settings", settings);

    /* Launch program */
    gint ret = g_application_run (gapp, argc, argv);

    /* Free memory */
    g_free (update_colors (NULL, NULL, NULL));

    SETTING_SIGNAL_CLEANUP (AUDIBLE_BELL);
    SETTING_SIGNAL_CLEANUP (BACKCOLOR);
    SETTING_SIGNAL_CLEANUP (FORECOLOR);
    SETTING_SIGNAL_CLEANUP (PALETTE);
    SETTING_SIGNAL_CLEANUP (FONT);
    SETTING_SIGNAL_CLEANUP (SCROLLBACK);
    SETTING_SIGNAL_CLEANUP (WORD_CHAR_EXCEPTIONS);

    return ret;
}
