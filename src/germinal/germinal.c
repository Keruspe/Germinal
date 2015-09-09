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

#include "germinal-cleanup.h"

#include <glib/gi18n-lib.h>

#include <stdlib.h>

static GStrv command = NULL;

static void
germinal_exit (GtkWidget *widget G_GNUC_UNUSED,
               gpointer   user_data)
{
    g_application_quit (G_APPLICATION (user_data));
}

static void
on_child_exited (VteTerminal *vteterminal,
                 gint         status,
                 gpointer     user_data)
{
    if (status)
        g_critical ("child exited with code %d", status);
    germinal_exit (GTK_WIDGET (vteterminal), user_data);
}

static gboolean
do_copy (GtkWidget *widget G_GNUC_UNUSED,
         gpointer   user_data)
{
    vte_terminal_copy_clipboard (VTE_TERMINAL (user_data));
    return TRUE;
}

static gboolean
do_paste (GtkWidget *widget G_GNUC_UNUSED,
          gpointer   user_data)
{
    vte_terminal_paste_clipboard (VTE_TERMINAL (user_data));
    return TRUE;
}

static gchar *
get_url (VteTerminal    *terminal,
         GdkEventButton *button_event)
{
    static gchar *url = NULL; /* cache the url */

    if (button_event) /* only access to cached url if no button_event available */
    {
        g_free (url); /* free previous url */
        glong column = (glong)button_event->x / vte_terminal_get_char_width (terminal);
        glong row = (glong)button_event->y / vte_terminal_get_char_height (terminal);
        gint tag; /* avoid stupid vte segv (said to be optional) */

        url = vte_terminal_match_check (terminal, column, row, &tag);
    }

    return url;
}

static gboolean
open_url (gchar *url)
{
    if (!url)
        return FALSE;

    g_autoptr (GError) error = NULL;
    /* Always strdup because we free later */
    g_autofree gchar *browser = g_strdup (g_getenv ("BROWSER"));

    /* If BROWSER is not in env, try xdg-open or fallback to firefox */
    if (!browser)
        browser = g_find_program_in_path ("xdg-open");
    if (!browser)
        browser = g_strdup ("firefox");

    gchar *cmd[] = {browser, url, NULL};

    if (!g_spawn_async (NULL, /* working directory */
                        cmd,
                        NULL, /* env */
                        G_SPAWN_SEARCH_PATH,
                        NULL, /* child setup */
                        NULL, /* child setup data */
                        NULL, /* child pid */
                        &error))
        g_warning ("%s \"%s %s\": %s", _("Couldn't exec"), browser, url, error->message);

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
    gchar *url = get_url (VTE_TERMINAL (user_data), NULL);

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
    return open_url (get_url (VTE_TERMINAL (user_data), NULL));
}

typedef enum {
    FONT_SIZE_DELTA_RESET = 0,
    FONT_SIZE_DELTA_INC = 1,
    FONT_SIZE_DELTA_DEC = -1,
    FONT_SIZE_DELTA_SET_DEFAULT = 42
} FontSizeDelta;

static void
update_font_size (VteTerminal  *terminal,
                  FontSizeDelta delta)
{
    static gdouble default_size = 0;

    GERMINAL_FONT_CLEANUP PangoFontDescription *font = pango_font_description_copy (vte_terminal_get_font (terminal));
    gdouble size = pango_font_description_get_size (font);

    switch (delta)
    {
    case FONT_SIZE_DELTA_SET_DEFAULT:
        default_size = size;
        return;
    case FONT_SIZE_DELTA_RESET:
        size = default_size;
        break;
    default:
        size = CLAMP (size / PANGO_SCALE + delta, 4., 144.) * PANGO_SCALE;
        break;
    }

    pango_font_description_set_size (font, size);
    vte_terminal_set_font (terminal, font);
}

static gboolean
do_zoom (GtkWidget *widget G_GNUC_UNUSED,
         gpointer   user_data)
{
    update_font_size (VTE_TERMINAL (user_data), FONT_SIZE_DELTA_INC);

    return TRUE;
}

static gboolean
do_dezoom (GtkWidget *widget G_GNUC_UNUSED,
           gpointer   user_data)
{
    update_font_size (VTE_TERMINAL (user_data), FONT_SIZE_DELTA_DEC);

    return TRUE;
}

static gboolean
do_reset_zoom (GtkWidget *widget G_GNUC_UNUSED,
               gpointer   user_data)
{
    update_font_size (VTE_TERMINAL (user_data), FONT_SIZE_DELTA_RESET);

    return TRUE;
}

static gboolean
do_quit (GtkWidget *widget)
{
    germinal_exit (NULL, gtk_window_get_application (GTK_WINDOW (widget)));

    return TRUE;
}

static gboolean
launch_cmd (const gchar *cmd)
{
    return g_spawn_command_line_async (cmd, NULL);
}

static gboolean
on_key_press (GtkWidget   *widget,
              GdkEventKey *event,
              gpointer     user_data)
{
    if (event->type != GDK_KEY_PRESS)
        return FALSE;

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
            return do_quit (widget);
        /* Window split (inspired by terminator) */
        case GDK_KEY_O:
            return launch_cmd ("tmux split-window -v");
        case GDK_KEY_E:
            return launch_cmd ("tmux split-window -h");
        /* Next/Previous window (tab) */
        case GDK_KEY_Tab:
            return launch_cmd ("tmux next-window");
        case GDK_KEY_ISO_Left_Tab:
            return launch_cmd ("tmux previous-window");
        /* New window (tab) */
        case GDK_KEY_T:
            return launch_cmd ("tmux new-window");
        /* Next/Previous pane */
        case GDK_KEY_N:
            return launch_cmd ("tmux select-pane -t :.+");
        case GDK_KEY_P:
            return launch_cmd ("tmux select-pane -t :.-");
        /* Close current pane */
        case GDK_KEY_W:
            return launch_cmd ("tmux kill-pane");
        /* Resize current pane */
        case GDK_KEY_X:
            return launch_cmd ("tmux resize-pane -Z");
        }
    }

    return GTK_WIDGET_GET_CLASS (user_data)->key_press_event (user_data, event);
}

static gboolean
on_button_press (GtkWidget      *widget,
                 GdkEventButton *button_event,
                 gpointer        user_data)
{
    if (button_event->type != GDK_BUTTON_PRESS)
        return FALSE;

    gchar *url = get_url (VTE_TERMINAL (widget), button_event);

    /* Shift + Left clic */
    if ((button_event->button == GDK_BUTTON_PRIMARY) &&
        (button_event->state & GDK_SHIFT_MASK))
            return open_url (url);
    else if (button_event->button == GDK_BUTTON_SECONDARY)
    {
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
        gtk_menu_popup (GTK_MENU (user_data),
                        NULL, /* parent menu_shell */
                        NULL, /* parent menu_item */
                        NULL, /* menu position func */
                        NULL, /* user_data */
                        button_event->button,
                        button_event->time);
        return TRUE;
    }

    return FALSE;
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
update_font (GSettings   *settings,
             const gchar *key,
             gpointer     user_data)
{
    g_autofree gchar *setting = get_setting (settings, key);
    GERMINAL_FONT_CLEANUP PangoFontDescription *font = pango_font_description_from_string (setting);
    VteTerminal *term = user_data;

    vte_terminal_set_font (term, font);
    update_font_size (term, FONT_SIZE_DELTA_SET_DEFAULT);
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
germinal_activate (GApplication *application)
{
    g_autoptr (GError) error = NULL;
    GtkApplication *app = GTK_APPLICATION (application);

    /* Create window */
    GtkWidget *window = gtk_application_window_new (app);
    GtkWindow *win = GTK_WINDOW (window);

    /* Vte settings */
    GtkWidget *terminal = vte_terminal_new ();
    VteTerminal *term = VTE_TERMINAL (terminal);

    vte_terminal_set_mouse_autohide      (term, TRUE);
    vte_terminal_set_audible_bell        (term, FALSE);
    vte_terminal_set_scroll_on_output    (term, FALSE);
    vte_terminal_set_scroll_on_keystroke (term, TRUE);

    /* Fill window */
    gtk_container_add (GTK_CONTAINER (window), terminal);
    gtk_widget_grab_focus (terminal);
    gtk_widget_show_all (window);

    /* Window settings */
    gtk_window_set_decorated (win, FALSE);
    gtk_window_maximize (win);

    /* Url matching stuff */
    g_autoptr (GRegex) url_regexp = g_regex_new (URL_REGEXP,
                                                 G_REGEX_CASELESS | G_REGEX_OPTIMIZE,
                                                 G_REGEX_MATCH_NOTEMPTY,
                                                 NULL); /* error */

    vte_terminal_match_add_gregex (term, url_regexp, 0);

    /* Apply and track user settings */
    g_autoptr (GSettings) settings = g_settings_new ("org.gnome.Germinal");

    SETTING_SIGNAL (BACKCOLOR, colors);
    SETTING_SIGNAL (FORECOLOR, colors);
    SETTING_SIGNAL (PALETTE,   colors);
    update_colors (settings, NULL, terminal);

    SETTING (FONT,                 font);
    SETTING (SCROLLBACK,           scrollback);
    SETTING (WORD_CHAR_EXCEPTIONS, word_char_exceptions);

    /* Launch base command */
    g_autofree gchar *cwd = g_get_current_dir ();

    if (G_LIKELY (!command))
    {
        g_autofree gchar *setting = get_setting (settings, STARTUP_COMMAND_KEY);
        command = g_strsplit (setting , " ", 0);
    }

    /* Override TERM */
    g_auto (GStrv) envp = g_environ_setenv (NULL, "TERM", get_setting (settings, TERM_KEY), TRUE);

    /* Spawn our command */
    if (!vte_terminal_spawn_sync (term, VTE_PTY_DEFAULT, cwd, command, envp, G_SPAWN_SEARCH_PATH,
                                  NULL, /* child_setup */
                                  NULL, /* child_setup_data */
                                  NULL, /* child_pid */
                                  NULL, /* cancellable */
                                  &error))
    {
        g_critical ("%s", error->message);
        return;
    }

    /* Populate right click menu */
    GtkWidget *menu = gtk_menu_new ();

    MENU_ACTION (copy_url, _("Copy url"));
    MENU_ACTION (open_url, _("Open url"));

    MENU_SEPARATOR;

    MENU_ACTION (copy,  _("Copy"));
    MENU_ACTION (paste, _("Paste"));

    MENU_SEPARATOR;

    MENU_ACTION (zoom,       _("Zoom"));
    MENU_ACTION (dezoom,     _("Dezoom"));
    MENU_ACTION (reset_zoom, _("Reset zoom"));

    MENU_SEPARATOR;

    MENU_ACTION (quit,       _("Quit"));

    gtk_widget_show_all (menu);

    /* Bind signals */
    CONNECT_SIGNAL (terminal, "button-press-event", on_button_press, menu);
    CONNECT_SIGNAL (terminal, "child-exited",       on_child_exited, application);
    CONNECT_SIGNAL (window,   "key-press-event",    on_key_press,    terminal);
}

static gint
germinal_handle_options (GApplication *gapp,
                         GVariantDict *options)
{
    g_variant_lookup ((GVariant *) options, G_OPTION_REMAINING, "as", &command, NULL);

    return 0;
}

gint
main(gint   argc,
     gchar *argv[])
{
    /* Gettext and gtk initialization */
    textdomain(GETTEXT_PACKAGE);
    bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);
    bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");

    gtk_init (&argc, &argv);
    g_object_set (gtk_settings_get_default (), "gtk-application-prefer-dark-theme", TRUE, NULL);

    /* GtkApplication initialization */
    GtkApplication *app = gtk_application_new ("org.gnome.Germinal", G_APPLICATION_SEND_ENVIRONMENT | G_APPLICATION_NON_UNIQUE);
    GApplication *gapp = G_APPLICATION (app);
    GApplicationClass *klass = G_APPLICATION_GET_CLASS (gapp);
    g_autoptr (GError) error = NULL;

    g_application_add_main_option (gapp, G_OPTION_REMAINING, 'e', 0, G_OPTION_ARG_STRING_ARRAY, N_("the command to launch"), "command");

    klass->activate = germinal_activate;
    klass->handle_local_options = germinal_handle_options;
    g_application_register (gapp, NULL, &error);

    if (error)
    {
        fprintf (stderr, "%s: %s\n", _("Failed to register the gtk application"), error->message);
        return EXIT_FAILURE;
    }

    if (g_application_get_is_remote (gapp))
    {
        g_application_activate (gapp);
        return EXIT_SUCCESS;
    }

    /* Launch program */
    gint ret = g_application_run (gapp, argc, argv);

    /* Free memory */
    g_free (get_url (NULL, NULL));
    g_free (update_colors (NULL, NULL, NULL));

    return ret;
}
