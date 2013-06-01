/*
 * This file is part of Germinal.
 *
 * Copyright 2011-2013 Marc-Antoine Perennou <Marc-Antoine@Perennou.com>
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
#include <vte/vte.h>

#include "germinal-util.h"

static void
germinal_exit (GtkWidget *widget    GERMINAL_UNUSED,
               gpointer   user_data GERMINAL_UNUSED)
{
    gtk_main_quit ();
}

static gboolean
do_copy (GtkWidget *widget    GERMINAL_UNUSED,
         gpointer   user_data GERMINAL_UNUSED)
{
    vte_terminal_copy_clipboard (VTE_TERMINAL (user_data));
    return TRUE;
}

static gboolean
do_paste (GtkWidget *widget GERMINAL_UNUSED,
          gpointer   user_data)
{
    vte_terminal_paste_clipboard (VTE_TERMINAL (user_data));
    return TRUE;
}

static gchar *
get_url (VteTerminal    *terminal,
         GdkEventButton *button_event)
{
    static gchar *url = NULL; /* cache the url */
    if (button_event) /* only access to cached url if no button_event available */
    {
        g_free (url); /* free previous url */
        glong column = (glong)button_event->x / vte_terminal_get_char_width (terminal);
        glong row = (glong)button_event->y / vte_terminal_get_char_height (terminal);
        gint tag; /* avoid stupid vte segv (said to be optional) */
        url = vte_terminal_match_check (terminal,
                                        column,
                                        row,
                                        &tag);
    }
    return url;
}

static gboolean
open_url (gchar *url)
{
    if (!url)
        return FALSE;

    GError GERMINAL_ERROR_CLEANUP *error = NULL;
    /* Always strdup because we free later */
    gchar GERMINAL_STR_CLEANUP *browser = g_strdup (g_getenv ("BROWSER"));

    /* If BROWSER is not in env, try xdg-open or fallback to firefox */
    if (!browser && !(browser = g_find_program_in_path ("xdg-open")))
        browser = g_strdup ("firefox");

    gchar *cmd[] = {browser, url, NULL};
    if (!g_spawn_async (NULL, /* working directory */
                        cmd,
                        NULL, /* env */
                        G_SPAWN_SEARCH_PATH,
                        NULL, /* child setup */
                        NULL, /* child setup data */
                        NULL, /* child pid */
                        &error))
        fprintf (stderr, _("Couldn't exec \"%s %s\": %s"), browser, url, error->message);

    return TRUE;
}

static gboolean
do_copy_url (GtkWidget *widget GERMINAL_UNUSED,
             gpointer   user_data)
{
    gchar *url = get_url (VTE_TERMINAL (user_data), NULL);
    if (!url)
        return FALSE;
    GtkClipboard *clip = gtk_clipboard_get (GDK_SELECTION_CLIPBOARD);
    gtk_clipboard_set_text (clip, url, -1);
    clip = gtk_clipboard_get (GDK_SELECTION_PRIMARY);
    gtk_clipboard_set_text (clip, url, -1);
    return TRUE;
}

static gboolean
do_open_url (GtkWidget *widget GERMINAL_UNUSED,
             gpointer   user_data)
{
    return open_url (get_url (VTE_TERMINAL (user_data), NULL));
}

typedef enum {
    FONT_SIZE_DELTA_RESET = 0,
    FONT_SIZE_DELTA_INC = 1,
    FONT_SIZE_DELTA_DEC = -1,
    FONT_SIZE_DELTA_SET_DEFAULT = 42
} FontSizeDelta;

static void
update_font_size (VteTerminal  *terminal,
                  FontSizeDelta delta)
{
    static gdouble default_size = 0;
    PangoFontDescription GERMINAL_FONT_CLEANUP *font = pango_font_description_copy (vte_terminal_get_font (terminal));
    gdouble size = pango_font_description_get_size (font);
    switch (delta)
    {
    case FONT_SIZE_DELTA_SET_DEFAULT:
        default_size = size;
        return;
    case FONT_SIZE_DELTA_RESET:
        size = default_size;
        break;
    default:
        size = CLAMP (size / PANGO_SCALE + delta, 4., 144.) * PANGO_SCALE;
        break;
    }
    pango_font_description_set_size (font, size);
    vte_terminal_set_font (terminal, font);
}

static gboolean
do_zoom (GtkWidget *widget GERMINAL_UNUSED,
         gpointer   user_data)
{
    update_font_size (VTE_TERMINAL (user_data), FONT_SIZE_DELTA_INC);
    return TRUE;
}

static gboolean
do_dezoom (GtkWidget *widget GERMINAL_UNUSED,
           gpointer   user_data)
{
    update_font_size (VTE_TERMINAL (user_data), FONT_SIZE_DELTA_DEC);
    return TRUE;
}

static gboolean
do_reset_zoom (GtkWidget *widget GERMINAL_UNUSED,
               gpointer   user_data)
{
    update_font_size (VTE_TERMINAL (user_data), FONT_SIZE_DELTA_RESET);
    return TRUE;
}

static gboolean
on_key_press (GtkWidget   *widget,
              GdkEventKey *event,
              gpointer     user_data)
{
    if (event->type != GDK_KEY_PRESS)
        return FALSE;

    /* Ctrl + foo */
    if (event->state & GDK_CONTROL_MASK)
    {
        /* Ctrl + Shift + foo */
        if (event->state & GDK_SHIFT_MASK)
        {
            switch (event->keyval)
            {
            case GDK_KEY_C:
                do_copy (widget, user_data);
                return TRUE;
            case GDK_KEY_V:
                do_paste (widget, user_data);
                return TRUE;
            }
        }
        switch (event->keyval)
        {
        case GDK_KEY_KP_Add:
        case GDK_KEY_plus:
            return do_zoom (widget, user_data);
        case GDK_KEY_KP_Subtract:
        case GDK_KEY_minus:
            return do_dezoom (widget, user_data);
        case GDK_KEY_KP_0:
        case GDK_KEY_0:
            return do_reset_zoom (widget, user_data);
        }
    }

    return FALSE;
}

static gboolean
on_button_press (GtkWidget      *widget,
                 GdkEventButton *button_event,
                 gpointer        user_data)
{
    if (button_event->type != GDK_BUTTON_PRESS)
        return FALSE;

    gchar *url = get_url (VTE_TERMINAL (widget), button_event);
    /* Shift + Left clic */
    if ((button_event->button == 1) &&
        (button_event->state & GDK_SHIFT_MASK))
            return open_url (url);
    else if (button_event->button == 3)
    {
        if (url) /* show url stuff */
            gtk_widget_show_all (GTK_WIDGET (user_data));
        else
        {
            /* hide url stuff */
            GList *children = gtk_container_get_children (GTK_CONTAINER (user_data));
            gtk_widget_hide (GTK_WIDGET (children->data));
            gtk_widget_hide (GTK_WIDGET (children->next->data));
            gtk_widget_hide (GTK_WIDGET (children->next->next->data));
        }
        gtk_menu_popup (GTK_MENU (user_data),
                        NULL, /* parent menu_shell */
                        NULL, /* parent menu_item */
                        NULL, /* menu position func */
                        NULL, /* user_data */
                        button_event->button,
                        button_event->time);
        return TRUE;
    }

    return FALSE;
}

static gchar *
get_setting (GSettings   *settings,
             const gchar *name)
{
    return (name) ? g_settings_get_string (settings, name) : NULL;
}

static GdkColor *
get_palette (GSettings   *settings,
             const gchar *name,
             gsize *palette_size)
{
    gchar GERMINAL_STRV_CLEANUP **colors = NULL;
    guint size, i;
    GdkColor *palette;

    colors = g_settings_get_strv (settings, name);
    size = g_strv_length (colors);
    if (!((size == 0) ||
          (size == 8) ||
          (size == 16) ||
          (size == 24) ||
          ((size >= 25) && (size <= 255))))
    {
        g_settings_reset (settings, name);
        return get_palette (settings, name, palette_size);
    }

    palette = g_new(GdkColor, size);
    for (i = 0 ; i < size ; ++i)
        gdk_color_parse(colors[i], &palette[i]);

    *palette_size = size;
    return palette;
}

static void
update_scrollback (GSettings   *settings,
                   const gchar *key,
                   gpointer     user_data)
{
    vte_terminal_set_scrollback_lines (VTE_TERMINAL (user_data), g_settings_get_int (settings, key));
}

static void
update_word_chars (GSettings   *settings,
                   const gchar *key,
                   gpointer     user_data)
{
    gchar GERMINAL_STR_CLEANUP *setting = get_setting (settings, key);
    vte_terminal_set_word_chars (VTE_TERMINAL (user_data), setting);
}

static void
update_font (GSettings   *settings,
             const gchar *key,
             gpointer     user_data)
{
    gchar GERMINAL_STR_CLEANUP *setting = get_setting (settings, key);
    vte_terminal_set_font_from_string (VTE_TERMINAL (user_data), setting);
    update_font_size (VTE_TERMINAL (user_data), FONT_SIZE_DELTA_SET_DEFAULT);
}

static GdkColor *
update_colors (GSettings   *settings,
               const gchar *key, /* NULL for initialization */
               gpointer     user_data) /* NULL to get the palette */
{
    static GdkColor forecolor, backcolor;
    static GdkColor *palette = NULL;
    static gsize palette_size;

    if (!user_data)
        return palette;

    if (key == NULL)
    {
        gchar GERMINAL_STR_CLEANUP *backcolor_str = get_setting (settings, BACKCOLOR_KEY);
        gdk_color_parse (backcolor_str, &backcolor);
        gchar GERMINAL_STR_CLEANUP *forecolor_str = get_setting (settings, FORECOLOR_KEY);
        gdk_color_parse (forecolor_str, &forecolor);
        palette = get_palette(settings, PALETTE_KEY, &palette_size);
    }
    else if (strcmp (key, BACKCOLOR_KEY) == 0)
    {
        gchar GERMINAL_STR_CLEANUP *backcolor_str = get_setting (settings, BACKCOLOR_KEY);
        gdk_color_parse (backcolor_str, &backcolor);
    }
    else if (strcmp (key, FORECOLOR_KEY) == 0)
    {
        gchar GERMINAL_STR_CLEANUP *forecolor_str = get_setting (settings, FORECOLOR_KEY);
        gdk_color_parse (forecolor_str, &forecolor);
    }
    else if (strcmp (key, PALETTE_KEY) == 0)
    {
        g_free(palette);
        palette = get_palette(settings, PALETTE_KEY, &palette_size);
    }

    vte_terminal_set_colors (VTE_TERMINAL (user_data),
                             &forecolor,
                             &backcolor,
                             palette,
                             palette_size);

    return NULL;
}

static void
update_background_image (GSettings   *settings,
                         const gchar *key,
                         gpointer     user_data)
{
    gchar GERMINAL_STR_CLEANUP *setting = get_setting (settings, key);
    vte_terminal_set_background_image_file (VTE_TERMINAL (user_data), setting);
}
static void
update_opacity (GSettings   *settings,
                const gchar *key,
                gpointer     user_data)
{
    gtk_widget_set_opacity (GTK_WIDGET (user_data), g_settings_get_double (settings, key));
}


int
main(int   argc,
     char *argv[])
{
    /* Options */
    gchar GERMINAL_STRV_CLEANUP **command = NULL;
    GOptionEntry options[] =
    {
        { G_OPTION_REMAINING, 'e', 0, G_OPTION_ARG_STRING_ARRAY, &command, N_("the command to launch"), "command" },
        { NULL, '\0', 0, G_OPTION_ARG_NONE, NULL, NULL, NULL }
    };

    /* Gettext and gtk initialization */
    textdomain(GETTEXT_PACKAGE);
    bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);
    bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
    gtk_init_with_args (&argc,
                        &argv,
                         N_(" - minimalist vte-based terminal emulator"),
                         options,
                         GETTEXT_PACKAGE,
                         NULL); /* error */

    /* Window settings */
    GtkWidget *window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_maximize (GTK_WINDOW (window));
    gtk_window_set_decorated (GTK_WINDOW (window), FALSE);

    /* Vte settings */
    GtkWidget *terminal = vte_terminal_new ();
    vte_terminal_set_mouse_autohide (VTE_TERMINAL (terminal), TRUE);
    vte_terminal_set_audible_bell (VTE_TERMINAL (terminal), FALSE);
    vte_terminal_set_visible_bell (VTE_TERMINAL (terminal), FALSE);
    vte_terminal_set_scroll_on_output (VTE_TERMINAL (terminal), FALSE);
    vte_terminal_set_scroll_on_keystroke (VTE_TERMINAL (terminal), TRUE);

    /* Fill window */
    gtk_container_add (GTK_CONTAINER (window), terminal);
    gtk_widget_grab_focus (terminal);
    gtk_widget_show_all (window);

    /* Url matching stuff */
    GRegex GERMINAL_REGEX_CLEANUP *url_regexp = g_regex_new (URL_REGEXP,
                                                             G_REGEX_CASELESS | G_REGEX_OPTIMIZE,
                                                             G_REGEX_MATCH_NOTEMPTY,
                                                             NULL); /* error */
    vte_terminal_match_add_gregex (VTE_TERMINAL (terminal),
                                   url_regexp,
                                   0);

    /* Apply user settings */
    GSettings GERMINAL_SETTINGS_CLEANUP *settings = g_settings_new ("org.gnome.Germinal");
    update_scrollback (settings, SCROLLBACK_KEY, terminal);
    g_signal_connect (G_OBJECT (settings),
                      "changed::" SCROLLBACK_KEY,
                      G_CALLBACK (update_scrollback),
                      terminal);
    update_word_chars (settings, WORD_CHARS_KEY, terminal);
    g_signal_connect (G_OBJECT (settings),
                      "changed::" WORD_CHARS_KEY,
                      G_CALLBACK (update_word_chars),
                      terminal);
    update_font (settings, FONT_KEY, terminal);
    g_signal_connect (G_OBJECT (settings),
                      "changed::" FONT_KEY,
                      G_CALLBACK (update_font),
                      terminal);
    update_colors (settings, NULL, terminal);
    g_signal_connect (G_OBJECT (settings),
                      "changed::" BACKCOLOR_KEY,
                      G_CALLBACK (update_colors),
                      terminal);
    g_signal_connect (G_OBJECT (settings),
                      "changed::" FORECOLOR_KEY,
                      G_CALLBACK (update_colors),
                      terminal);
    g_signal_connect (G_OBJECT (settings),
                      "changed::" PALETTE_KEY,
                      G_CALLBACK (update_colors),
                      terminal);
    update_background_image (settings, BACKGROUND_IMAGE_KEY, terminal);
    g_signal_connect (G_OBJECT (settings),
                      "changed::" BACKGROUND_IMAGE_KEY,
                      G_CALLBACK (update_background_image),
                      terminal);
    update_opacity (settings, OPACITY_KEY, window);
    g_signal_connect (G_OBJECT (settings),
                      "changed::" OPACITY_KEY,
                      G_CALLBACK (update_opacity),
                      window);

    /* Launch base command */
    gchar GERMINAL_STR_CLEANUP *cwd = g_get_current_dir ();
    
    if (G_LIKELY (command == NULL))
    {
        gchar GERMINAL_STR_CLEANUP *setting = get_setting (settings, STARTUP_COMMAND_KEY);
        command = g_strsplit (setting , " ", 0);
    }

    vte_terminal_fork_command_full (VTE_TERMINAL (terminal),
                                    VTE_PTY_DEFAULT,
                                    cwd,
                                    command,
                                    NULL, /* env */
                                    G_SPAWN_SEARCH_PATH,
                                    NULL, /* child setup */
                                    NULL, /* child setup data */
                                    NULL, /* child pid */
                                    NULL); /* error */

    /* Populate right click menu */
    GtkWidget *menu = gtk_menu_new ();

    GtkAction *copy_url_action = gtk_action_new ("copy_url",
                                                 _("Copy _url"),
                                                 NULL, /* tooltip */
                                                 NULL); /* stock_id */
    GtkWidget *copy_url_menu_item = gtk_action_create_menu_item (copy_url_action);
    g_signal_connect (G_OBJECT (copy_url_action),
                      "activate",
                      G_CALLBACK (do_copy_url),
                      terminal);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), copy_url_menu_item);

    GtkAction *open_url_action = gtk_action_new ("open_url",
                                                 _("_Open url"),
                                                 NULL, /* tooltip */
                                                 NULL); /* stock_id */
    GtkWidget *open_url_menu_item = gtk_action_create_menu_item (open_url_action);
    g_signal_connect (G_OBJECT (open_url_action),
                      "activate",
                      G_CALLBACK (do_open_url),
                      terminal);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), open_url_menu_item);

    gtk_menu_shell_append (GTK_MENU_SHELL (menu), gtk_separator_menu_item_new ());

    GtkAction *copy_action = gtk_action_new ("copy",
                                             _("_Copy"),
                                             NULL, /* tooltip */
                                             NULL); /* stock_id */
    GtkWidget *copy_menu_item = gtk_action_create_menu_item (copy_action);
    g_signal_connect (G_OBJECT (copy_action),
                      "activate",
                      G_CALLBACK (do_copy),
                      terminal);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), copy_menu_item);

    GtkAction *paste_action = gtk_action_new ("paste",
                                              _("_Paste"),
                                              NULL, /* tooltip */
                                              NULL); /* stock_id */
    GtkWidget *paste_menu_item = gtk_action_create_menu_item (paste_action);
    g_signal_connect (G_OBJECT (paste_action),
                      "activate",
                      G_CALLBACK (do_paste),
                      terminal);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), paste_menu_item);

    gtk_menu_shell_append (GTK_MENU_SHELL (menu), gtk_separator_menu_item_new ());

    GtkAction *zoom_action = gtk_action_new ("zoom",
                                             _("_Zoom"),
                                             NULL, /* tooltip */
                                             NULL); /* stock_id */
    GtkWidget *zoom_menu_item = gtk_action_create_menu_item (zoom_action);
    g_signal_connect (G_OBJECT (zoom_action),
                      "activate",
                      G_CALLBACK (do_zoom),
                      terminal);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), zoom_menu_item);

    GtkAction *dezoom_action = gtk_action_new ("dezoom",
                                               _("_Dezoom"),
                                               NULL, /* tooltip */
                                               NULL); /* stock_id */
    GtkWidget *dezoom_menu_item = gtk_action_create_menu_item (dezoom_action);
    g_signal_connect (G_OBJECT (dezoom_action),
                      "activate",
                      G_CALLBACK (do_dezoom),
                      terminal);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), dezoom_menu_item);

    GtkAction *reset_zoom_action = gtk_action_new ("reset-zoom",
                                                   _("_Reset zoom"),
                                                   NULL, /* tooltip */
                                                   NULL); /* stock_id */
    GtkWidget *reset_zoom_menu_item = gtk_action_create_menu_item (reset_zoom_action);
    g_signal_connect (G_OBJECT (reset_zoom_action),
                      "activate",
                      G_CALLBACK (do_reset_zoom),
                      terminal);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), reset_zoom_menu_item);

    gtk_menu_shell_append (GTK_MENU_SHELL (menu), gtk_separator_menu_item_new ());

    GtkWidget *im_submenu = gtk_menu_new ();
    vte_terminal_im_append_menuitems (VTE_TERMINAL (terminal), GTK_MENU_SHELL (im_submenu));
    GtkWidget *im_menu_item = gtk_menu_item_new_with_mnemonic (_("Input _Methods"));
    gtk_menu_item_set_submenu (GTK_MENU_ITEM (im_menu_item), im_submenu);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), im_menu_item);

    gtk_widget_show_all (menu);

    /* Bind signals */
    g_signal_connect (G_OBJECT (terminal),
                      "button-press-event",
                      G_CALLBACK (on_button_press),
                      menu);
    g_signal_connect (G_OBJECT (window),
                      "destroy",
                      G_CALLBACK (germinal_exit),
                      NULL);
    g_signal_connect (G_OBJECT (window),
                      "key-press-event",
                      G_CALLBACK (on_key_press),
                      terminal);
    g_signal_connect(G_OBJECT (terminal),
                     "child-exited",
                     G_CALLBACK (germinal_exit),
                     NULL);

    /* Launch program */
    gtk_main ();

    /* Free memory */
    g_free (get_url (NULL, NULL));
    g_free (update_colors (NULL, NULL, NULL));

    return 0;
}
