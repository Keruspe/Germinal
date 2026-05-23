// SPDX-FileCopyrightText: 2026 Marc-Antoine Perennou <Marc-Antoine@Perennou.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#define G_SETTINGS_ENABLE_BACKEND 1
#include "germinal-palette-editor.h"
#include "germinal-settings.h"

#include <gio/gsettingsbackend.h>

static GSettings *
make_settings (void)
{
    g_autoptr (GSettingsBackend) backend = g_memory_settings_backend_new ();
    return g_settings_new_with_backend ("org.gnome.Germinal", backend);
}

/* Sink the floating reference so g_object_unref / g_autoptr work correctly */
static GerminalPaletteEditor *
make_editor (GSettings *settings)
{
    return GERMINAL_PALETTE_EDITOR (g_object_ref_sink (germinal_palette_editor_new (settings)));
}

static void
test_create (void)
{
    g_autoptr (GSettings) settings = make_settings ();
    g_autoptr (GerminalPaletteEditor) editor = make_editor (settings);
    g_assert_nonnull (editor);
    g_assert_true (GERMINAL_IS_PALETTE_EDITOR (editor));
}

static void
test_load_from_settings (void)
{
    g_autoptr (GSettings) settings = make_settings ();

    const gchar * const colors[] = {
        "#ff0000", "#00ff00", "#0000ff", "#ffff00",
        "#00ffff", "#ff00ff", "#ffffff", "#000000",
        "#800000", "#008000", "#000080", "#808000",
        "#008080", "#800080", "#808080", "#c0c0c0",
        NULL
    };
    g_settings_set_strv (settings, PALETTE_KEY, colors);

    g_autoptr (GerminalPaletteEditor) editor = make_editor (settings);

    /* First color: #ff0000 → red */
    const GdkRGBA *rgba = germinal_palette_editor_get_rgba (editor, 0);
    g_assert_nonnull (rgba);
    g_assert_cmpfloat_with_epsilon (rgba->red,   1.0, 1e-3);
    g_assert_cmpfloat_with_epsilon (rgba->green, 0.0, 1e-3);
    g_assert_cmpfloat_with_epsilon (rgba->blue,  0.0, 1e-3);

    /* Second color: #00ff00 → green */
    rgba = germinal_palette_editor_get_rgba (editor, 1);
    g_assert_nonnull (rgba);
    g_assert_cmpfloat_with_epsilon (rgba->red,   0.0, 1e-3);
    g_assert_cmpfloat_with_epsilon (rgba->green, 1.0, 1e-3);
    g_assert_cmpfloat_with_epsilon (rgba->blue,  0.0, 1e-3);
}

static void
test_save_to_settings (void)
{
    g_autoptr (GSettings) settings = make_settings ();
    g_autoptr (GerminalPaletteEditor) editor = make_editor (settings);

    GdkRGBA blue = { 0.0, 0.0, 1.0, 1.0 };
    germinal_palette_editor_set_rgba (editor, 3, &blue);

    gsize size = 0;
    g_autofree GdkRGBA *palette = germinal_settings_get_palette (settings, &size);
    g_assert_cmpuint (size, ==, 16);
    g_assert_cmpfloat_with_epsilon (palette[3].red,   0.0, 1e-3);
    g_assert_cmpfloat_with_epsilon (palette[3].green, 0.0, 1e-3);
    g_assert_cmpfloat_with_epsilon (palette[3].blue,  1.0, 1e-3);
}

static void
test_settings_change_updates_editor (void)
{
    g_autoptr (GSettings) settings = make_settings ();
    g_autoptr (GerminalPaletteEditor) editor = make_editor (settings);

    const gchar * const new_colors[] = {
        "#ff0000", "#ff0000", "#ff0000", "#ff0000",
        "#ff0000", "#ff0000", "#ff0000", "#ff0000",
        "#ff0000", "#ff0000", "#ff0000", "#ff0000",
        "#ff0000", "#ff0000", "#ff0000", "#ff0000",
        NULL
    };
    g_settings_set_strv (settings, PALETTE_KEY, new_colors);

    const GdkRGBA *rgba = germinal_palette_editor_get_rgba (editor, 5);
    g_assert_nonnull (rgba);
    g_assert_cmpfloat_with_epsilon (rgba->red,   1.0, 1e-3);
    g_assert_cmpfloat_with_epsilon (rgba->green, 0.0, 1e-3);
    g_assert_cmpfloat_with_epsilon (rgba->blue,  0.0, 1e-3);
}

static void
test_roundtrip (void)
{
    g_autoptr (GSettings) settings = make_settings ();
    g_autoptr (GerminalPaletteEditor) editor = make_editor (settings);

    for (guint i = 0; i < 16; ++i)
    {
        GdkRGBA color = { (gdouble) i / 16.0, 0.0, 0.0, 1.0 };
        germinal_palette_editor_set_rgba (editor, i, &color);
    }

    gsize size = 0;
    g_autofree GdkRGBA *palette = germinal_settings_get_palette (settings, &size);
    g_assert_cmpuint (size, ==, 16);

    /* gdk_rgba_to_string quantises to 8-bit integers; max rounding error is 0.5/255 ≈ 0.002 */
    for (guint i = 0; i < 16; ++i)
        g_assert_cmpfloat_with_epsilon (palette[i].red, (gdouble) i / 16.0, 3e-3);
}

static void
test_no_display (void)
{
    g_test_skip ("no display available");
}

gint
main (gint argc, gchar *argv[])
{
    g_test_init (&argc, &argv, NULL);

    if (!g_getenv ("DISPLAY") && !g_getenv ("WAYLAND_DISPLAY"))
    {
        g_test_add_func ("/palette-editor/no-display", test_no_display);
        return g_test_run ();
    }

    gtk_init ();

    g_test_add_func ("/palette-editor/create",                  test_create);
    g_test_add_func ("/palette-editor/load-from-settings",      test_load_from_settings);
    g_test_add_func ("/palette-editor/save-to-settings",        test_save_to_settings);
    g_test_add_func ("/palette-editor/settings-change-updates", test_settings_change_updates_editor);
    g_test_add_func ("/palette-editor/roundtrip",               test_roundtrip);

    return g_test_run ();
}
