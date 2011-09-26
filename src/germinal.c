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

static void
germinal_exit (GtkWidget *widget, void *data)
{
    widget = widget;
    data = data;
    gtk_main_quit ();
}

static gboolean
run_tmux (GtkWidget *terminal, gboolean attach)
{
    gchar *tmux_attach_argv[] = { "tmux", "-u2", attach ? "a" : NULL, NULL };
    gchar *cwd = g_get_current_dir ();
    gboolean ret = vte_terminal_fork_command_full (VTE_TERMINAL (terminal), VTE_PTY_DEFAULT, cwd, tmux_attach_argv, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, NULL, NULL);
    g_free (cwd);
    return ret;

}

static const GdkColor xterm_palette[16] =
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

static gboolean
on_key_press (GtkWidget *widget, GdkEventKey *event, gpointer user_data)
{
    if (event->type != GDK_KEY_PRESS)
        return FALSE;

    VteTerminal *terminal = VTE_TERMINAL (user_data);

    if ((event->state & GDK_CONTROL_MASK) && (event->state & GDK_SHIFT_MASK)) {
        switch (event->keyval) {
        case GDK_KEY_C:
            vte_terminal_copy_clipboard (terminal);
            return TRUE;
        case GDK_KEY_V:
            vte_terminal_paste_clipboard (terminal);
            return TRUE;
        }
    }

    widget = widget;

    return FALSE;
}

static gboolean
on_button_press (GtkWidget *widget, GdkEventButton *button_event, gpointer user_data)
{
    if (button_event->type != GDK_BUTTON_PRESS)
        return FALSE;

    VteTerminal *terminal = VTE_TERMINAL (widget);
    gint tag;
    glong column = (glong)button_event->x / vte_terminal_get_char_width (VTE_TERMINAL (terminal));
    glong row = (glong)button_event->y / vte_terminal_get_char_height (VTE_TERMINAL (terminal));
    gchar *url = vte_terminal_match_check (VTE_TERMINAL (terminal), column, row, &tag);

    if (button_event->button == 1 && (button_event->state & GDK_CONTROL_MASK) && (button_event->state & GDK_SHIFT_MASK) && url) {
        GError *error = NULL;
        gchar *cmd;
        gchar *browser = (gchar *)g_getenv("BROWSER");

        if (browser)
            cmd = g_strdup_printf("%s %s", browser, url);
        else {
            if ((browser = g_find_program_in_path("xdg-open"))) {
                cmd = g_strdup_printf ("%s %s", browser, url);
                g_free (browser);
            } else
                cmd = g_strdup_printf("firefox %s", url);
        }

        if (!g_spawn_command_line_async (cmd, &error))
            fprintf (stderr, _("Couldn't exec \"%s\": %s"), cmd, error->message);

        g_free (cmd);

        return TRUE;
    }

    user_data = user_data;

    return FALSE;
}

int
main(int argc, char *argv[])
{
    textdomain(GETTEXT_PACKAGE);
    bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);
    bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
    gtk_init (&argc, &argv);
    GtkWidget *window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    GtkWidget *terminal = vte_terminal_new ();
    GRegex *http_regexp = g_regex_new ("(ftp|http)s?://[-a-zA-Z0-9.?$%&/=_~#.,:;+]*", G_REGEX_CASELESS, G_REGEX_MATCH_NOTEMPTY, NULL);
    GSettings *settings = g_settings_new ("org.gnome.Germinal");
    PangoFontDescription *font = pango_font_description_from_string (g_settings_get_string (settings, "font"));
    GdkColor forecolor, backcolor;
    gdk_color_parse (g_settings_get_string (settings, "forecolor"), &forecolor);
    gdk_color_parse (g_settings_get_string (settings, "backcolor"), &backcolor);
    vte_terminal_match_add_gregex (VTE_TERMINAL (terminal), http_regexp, 0);
    vte_terminal_set_mouse_autohide (VTE_TERMINAL (terminal), TRUE);
    vte_terminal_set_font (VTE_TERMINAL (terminal), font);
    vte_terminal_set_colors (VTE_TERMINAL(terminal), &forecolor, &backcolor, xterm_palette, 16);
    gtk_window_set_decorated (GTK_WINDOW (window), FALSE);
    gtk_window_maximize (GTK_WINDOW (window));
    gtk_widget_grab_focus(terminal);
    gtk_container_add (GTK_CONTAINER (window), terminal);
    if (!run_tmux (terminal, TRUE))
        run_tmux (terminal, FALSE);
    gtk_widget_show_all (window);
    g_signal_connect (G_OBJECT (window), "destroy", G_CALLBACK (germinal_exit), NULL);
    g_signal_connect (G_OBJECT (window), "key-press-event", G_CALLBACK (on_key_press), terminal);
    g_signal_connect (G_OBJECT (terminal), "button-press-event", G_CALLBACK (on_button_press), NULL);
    gtk_main ();
    pango_font_description_free (font);
    g_object_unref (settings);
    return 0;
}
