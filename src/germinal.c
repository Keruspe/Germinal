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

#include "config.h"
#include "germinal-cleanup.h"

#include <glib/gi18n-lib.h>

static void
germinal_exit (GtkWidget *widget    G_GNUC_UNUSED,
               gpointer   user_data G_GNUC_UNUSED)
{
    gtk_main_quit ();
}

static void
on_child_exited (VteTerminal *vteterminal,
                 gint         status,
                 gpointer     user_data)
{
    if (status)
        g_critical ("child exited with code %d. Did you add 'new-session' to your '~/.tmux.conf'?", status);
    germinal_exit (GTK_WIDGET (vteterminal), user_data);
}

static gboolean
do_copy (GtkWidget *widget    G_GNUC_UNUSED,
         gpointer   user_data G_GNUC_UNUSED)
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
        url = vte_terminal_match_check (terminal,
                                        column,
                                        row,
                                        &tag);
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
    if (!browser && !(browser = g_find_program_in_path ("xdg-open")))
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

static gboolean
do_copy_url (GtkWidget *widget G_GNUC_UNUSED,
             gpointer   user_data)
{
    gchar *url = get_url (VTE_TERMINAL (user_data), NULL);
    if (!url)
        return FALSE;
    GtkClipboard *clip = gtk_clipboard_get (GDK_SELECTION_CLIPBOARD);
    gtk_clipboard_set_text (clip, url, -1);
    clip = gtk_clipboard_get (GDK_SELECTION_PRIMARY);
    gtk_clipboard_set_text (clip, url, -1);
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
    PangoFontDescription GERMINAL_FONT_CLEANUP *font = pango_font_description_copy (vte_terminal_get_font (terminal));
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
do_quit (GtkWidget *widget    G_GNUC_UNUSED,
         gpointer   user_data G_GNUC_UNUSED)
{
    gtk_main_quit ();
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
        /* Ctrl + Shift + foo */
        if (event->state & GDK_SHIFT_MASK)
        {
            switch (event->keyval)
            {
            case GDK_KEY_C:
                return do_copy (widget, user_data);
            case GDK_KEY_V:
                return do_paste (widget, user_data);
            case GDK_KEY_Q:
                return do_quit (widget, user_data);
            case GDK_KEY_O:
                return launch_cmd ("tmux split-window -v");
            case GDK_KEY_E:
                return launch_cmd ("tmux split-window -h");
            }
        }
        switch (event->keyval)
        {
        case GDK_KEY_KP_Add:
        case GDK_KEY_plus:
            return do_zoom (widget, user_data);
        case GDK_KEY_KP_Subtract:
        case GDK_KEY_minus:
            return do_dezoom (widget, user_data);
        case GDK_KEY_KP_0:
        case GDK_KEY_0:
            return do_reset_zoom (widget, user_data);
        }
    }

    return FALSE;
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
    if ((button_event->button == 1) &&
        (button_event->state & GDK_SHIFT_MASK))
            return open_url (url);
    else if (button_event->button == 3)
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
             gsize *palette_size)
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
    PangoFontDescription GERMINAL_FONT_CLEANUP *font = pango_font_description_from_string (setting);
    vte_terminal_set_font (VTE_TERMINAL (user_data), font);
    update_font_size (VTE_TERMINAL (user_data), FONT_SIZE_DELTA_SET_DEFAULT);
}

static GdkRGBA *
update_colors (GSettings   *settings,
               const gchar *key, /* NULL for initialization */
               gpointer     user_data) /* NULL to get the palette */
{
    static GdkRGBA forecolor, backcolor;
    static GdkRGBA *palette = NULL;
    static gsize palette_size;

    if (!user_data)
        return palette;

    if (key == NULL)
    {
        g_autofree gchar *backcolor_str = get_setting (settings, BACKCOLOR_KEY);
        gdk_rgba_parse (&backcolor, backcolor_str);
        g_autofree gchar *forecolor_str = get_setting (settings, FORECOLOR_KEY);
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

int
main(int   argc,
     char *argv[])
{
    g_autoptr (GError) error = NULL;

    /* Options */
    g_auto (GStrv) command = NULL;
    GOptionEntry options[] =
    {
        { G_OPTION_REMAINING, 'e', 0, G_OPTION_ARG_STRING_ARRAY, &command, N_("the command to launch"), "command" },
        { NULL, '\0', 0, G_OPTION_ARG_NONE, NULL, NULL, NULL }
    };

    /* Gettext and gtk initialization */
    textdomain(GETTEXT_PACKAGE);
    bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);
    bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
    if (!gtk_init_with_args (&argc,
                             &argv,
                             N_(" - minimalist vte-based terminal emulator"),
                             options,
                             GETTEXT_PACKAGE,
                             &error))
    {
        g_critical ("%s", error->message);
        return 1;
    }

    GtkWidget *window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

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
    gtk_window_set_decorated (GTK_WINDOW (window), FALSE);
    gtk_window_maximize (GTK_WINDOW (window));

    /* Url matching stuff */
    g_autoptr (GRegex) url_regexp = g_regex_new (URL_REGEXP,
                                                 G_REGEX_CASELESS | G_REGEX_OPTIMIZE,
                                                 G_REGEX_MATCH_NOTEMPTY,
                                                 NULL); /* error */
    vte_terminal_match_add_gregex (term,
                                   url_regexp,
                                   0);

    /* Apply user settings */
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

    if (G_LIKELY (command == NULL))
    {
        g_autofree gchar *setting = get_setting (settings, STARTUP_COMMAND_KEY);
        command = g_strsplit (setting , " ", 0);
    }

    /* Override TERM */
    g_auto (GStrv) envp = g_environ_setenv (NULL, "TERM", get_setting (settings, TERM_KEY), TRUE);

    /* Spawn our command */
    if (!vte_terminal_spawn_sync (term,
                                  VTE_PTY_DEFAULT,
                                  cwd,
                                  command,
                                  envp,
                                  G_SPAWN_SEARCH_PATH,
                                  NULL, /* child_setup */
                                  NULL, /* child_setup_data */
                                  NULL, /* child_pid */
                                  NULL, /* cancellable */
                                  &error))
    {
        g_critical ("%s", error->message);
        return 1;
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
    CONNECT_SIGNAL (terminal, "child-exited",       on_child_exited, NULL);
    CONNECT_SIGNAL (window,   "destroy",            germinal_exit,   NULL);
    CONNECT_SIGNAL (window,   "key-press-event",    on_key_press,    terminal);

    /* Launch program */
    gtk_main ();

    /* Free memory */
    g_free (get_url (NULL, NULL));
    g_free (update_colors (NULL, NULL, NULL));

    return 0;
}
