// SPDX-FileCopyrightText: 2026 Marc-Antoine Perennou <Marc-Antoine@Perennou.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "germinal-about.h"

#include <glib/gi18n-lib.h>

AdwDialog *
germinal_about_new (void)
{
    static const gchar *developers[] = { "Marc-Antoine Perennou <Marc-Antoine@Perennou.com>", NULL };

    AdwAboutDialog *about = ADW_ABOUT_DIALOG (adw_about_dialog_new ());

    adw_about_dialog_set_application_name (about, "Germinal");
    adw_about_dialog_set_application_icon (about, "utilities-terminal");
    adw_about_dialog_set_developer_name (about, "Marc-Antoine Perennou");
    adw_about_dialog_set_version (about, PACKAGE_VERSION);
    adw_about_dialog_set_comments (about, _("Minimalist VTE-based terminal emulator"));
    adw_about_dialog_set_website (about, "https://github.com/Keruspe/Germinal");
    adw_about_dialog_set_issue_url (about, "https://github.com/Keruspe/Germinal/issues");
    adw_about_dialog_set_copyright (about, "© 2011-2026 Marc-Antoine Perennou");
    adw_about_dialog_set_license_type (about, GTK_LICENSE_GPL_3_0);
    adw_about_dialog_set_developers (about, developers);

    return ADW_DIALOG (about);
}
