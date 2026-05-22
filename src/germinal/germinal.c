// SPDX-FileCopyrightText: 2011-2026 Marc-Antoine Perennou <Marc-Antoine@Perennou.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "germinal-window.h"

#include <stdlib.h>

static gint
germinal_create_window (GApplication *application,
                        GStrv         command)
{
    adw_style_manager_set_color_scheme (adw_style_manager_get_default (), ADW_COLOR_SCHEME_PREFER_DARK);

    GerminalTerminal *terminal = GERMINAL_TERMINAL (germinal_terminal_new ());
    GerminalWindow *window = GERMINAL_WINDOW (germinal_window_new (GTK_APPLICATION (application), terminal));

    germinal_window_present (window);
    germinal_window_spawn_command (window, command);

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

    /* AdwApplication initialization */
    AdwApplication *app = adw_application_new ("org.gnome.Germinal", G_APPLICATION_HANDLES_COMMAND_LINE|G_APPLICATION_SEND_ENVIRONMENT);
    GApplication *gapp = G_APPLICATION (app);
    GApplicationClass *klass = G_APPLICATION_GET_CLASS (gapp);

    g_application_add_main_option (gapp, "version",          'v', 0, G_OPTION_ARG_NONE,         N_("display the version"),   NULL);
    g_application_add_main_option (gapp, G_OPTION_REMAINING, 'e', 0, G_OPTION_ARG_STRING_ARRAY, N_("the command to launch"), "command");

    klass->command_line = germinal_command_line;
    klass->activate = germinal_activate;

    /* Launch program */
    return g_application_run (gapp, argc, argv);
}
