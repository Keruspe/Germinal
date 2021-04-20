#include "germinal-settings.h"

#define G_SETTINGS_ENABLE_BACKEND 1
#include <gio/gsettingsbackend.h>

GSettings *
germinal_settings_new (void)
{
    g_autofree gchar *config_file_path = g_build_filename (g_get_user_config_dir (), "germinal", "settings", NULL);
    g_autoptr (GFile) config_file = g_file_new_for_path (config_file_path);

    if (g_file_query_exists (config_file, NULL /* cancellable */))
    {
        g_autoptr (GSettingsBackend) backend = g_keyfile_settings_backend_new (config_file_path, "/org/gnome/Germinal/", "Germinal");

        return g_settings_new_with_backend ("org.gnome.Germinal", backend);
    }
    else
    {
        return g_settings_new ("org.gnome.Germinal");
    }
}
