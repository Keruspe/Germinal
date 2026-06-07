// SPDX-FileCopyrightText: 2011-2026 Marc-Antoine Perennou <Marc-Antoine@Perennou.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "germinal-about.h"
#include "germinal-preferences.h"
#include "germinal-window.h"

#include <stdlib.h>

static gboolean
germinal_command_matches (GStrv        command,
                          const gchar *name)
{
    return command && command[0] && !command[1] && !g_strcmp0 (command[0], name);
}

/* Map a lone "preferences"/"settings"/"about" command to the application
 * action that presents the matching dialog, or NULL for a real command. */
static const gchar *
germinal_command_action (GStrv command)
{
    if (germinal_command_matches (command, "preferences") || germinal_command_matches (command, "settings"))
        return "preferences";
    if (germinal_command_matches (command, "about"))
        return "about";
    return NULL;
}

static void
germinal_present_dialog (GApplication *application,
                         AdwDialog    *dialog)
{
    GtkWindow *active = gtk_application_get_active_window (GTK_APPLICATION (application));

    if (active)
    {
        adw_dialog_present (dialog, GTK_WIDGET (active));
        return;
    }

    /* No window to host the dialog: present it as its own window and hold the
     * application alive until it is dismissed, so a bare "germinal preferences"
     * never flashes an empty terminal. */
    g_application_hold (application);
    g_signal_connect_swapped (dialog, "closed", G_CALLBACK (g_application_release), application);
    adw_dialog_present (dialog, NULL);
}

static void
action_preferences (GSimpleAction *action G_GNUC_UNUSED,
                    GVariant      *param  G_GNUC_UNUSED,
                    gpointer       user_data)
{
    germinal_present_dialog (user_data, germinal_preferences_new ());
}

static void
action_about (GSimpleAction *action G_GNUC_UNUSED,
              GVariant      *param  G_GNUC_UNUSED,
              gpointer       user_data)
{
    germinal_present_dialog (user_data, germinal_about_new ());
}

static void
germinal_create_window (GApplication *application,
                        GStrv         command)
{
    GerminalTerminal *terminal = GERMINAL_TERMINAL (germinal_terminal_new ());
    GerminalWindow *window = GERMINAL_WINDOW (germinal_window_new (GTK_APPLICATION (application), terminal));

    germinal_window_present (window);
    germinal_window_spawn_command (window, command);
}

static void
germinal_startup (GApplication *application,
                  gpointer      user_data G_GNUC_UNUSED)
{
    static const GActionEntry app_actions[] = {
        { .name = "preferences", .activate = action_preferences },
        { .name = "about",       .activate = action_about       },
    };

    g_action_map_add_action_entries (G_ACTION_MAP (application), app_actions, G_N_ELEMENTS (app_actions), application);

    adw_style_manager_set_color_scheme (adw_style_manager_get_default (), ADW_COLOR_SCHEME_PREFER_DARK);
}

static gint
germinal_command_line (GApplication            *application,
                       GApplicationCommandLine *command_line,
                       G_GNUC_UNUSED gpointer   user_data)
{
    GVariantDict *dict = g_application_command_line_get_options_dict (command_line);

    if (g_variant_dict_contains (dict, "version"))
    {
        g_application_command_line_print (command_line, PACKAGE_STRING "\n");
        return 0;
    }

    g_autoptr (GVariant) v = g_variant_dict_lookup_value (dict, G_OPTION_REMAINING, NULL);
    g_auto (GStrv) command = (v) ? g_variant_dup_strv (v, NULL) : NULL;

    const gchar *action = germinal_command_action (command);
    if (action)
    {
        g_action_group_activate_action (G_ACTION_GROUP (application), action, NULL);
        return EXIT_SUCCESS;
    }

    germinal_create_window (application, g_steal_pointer (&command));
    return EXIT_SUCCESS;
}

static void
germinal_activate (GApplication *application,
                   G_GNUC_UNUSED gpointer user_data)
{
    germinal_create_window (application, NULL);
}

gint
main (gint   argc,
      gchar *argv[])
{
    textdomain (GETTEXT_PACKAGE);
    bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
    bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");

    g_autoptr (AdwApplication) app = adw_application_new ("org.gnome.Germinal", G_APPLICATION_HANDLES_COMMAND_LINE | G_APPLICATION_SEND_ENVIRONMENT);
    GApplication *gapp = G_APPLICATION (app);

    g_application_add_main_option (gapp, "version",          'v', 0, G_OPTION_ARG_NONE,         N_("display the version"),   NULL);
    g_application_add_main_option (gapp, G_OPTION_REMAINING, 'e', 0, G_OPTION_ARG_STRING_ARRAY, N_("the command to launch"), "command");

    gulong startup_id   = g_signal_connect (gapp, "startup",      G_CALLBACK (germinal_startup),      NULL);
    gulong activate_id  = g_signal_connect (gapp, "activate",     G_CALLBACK (germinal_activate),     NULL);
    gulong cmd_line_id  = g_signal_connect (gapp, "command-line", G_CALLBACK (germinal_command_line), NULL);

    gint ret = g_application_run (gapp, argc, argv);

    g_signal_handler_disconnect (gapp, startup_id);
    g_signal_handler_disconnect (gapp, activate_id);
    g_signal_handler_disconnect (gapp, cmd_line_id);

    return ret;
}
