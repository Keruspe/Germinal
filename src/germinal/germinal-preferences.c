// SPDX-FileCopyrightText: 2026 Marc-Antoine Perennou <Marc-Antoine@Perennou.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "germinal-preferences.h"
#include "germinal-settings.h"

#include <glib/gi18n-lib.h>
#include <pango/pango.h>

/* --- Mapping helpers -------------------------------------------------- */

static gboolean
color_get_mapping (GValue   *value,
                   GVariant *variant,
                   gpointer  user_data G_GNUC_UNUSED)
{
    GdkRGBA rgba = { 0 };
    gdk_rgba_parse (&rgba, g_variant_get_string (variant, NULL));
    g_value_set_boxed (value, &rgba);
    return TRUE;
}

static GVariant *
color_set_mapping (const GValue       *value,
                   const GVariantType *expected_type G_GNUC_UNUSED,
                   gpointer            user_data G_GNUC_UNUSED)
{
    const GdkRGBA *rgba = g_value_get_boxed (value);
    if (!rgba)
        return g_variant_new_string ("#000000");
    g_autofree gchar *str = gdk_rgba_to_string (rgba);
    return g_variant_new_string (str);
}

static gboolean
font_get_mapping (GValue   *value,
                  GVariant *variant,
                  gpointer  user_data G_GNUC_UNUSED)
{
    PangoFontDescription *desc = pango_font_description_from_string (g_variant_get_string (variant, NULL));
    g_value_set_boxed (value, desc);
    pango_font_description_free (desc);
    return TRUE;
}

static GVariant *
font_set_mapping (const GValue       *value,
                  const GVariantType *expected_type G_GNUC_UNUSED,
                  gpointer            user_data G_GNUC_UNUSED)
{
    const PangoFontDescription *desc = g_value_get_boxed (value);
    if (!desc)
        return g_variant_new_string ("");
    g_autofree gchar *str = pango_font_description_to_string (desc);
    return g_variant_new_string (str);
}

static gboolean
int_to_double (GValue   *value,
               GVariant *variant,
               gpointer  user_data G_GNUC_UNUSED)
{
    g_value_set_double (value, (gdouble) g_variant_get_int32 (variant));
    return TRUE;
}

static GVariant *
double_to_int (const GValue       *value,
               const GVariantType *expected_type G_GNUC_UNUSED,
               gpointer            user_data G_GNUC_UNUSED)
{
    return g_variant_new_int32 ((gint) g_value_get_double (value));
}

/* --- Reset button ------------------------------------------------------ */

static void
update_reset_sensitivity (GSettings   *settings,
                          const gchar *key,
                          gpointer     user_data)
{
    g_autoptr (GVariant) user_value = g_settings_get_user_value (settings, key);
    gtk_widget_set_sensitive (GTK_WIDGET (user_data), user_value != NULL);
}

static void
on_reset_clicked (GtkButton *button,
                  gpointer   user_data G_GNUC_UNUSED)
{
    GSettings   *settings = g_object_get_data (G_OBJECT (button), "germinal-settings");
    const gchar *key      = g_object_get_data (G_OBJECT (button), "germinal-key");
    g_settings_reset (settings, key);
}

static GtkWidget *
make_reset_button (GSettings   *settings,
                   const gchar *key)
{
    GtkWidget *button = gtk_button_new_from_icon_name ("edit-undo-symbolic");
    gtk_widget_set_valign (button, GTK_ALIGN_CENTER);
    gtk_widget_set_tooltip_text (button, _("Reset to default"));
    gtk_widget_add_css_class (button, "flat");
    gtk_widget_add_css_class (button, "circular");

    g_object_set_data_full (G_OBJECT (button), "germinal-settings",
                            g_object_ref (settings), g_object_unref);
    g_object_set_data (G_OBJECT (button), "germinal-key", (gpointer) key);

    g_autoptr (GVariant) user_value = g_settings_get_user_value (settings, key);
    gtk_widget_set_sensitive (button, user_value != NULL);

    g_signal_connect (button, "clicked", G_CALLBACK (on_reset_clicked), NULL);

    g_autofree gchar *detail = g_strconcat ("changed::", key, NULL);
    g_signal_connect_object (settings, detail,
                             G_CALLBACK (update_reset_sensitivity), button, 0);

    return button;
}

/* --- Dialog factory ---------------------------------------------------- */

AdwDialog *
germinal_preferences_new (void)
{
    g_autoptr (GSettings) settings = germinal_settings_new ();

    AdwPreferencesDialog *dialog = ADW_PREFERENCES_DIALOG (adw_preferences_dialog_new ());

    /* --- Appearance page -------------------------------------------------- */
    AdwPreferencesPage *appearance = ADW_PREFERENCES_PAGE (adw_preferences_page_new ());
    adw_preferences_page_set_title (appearance, _("Appearance"));
    adw_preferences_page_set_icon_name (appearance, "preferences-desktop-appearance-symbolic");

    /* Font group */
    AdwPreferencesGroup *font_group = ADW_PREFERENCES_GROUP (adw_preferences_group_new ());
    adw_preferences_group_set_title (font_group, _("Font"));

    GtkFontDialog *font_dialog = gtk_font_dialog_new ();
    GtkWidget *font_button = gtk_font_dialog_button_new (font_dialog);
    AdwActionRow *font_row = ADW_ACTION_ROW (adw_action_row_new ());
    adw_preferences_row_set_title (ADW_PREFERENCES_ROW (font_row), _("Font"));
    adw_action_row_add_suffix (font_row, font_button);
    adw_action_row_add_suffix (font_row, make_reset_button (settings, FONT_KEY));
    adw_action_row_set_activatable_widget (font_row, font_button);
    g_settings_bind_with_mapping (settings, FONT_KEY, font_button, "font-desc",
                                  G_SETTINGS_BIND_DEFAULT,
                                  font_get_mapping, font_set_mapping, NULL, NULL);
    adw_preferences_group_add (font_group, GTK_WIDGET (font_row));
    adw_preferences_page_add (appearance, font_group);

    /* Colors group */
    AdwPreferencesGroup *colors_group = ADW_PREFERENCES_GROUP (adw_preferences_group_new ());
    adw_preferences_group_set_title (colors_group, _("Colors"));

    GtkWidget *fore_button = gtk_color_dialog_button_new (gtk_color_dialog_new ());
    AdwActionRow *fore_row = ADW_ACTION_ROW (adw_action_row_new ());
    adw_preferences_row_set_title (ADW_PREFERENCES_ROW (fore_row), _("Foreground color"));
    adw_action_row_add_suffix (fore_row, fore_button);
    adw_action_row_add_suffix (fore_row, make_reset_button (settings, FORECOLOR_KEY));
    adw_action_row_set_activatable_widget (fore_row, fore_button);
    g_settings_bind_with_mapping (settings, FORECOLOR_KEY, fore_button, "rgba",
                                  G_SETTINGS_BIND_DEFAULT,
                                  color_get_mapping, color_set_mapping, NULL, NULL);
    adw_preferences_group_add (colors_group, GTK_WIDGET (fore_row));

    GtkWidget *back_button = gtk_color_dialog_button_new (gtk_color_dialog_new ());
    AdwActionRow *back_row = ADW_ACTION_ROW (adw_action_row_new ());
    adw_preferences_row_set_title (ADW_PREFERENCES_ROW (back_row), _("Background color"));
    adw_action_row_add_suffix (back_row, back_button);
    adw_action_row_add_suffix (back_row, make_reset_button (settings, BACKCOLOR_KEY));
    adw_action_row_set_activatable_widget (back_row, back_button);
    g_settings_bind_with_mapping (settings, BACKCOLOR_KEY, back_button, "rgba",
                                  G_SETTINGS_BIND_DEFAULT,
                                  color_get_mapping, color_set_mapping, NULL, NULL);
    adw_preferences_group_add (colors_group, GTK_WIDGET (back_row));

    adw_preferences_page_add (appearance, colors_group);
    adw_preferences_dialog_add (dialog, appearance);

    /* --- Terminal page ----------------------------------------------------- */
    AdwPreferencesPage *terminal = ADW_PREFERENCES_PAGE (adw_preferences_page_new ());
    adw_preferences_page_set_title (terminal, _("Terminal"));
    adw_preferences_page_set_icon_name (terminal, "preferences-system-symbolic");

    /* Behavior group */
    AdwPreferencesGroup *behavior_group = ADW_PREFERENCES_GROUP (adw_preferences_group_new ());
    adw_preferences_group_set_title (behavior_group, _("Behavior"));

    GtkWidget *bell_row = adw_switch_row_new ();
    adw_preferences_row_set_title (ADW_PREFERENCES_ROW (bell_row), _("Audible bell"));
    adw_action_row_add_suffix (ADW_ACTION_ROW (bell_row), make_reset_button (settings, AUDIBLE_BELL_KEY));
    g_settings_bind (settings, AUDIBLE_BELL_KEY, bell_row, "active", G_SETTINGS_BIND_DEFAULT);
    adw_preferences_group_add (behavior_group, bell_row);

    GtkWidget *scrollback_row = adw_spin_row_new_with_range (1.0, 1000000.0, 1024.0);
    adw_preferences_row_set_title (ADW_PREFERENCES_ROW (scrollback_row), _("Scrollback lines"));
    adw_action_row_add_suffix (ADW_ACTION_ROW (scrollback_row), make_reset_button (settings, SCROLLBACK_KEY));
    g_settings_bind_with_mapping (settings, SCROLLBACK_KEY, scrollback_row, "value",
                                  G_SETTINGS_BIND_DEFAULT,
                                  int_to_double, double_to_int, NULL, NULL);
    adw_preferences_group_add (behavior_group, scrollback_row);

    GtkWidget *word_chars_row = adw_entry_row_new ();
    adw_preferences_row_set_title (ADW_PREFERENCES_ROW (word_chars_row), _("Word char exceptions"));
    adw_entry_row_add_suffix (ADW_ENTRY_ROW (word_chars_row), make_reset_button (settings, WORD_CHAR_EXCEPTIONS_KEY));
    g_settings_bind (settings, WORD_CHAR_EXCEPTIONS_KEY, word_chars_row, "text", G_SETTINGS_BIND_DEFAULT);
    adw_preferences_group_add (behavior_group, word_chars_row);

    adw_preferences_page_add (terminal, behavior_group);

    /* Window group */
    AdwPreferencesGroup *window_group = ADW_PREFERENCES_GROUP (adw_preferences_group_new ());
    adw_preferences_group_set_title (window_group, _("Window"));

    GtkWidget *decorated_row = adw_switch_row_new ();
    adw_preferences_row_set_title (ADW_PREFERENCES_ROW (decorated_row), _("Window decorations"));
    adw_action_row_add_suffix (ADW_ACTION_ROW (decorated_row), make_reset_button (settings, DECORATED_KEY));
    g_settings_bind (settings, DECORATED_KEY, decorated_row, "active", G_SETTINGS_BIND_DEFAULT);
    adw_preferences_group_add (window_group, decorated_row);

    adw_preferences_page_add (terminal, window_group);
    adw_preferences_dialog_add (dialog, terminal);

    /* --- Shell page -------------------------------------------------------- */
    AdwPreferencesPage *shell = ADW_PREFERENCES_PAGE (adw_preferences_page_new ());
    adw_preferences_page_set_title (shell, _("Shell"));
    adw_preferences_page_set_icon_name (shell, "utilities-terminal-symbolic");

    /* Command group */
    AdwPreferencesGroup *command_group = ADW_PREFERENCES_GROUP (adw_preferences_group_new ());
    adw_preferences_group_set_title (command_group, _("Command"));

    GtkWidget *startup_row = adw_entry_row_new ();
    adw_preferences_row_set_title (ADW_PREFERENCES_ROW (startup_row), _("Startup command"));
    adw_entry_row_add_suffix (ADW_ENTRY_ROW (startup_row), make_reset_button (settings, STARTUP_COMMAND_KEY));
    g_settings_bind (settings, STARTUP_COMMAND_KEY, startup_row, "text", G_SETTINGS_BIND_DEFAULT);
    adw_preferences_group_add (command_group, startup_row);

    GtkWidget *term_row = adw_entry_row_new ();
    adw_preferences_row_set_title (ADW_PREFERENCES_ROW (term_row), _("TERM variable"));
    adw_entry_row_add_suffix (ADW_ENTRY_ROW (term_row), make_reset_button (settings, TERM_KEY));
    g_settings_bind (settings, TERM_KEY, term_row, "text", G_SETTINGS_BIND_DEFAULT);
    adw_preferences_group_add (command_group, term_row);

    adw_preferences_page_add (shell, command_group);
    adw_preferences_dialog_add (dialog, shell);

    return ADW_DIALOG (dialog);
}
