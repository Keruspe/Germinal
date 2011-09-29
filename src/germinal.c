/*
 * This file is part of Germinal.
 *
 * Copyright 2011 Marc-Antoine Perennou <Marc-Antoine@Perennou.com>
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

#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>
#include <vte/vte.h>

#define PALETTE_SIZE 16
#define URL_REGEXP "(ftp|http)s?://[-a-zA-Z0-9.?$%&/=_~#.,:;+]*"

#define FONT_KEY "font"
#define FORECOLOR_KEY "forecolor"
#define BACKCOLOR_KEY "backcolor"

/* Stolen from sakura which stole it from gnome-terminal */
static const GdkColor xterm_palette[PALETTE_SIZE] =
{
    {0, 0x0000, 0x0000, 0x0000 },
    {0, 0xcdcb, 0x0000, 0x0000 },
    {0, 0x0000, 0xcdcb, 0x0000 },
    {0, 0xcdcb, 0xcdcb, 0x0000 },
    {0, 0x1e1a, 0x908f, 0xffff },
    {0, 0xcdcb, 0x0000, 0xcdcb },
    {0, 0x0000, 0xcdcb, 0xcdcb },
    {0, 0xe5e2, 0xe5e2, 0xe5e2 },
    {0, 0x4ccc, 0x4ccc, 0x4ccc },
    {0, 0xffff, 0x0000, 0x0000 },
    {0, 0x0000, 0xffff, 0x0000 },
    {0, 0xffff, 0xffff, 0x0000 },
    {0, 0x4645, 0x8281, 0xb4ae },
    {0, 0xffff, 0x0000, 0xffff },
    {0, 0x0000, 0xffff, 0xffff },
    {0, 0xffff, 0xffff, 0xffff }
};

static void
germinal_exit (GtkWidget *widget,
               gpointer   user_data)
{
    gtk_main_quit ();

    /* Silence stupid warnings */
    widget = widget;
    user_data = user_data;
}

static gboolean
on_key_press (GtkWidget   *widget,
              GdkEventKey *event,
              gpointer     user_data)
{
    if (event->type != GDK_KEY_PRESS)
        return FALSE;

    VteTerminal *terminal = VTE_TERMINAL (user_data);

    /* Ctrl + Shift + foo */
    if ((event->state & GDK_CONTROL_MASK) &&
        (event->state & GDK_SHIFT_MASK))
    {
        switch (event->keyval) {
        case GDK_KEY_C:
            vte_terminal_copy_clipboard (terminal);
            return TRUE;
        case GDK_KEY_V:
            vte_terminal_paste_clipboard (terminal);
            return TRUE;
        }
    }

    /* Silence stupid warning */
    widget = widget;

    return FALSE;
}

static gboolean
on_button_press (GtkWidget      *widget,
                 GdkEventButton *button_event,
                 gpointer        user_data)
{
    if (button_event->type != GDK_BUTTON_PRESS)
        return FALSE;

    VteTerminal *terminal = VTE_TERMINAL (widget);

    glong column = (glong)button_event->x / vte_terminal_get_char_width (terminal);
    glong row = (glong)button_event->y / vte_terminal_get_char_height (terminal);
    gint tag; /* avoid stupid vte segv (said to be optional) */
    gchar *url = vte_terminal_match_check (terminal,
                                           column,
                                           row,
                                           &tag);

    /* Shift + Left clic */
    if ((button_event->button == 1) &&
        (button_event->state & GDK_SHIFT_MASK) &&
        (url))
    {
        GError *error = NULL;
        gchar *cmd;
        /* Always strdup because we free later */
        gchar *browser = g_strdup (g_getenv ("BROWSER"));

        /* If BROWSER is not in env, try xdg-open or fallback to firefox */
        if (!browser && !(browser = g_find_program_in_path ("xdg-open")))
            browser = g_strdup ("firefox");

        cmd = g_strdup_printf ("%s %s", browser, url);
        g_free (browser);
        g_free (url);

        if (!g_spawn_command_line_async (cmd, &error))
        {
            fprintf (stderr, _("Couldn't exec \"%s\": %s"), cmd, error->message);
            g_error_free (error);
        }

        g_free (cmd);

        return TRUE;
    }
    else if (button_event->button == 3)
    {
        gtk_menu_popup (GTK_MENU (user_data), NULL, NULL, NULL, NULL, button_event->button, button_event->time);
        return TRUE;
    }

    return FALSE;
}

static gchar *
get_setting (GSettings   *settings,
             const gchar *name)
{
    static gchar *dest = NULL;
    g_free (dest);
    if (name != NULL)
        dest = g_settings_get_string (settings, name);
    else
        dest = NULL;
    return dest;
}

static void
update_font (GSettings   *settings,
             const gchar *key,
             gpointer     user_data)
{
    PangoFontDescription *font = pango_font_description_from_string (get_setting (settings, key));
    vte_terminal_set_font (VTE_TERMINAL (user_data), font);
    pango_font_description_free (font);
}

static void
update_colors (GSettings   *settings,
               const gchar *key, /* NULL for initialization */
               gpointer     user_data)
{
    static GdkColor forecolor, backcolor;
    if (key == NULL)
    {
        gdk_color_parse (get_setting (settings, FORECOLOR_KEY), &forecolor);
        gdk_color_parse (get_setting (settings, BACKCOLOR_KEY), &backcolor);
    }
    else if (strcmp (key, FORECOLOR_KEY) == 0)
        gdk_color_parse (get_setting (settings, FORECOLOR_KEY), &forecolor);
    else if (strcmp (key, BACKCOLOR_KEY) == 0)
        gdk_color_parse (get_setting (settings, BACKCOLOR_KEY), &backcolor);
    vte_terminal_set_colors (VTE_TERMINAL (user_data),
                            &forecolor,
                            &backcolor,
                             xterm_palette,
                             PALETTE_SIZE);
}

int
main(int   argc,
     char *argv[])
{
    /* Gettext and gtk initialization */
    textdomain(GETTEXT_PACKAGE);
    bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);
    bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
    gtk_init (&argc, &argv);

    GtkWidget *window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    GtkWidget *terminal = vte_terminal_new ();
    GtkWidget *menu = gtk_menu_new ();
    GSettings *settings = g_settings_new ("org.gnome.Germinal");

    /* Url matching stuff */
    GRegex *url_regexp = g_regex_new (URL_REGEXP,
                                      G_REGEX_CASELESS,
                                      G_REGEX_MATCH_NOTEMPTY,
                                      NULL); /* error */
    vte_terminal_match_add_gregex (VTE_TERMINAL (terminal),
                                   url_regexp,
                                   0);
    g_regex_unref (url_regexp);

    /* Window settings */
    gtk_window_set_decorated (GTK_WINDOW (window), FALSE);
    gtk_window_maximize (GTK_WINDOW (window));
    gtk_container_add (GTK_CONTAINER (window), terminal);
    gtk_widget_grab_focus (terminal);
    gtk_widget_show_all (window);

    /* Vte settings */
    vte_terminal_set_mouse_autohide (VTE_TERMINAL (terminal), TRUE);
    update_font (settings, FONT_KEY, terminal);
    g_signal_connect (G_OBJECT (settings),
                      "changed::" FONT_KEY,
                      G_CALLBACK (update_font),
                      terminal);
    update_colors (settings, NULL, terminal);
    g_signal_connect (G_OBJECT (settings),
                      "changed::" FORECOLOR_KEY,
                      G_CALLBACK (update_colors),
                      terminal);
    g_signal_connect (G_OBJECT (settings),
                      "changed::" BACKCOLOR_KEY,
                      G_CALLBACK (update_colors),
                      terminal);

    /* Base command */
    gchar *tmux_argv[] = { "tmux", "-u2", "a", NULL };
    gchar *cwd = g_get_current_dir ();
    vte_terminal_fork_command_full (VTE_TERMINAL (terminal),
                                    VTE_PTY_DEFAULT,
                                    cwd,
                                    tmux_argv,
                                    NULL, /* env */
                                    G_SPAWN_SEARCH_PATH,
                                    NULL, /* child_setup */
                                    NULL, /* child_setup_data */
                                    NULL, /* child_pid */
                                    NULL); /* error */
    g_free (cwd);

    /* Bind signals */
    g_signal_connect (G_OBJECT (terminal),
                      "button-press-event",
                      G_CALLBACK (on_button_press),
                      menu);
    g_signal_connect (G_OBJECT (window),
                      "destroy",
                      G_CALLBACK (germinal_exit),
                      NULL);
    g_signal_connect (G_OBJECT (window),
                      "key-press-event",
                      G_CALLBACK (on_key_press),
                      terminal);
    g_signal_connect(G_OBJECT (terminal),
                     "child-exited",
                     G_CALLBACK (germinal_exit),
                     NULL);

    /* Populate right click menu */
    GtkWidget *im_submenu = gtk_menu_new ();
    vte_terminal_im_append_menuitems (VTE_TERMINAL (terminal), GTK_MENU_SHELL (im_submenu));
    GtkWidget *im_menu_item = gtk_menu_item_new_with_mnemonic (_("Input _Methods"));
    gtk_menu_item_set_submenu (GTK_MENU_ITEM (im_menu_item), im_submenu);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), im_menu_item);
    gtk_widget_show_all (menu);

    /* Launch program */
    gtk_main ();

    /* Free memory */
    get_setting (settings, NULL); /* Free buffer */
    g_object_unref (settings);
    return 0;
}
