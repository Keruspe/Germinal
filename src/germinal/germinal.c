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

typedef struct {
    GerminalWindow   *win;
    GerminalTerminal *term;
    GStrv             command;
} GerminalCommandData;

static gboolean
germinal_spawn_command (gpointer user_data)
{
    g_autofree GerminalCommandData *data = user_data;

    if (!gtk_widget_get_realized (GTK_WIDGET (data->win)) || !gtk_widget_get_realized (GTK_WIDGET (data->term)))
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
    GerminalTerminal *terminal = GERMINAL_TERMINAL (germinal_terminal_new ());
    GerminalWindow *window = GERMINAL_WINDOW (germinal_window_new (GTK_APPLICATION (application), terminal));

    germinal_window_present (window);

    GerminalCommandData *data = g_new0 (GerminalCommandData, 1);
    data->win = window;
    data->term = terminal;
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
