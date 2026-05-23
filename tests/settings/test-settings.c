// SPDX-FileCopyrightText: 2026 Marc-Antoine Perennou <Marc-Antoine@Perennou.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#define G_SETTINGS_ENABLE_BACKEND 1
#include "germinal-settings.h"

#include <gio/gsettingsbackend.h>

static GSettings *
make_settings (void)
{
    g_autoptr (GSettingsBackend) backend = g_memory_settings_backend_new ();
    return g_settings_new_with_backend ("org.gnome.Germinal", backend);
}

static void
set_palette (GSettings *settings, guint n)
{
    if (n == 0)
    {
        g_settings_set_strv (settings, PALETTE_KEY, NULL);
        return;
    }
    g_auto (GStrv) strv = g_new0 (gchar *, n + 1);
    for (guint i = 0; i < n; ++i)
        strv[i] = g_strdup ("#000000");
    g_settings_set_strv (settings, PALETTE_KEY, (const gchar * const *) strv);
}

static void
test_palette_valid_empty (void)
{
    g_autoptr (GSettings) settings = make_settings ();
    set_palette (settings, 0);
    gsize size = 42;
    g_autofree GdkRGBA *palette = germinal_settings_get_palette (settings, &size);
    g_assert_cmpuint (size, ==, 0);
    (void) palette;
}

static void
test_palette_valid_8 (void)
{
    g_autoptr (GSettings) settings = make_settings ();
    set_palette (settings, 8);
    gsize size = 0;
    g_autofree GdkRGBA *palette = germinal_settings_get_palette (settings, &size);
    g_assert_cmpuint (size, ==, 8);
    g_assert_nonnull (palette);
}

static void
test_palette_valid_16 (void)
{
    g_autoptr (GSettings) settings = make_settings ();
    set_palette (settings, 16);
    gsize size = 0;
    g_autofree GdkRGBA *palette = germinal_settings_get_palette (settings, &size);
    g_assert_cmpuint (size, ==, 16);
    g_assert_nonnull (palette);
}

static void
test_palette_valid_24 (void)
{
    g_autoptr (GSettings) settings = make_settings ();
    set_palette (settings, 24);
    gsize size = 0;
    g_autofree GdkRGBA *palette = germinal_settings_get_palette (settings, &size);
    g_assert_cmpuint (size, ==, 24);
    g_assert_nonnull (palette);
}

static void
test_palette_valid_25 (void)
{
    g_autoptr (GSettings) settings = make_settings ();
    set_palette (settings, 25);
    gsize size = 0;
    g_autofree GdkRGBA *palette = germinal_settings_get_palette (settings, &size);
    g_assert_cmpuint (size, ==, 25);
    g_assert_nonnull (palette);
}

static void
test_palette_valid_255 (void)
{
    g_autoptr (GSettings) settings = make_settings ();
    set_palette (settings, 255);
    gsize size = 0;
    g_autofree GdkRGBA *palette = germinal_settings_get_palette (settings, &size);
    g_assert_cmpuint (size, ==, 255);
    g_assert_nonnull (palette);
}

static void
test_palette_invalid_resets (void)
{
    const guint invalid_sizes[] = { 1, 7, 9, 15, 17, 23 };
    for (guint i = 0; i < G_N_ELEMENTS (invalid_sizes); ++i)
    {
        g_autoptr (GSettings) settings = make_settings ();
        set_palette (settings, invalid_sizes[i]);
        gsize size = 0;
        g_autofree GdkRGBA *palette = germinal_settings_get_palette (settings, &size);
        g_assert_cmpuint (size, ==, 16);
        g_assert_nonnull (palette);
    }
}

static void
test_palette_color_parsing (void)
{
    g_autoptr (GSettings) settings = make_settings ();

    const gchar * const colors[] = {
        "#ff0000", "#00ff00", "#0000ff", "#ffffff",
        "#000000", "#808080", "#c0c0c0", "#ffff00",
        NULL
    };
    g_settings_set_strv (settings, PALETTE_KEY, colors);

    gsize size = 0;
    g_autofree GdkRGBA *palette = germinal_settings_get_palette (settings, &size);
    g_assert_cmpuint (size, ==, 8);
    g_assert_nonnull (palette);

    /* #ff0000 → red=1.0, green=0.0, blue=0.0 */
    g_assert_cmpfloat_with_epsilon (palette[0].red,   1.0, 1e-3);
    g_assert_cmpfloat_with_epsilon (palette[0].green, 0.0, 1e-3);
    g_assert_cmpfloat_with_epsilon (palette[0].blue,  0.0, 1e-3);

    /* #00ff00 → red=0.0, green=1.0, blue=0.0 */
    g_assert_cmpfloat_with_epsilon (palette[1].red,   0.0, 1e-3);
    g_assert_cmpfloat_with_epsilon (palette[1].green, 1.0, 1e-3);
    g_assert_cmpfloat_with_epsilon (palette[1].blue,  0.0, 1e-3);

    /* #0000ff → red=0.0, green=0.0, blue=1.0 */
    g_assert_cmpfloat_with_epsilon (palette[2].red,   0.0, 1e-3);
    g_assert_cmpfloat_with_epsilon (palette[2].green, 0.0, 1e-3);
    g_assert_cmpfloat_with_epsilon (palette[2].blue,  1.0, 1e-3);
}

gint
main (gint argc, gchar *argv[])
{
    g_test_init (&argc, &argv, NULL);

    g_test_add_func ("/palette/valid/empty",   test_palette_valid_empty);
    g_test_add_func ("/palette/valid/8",       test_palette_valid_8);
    g_test_add_func ("/palette/valid/16",      test_palette_valid_16);
    g_test_add_func ("/palette/valid/24",      test_palette_valid_24);
    g_test_add_func ("/palette/valid/25",      test_palette_valid_25);
    g_test_add_func ("/palette/valid/255",     test_palette_valid_255);
    g_test_add_func ("/palette/invalid/resets",  test_palette_invalid_resets);
    g_test_add_func ("/palette/color-parsing", test_palette_color_parsing);

    return g_test_run ();
}
