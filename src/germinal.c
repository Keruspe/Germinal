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
#define CHARACTER "[a-zA-Z]"
#define NOT_BLANK "[^ \t\n\r]"
#define NOT_BLANK_NOT_QUOTE "[^ \t\n\r\"\']"
#define URL_REGEXP CHARACTER "+://" NOT_BLANK "+" NOT_BLANK_NOT_QUOTE

#define SCROLLBACK_KEY "scrollback-lines"
#define WORD_CHARS_KEY "word-chars"
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
    /* Silence stupid warning */
    widget = widget;
    user_data = user_data;
}

static gboolean
do_copy (GtkWidget *widget,
         gpointer   user_data)
{
    vte_terminal_copy_clipboard (VTE_TERMINAL (user_data));
    /* Silence stupid warning */
    widget = widget;
    return TRUE;
}

static gboolean
do_paste (GtkWidget *widget,
          gpointer   user_data)
{
    vte_terminal_paste_clipboard (VTE_TERMINAL (user_data));
    /* Silence stupid warning */
    widget = widget;
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

    GError *error = NULL;
    /* Always strdup because we free later */
    gchar *browser = g_strdup (g_getenv ("BROWSER"));

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
    {
        fprintf (stderr, _("Couldn't exec \"%s %s\": %s"), browser, url, error->message);
        g_error_free (error);
    }
    g_free (browser);
    return TRUE;
}

static gboolean
do_copy_url (GtkWidget *widget,
             gpointer   user_data)
{
    gchar *url = get_url (VTE_TERMINAL (user_data), NULL);
    if (!url)
        return FALSE;
    GtkClipboard *clip = gtk_clipboard_get (GDK_SELECTION_CLIPBOARD);
    gtk_clipboard_set_text (clip, url, -1);
    clip = gtk_clipboard_get (GDK_SELECTION_PRIMARY);
    gtk_clipboard_set_text (clip, url, -1);
    /* Silence stupid warning */
    widget = widget;
    return TRUE;
}

static gboolean
do_open_url (GtkWidget *widget,
             gpointer   user_data)
{
    /* Silence stupid warning */
    widget = widget;
    return open_url (get_url (VTE_TERMINAL (user_data), NULL));
}

static gboolean
on_key_press (GtkWidget   *widget,
              GdkEventKey *event,
              gpointer     user_data)
{
    if (event->type != GDK_KEY_PRESS)
        return FALSE;

    /* Ctrl + Shift + foo */
    if ((event->state & GDK_CONTROL_MASK) &&
        (event->state & GDK_SHIFT_MASK))
    {
        switch (event->keyval) {
        case GDK_KEY_C:
            do_copy (widget, user_data);
            return TRUE;
        case GDK_KEY_V:
            do_paste (widget, user_data);
            return TRUE;
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
    /* helper to manage memory */
    static gchar *dest = NULL;
    g_free (dest);
    if (name != NULL)
        dest = g_settings_get_string (settings, name);
    else
        dest = NULL;
    return dest;
}

static void
update_scrollback (GSettings   *settings,
                   const gchar *key,
                   gpointer     user_data)
{
    vte_terminal_set_scrollback_lines (VTE_TERMINAL (user_data), g_settings_get_int (settings, key));
}

static void
update_word_chars (GSettings   *settings,
                   const gchar *key,
                   gpointer     user_data)
{
    vte_terminal_set_word_chars (VTE_TERMINAL (user_data), get_setting (settings, key));
}

static void
update_font (GSettings   *settings,
             const gchar *key,
             gpointer     user_data)
{
    vte_terminal_set_font_from_string (VTE_TERMINAL (user_data), get_setting (settings, key));
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
    /* Options */
    gchar **command = NULL;
    GOptionEntry options[] =
    {
        { G_OPTION_REMAINING, 'c', 0, G_OPTION_ARG_STRING_ARRAY, &command, N_("the command to launch"), "command" },
        { NULL, '\0', 0, G_OPTION_ARG_NONE, NULL, NULL, NULL }
    };

    /* Gettext and gtk initialization */
    textdomain(GETTEXT_PACKAGE);
    bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);
    bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
    gtk_init_with_args (&argc,
                        &argv,
                         N_(" - minimalist vte-based terminal emulator"),
                         options,
                         GETTEXT_PACKAGE,
                         NULL); /* error */

    GtkWidget *window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    GtkWidget *terminal = vte_terminal_new ();
    GtkWidget *menu = gtk_menu_new ();
    GSettings *settings = g_settings_new ("org.gnome.Germinal");

    /* Url matching stuff */
    GRegex *url_regexp = g_regex_new (URL_REGEXP,
                                      G_REGEX_CASELESS | G_REGEX_OPTIMIZE,
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
    vte_terminal_set_audible_bell (VTE_TERMINAL (terminal), FALSE);
    vte_terminal_set_visible_bell (VTE_TERMINAL (terminal), FALSE);
    vte_terminal_set_scroll_on_output (VTE_TERMINAL (terminal), FALSE);
    vte_terminal_set_scroll_on_keystroke (VTE_TERMINAL (terminal), TRUE);
    update_scrollback (settings, SCROLLBACK_KEY, terminal);
    g_signal_connect (G_OBJECT (settings),
                      "changed::" SCROLLBACK_KEY,
                      G_CALLBACK (update_scrollback),
                      terminal);
    update_word_chars (settings, WORD_CHARS_KEY, terminal);
    g_signal_connect (G_OBJECT (settings),
                      "changed::" WORD_CHARS_KEY,
                      G_CALLBACK (update_word_chars),
                      terminal);
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
                                    (command == NULL) ? tmux_argv : command,
                                    NULL, /* env */
                                    G_SPAWN_SEARCH_PATH,
                                    NULL, /* child setup */
                                    NULL, /* child setup data */
                                    NULL, /* child pid */
                                    NULL); /* error */
    g_strfreev (command);
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
    GtkAction *copy_url_action = gtk_action_new ("copy_url",
                                                 _("Copy _url"),
                                                 NULL, /* tooltip */
                                                 GTK_STOCK_COPY);
    GtkWidget *copy_url_menu_item = gtk_action_create_menu_item (copy_url_action);
    g_signal_connect (G_OBJECT (copy_url_action),
                      "activate",
                      G_CALLBACK (do_copy_url),
                      terminal);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), copy_url_menu_item);
    GtkAction *open_url_action = gtk_action_new ("open_url",
                                                 _("_Open url"),
                                                 NULL, /* tooltip */
                                                 GTK_STOCK_COPY);
    GtkWidget *open_url_menu_item = gtk_action_create_menu_item (open_url_action);
    g_signal_connect (G_OBJECT (open_url_action),
                      "activate",
                      G_CALLBACK (do_open_url),
                      terminal);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), open_url_menu_item);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), gtk_separator_menu_item_new ());
    GtkAction *copy_action = gtk_action_new ("copy",
                                             _("_Copy"),
                                             NULL, /* tooltip */
                                             GTK_STOCK_COPY);
    GtkWidget *copy_menu_item = gtk_action_create_menu_item (copy_action);
    g_signal_connect (G_OBJECT (copy_action),
                      "activate",
                      G_CALLBACK (do_copy),
                      terminal);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), copy_menu_item);
    GtkAction *paste_action = gtk_action_new ("paste",
                                              _("_Paste"),
                                              NULL, /* tooltip */
                                              GTK_STOCK_PASTE);
    g_signal_connect (G_OBJECT (paste_action),
                      "activate",
                      G_CALLBACK (do_paste),
                      terminal);
    GtkWidget *paste_menu_item = gtk_action_create_menu_item (paste_action);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), paste_menu_item);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), gtk_separator_menu_item_new ());
    GtkWidget *im_submenu = gtk_menu_new ();
    vte_terminal_im_append_menuitems (VTE_TERMINAL (terminal), GTK_MENU_SHELL (im_submenu));
    GtkWidget *im_menu_item = gtk_menu_item_new_with_mnemonic (_("Input _Methods"));
    gtk_menu_item_set_submenu (GTK_MENU_ITEM (im_menu_item), im_submenu);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), im_menu_item);
    gtk_widget_show_all (menu);

    /* Launch program */
    gtk_main ();

    /* Free memory */
    g_free (get_url (NULL, NULL));
    get_setting (settings, NULL); /* Free buffer */
    g_object_unref (settings);
    return 0;
}
