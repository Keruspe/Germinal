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
    vte_terminal_match_add_gregex (VTE_TERMINAL (terminal), http_regexp, 0);
    vte_terminal_set_mouse_autohide (VTE_TERMINAL (terminal), TRUE);
    gtk_window_set_decorated (GTK_WINDOW (window), FALSE);
    gtk_window_maximize (GTK_WINDOW (window));
    gtk_widget_grab_focus(terminal);
    gtk_container_add (GTK_CONTAINER (window), terminal);
    if (!run_tmux (terminal, TRUE))
        run_tmux (terminal, FALSE);
    gtk_widget_show_all (window);
    g_signal_connect (G_OBJECT (window), "destroy", G_CALLBACK (germinal_exit), NULL);
    gtk_main ();
    return 0;
}
