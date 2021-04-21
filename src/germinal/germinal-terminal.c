/*
 * This file is part of Germinal.
 *
 * Copyright 2018 Marc-Antoine Perennou <Marc-Antoine@Perennou.com>
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

#include "germinal-terminal.h"
#include "germinal-settings.h"

#define PCRE2_CODE_UNIT_WIDTH 0
#include <pcre2.h>

/* Watch a setting's changes */
#define SETTING_SIGNAL(key, fn)                                            \
    priv->c_signals[C_##key] = g_signal_connect (G_OBJECT (settings),      \
                                                 "changed::" key##_KEY,    \
                                                 G_CALLBACK (update_##fn), \
                                                 self)
#define SETTING_SIGNAL_CLEANUP(key)                                         \
    g_signal_handler_disconnect (priv->settings, priv->c_signals[C_##key]); \
    priv->c_signals[C_##key] = -1

enum
{
    C_AUDIBLE_BELL,
    C_BACKCOLOR,
    C_FORECOLOR,
    C_PALETTE,
    C_FONT,
    C_SCROLLBACK,
    C_WORD_CHAR_EXCEPTIONS,

    C_LAST_SIGNAL,
};

struct _GerminalTerminal
{
    VteTerminal parent_instance;
};

typedef struct
{
    GSettings *settings;
    GSettings *mouse_settings;
    GSettings *touchpad_settings;

    GdkRGBA   *palette;
    gsize      palette_size;
    GdkRGBA    forecolor;
    GdkRGBA    backcolor;


    gchar     *url;
    guint     *zero_keycodes;
    guint      n_zero_keycodes;

    guint64    c_signals[C_LAST_SIGNAL];
} GerminalTerminalPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (GerminalTerminal, germinal_terminal, VTE_TYPE_TERMINAL)

static gchar *
get_setting (GSettings   *settings,
             const gchar *name)
{
    return (name) ? g_settings_get_string (settings, name) : NULL;
}

static void
update_scrollback (GSettings   *settings,
                   const gchar *key,
                   gpointer     user_data)
{
    vte_terminal_set_scrollback_lines (VTE_TERMINAL (user_data), g_settings_get_int (settings, key));
}

static void
update_word_char_exceptions (GSettings   *settings,
                             const gchar *key,
                             gpointer     user_data)
{
    g_autofree gchar *setting = get_setting (settings, key);

    vte_terminal_set_word_char_exceptions (VTE_TERMINAL (user_data), setting);
}

static void
update_bell (GSettings   *settings,
             const gchar *key,
             gpointer     user_data)
{
    vte_terminal_set_audible_bell (VTE_TERMINAL (user_data), g_settings_get_boolean (settings, key));
}

static void
update_font (GSettings   *settings,
             const gchar *key,
             gpointer     user_data)
{
    g_autofree gchar *setting = get_setting (settings, key);

    germinal_terminal_update_font (GERMINAL_TERMINAL (user_data), setting);
}

static GdkRGBA *
get_palette (GSettings   *settings,
             const gchar *name,
             gsize       *palette_size)
{
    g_auto (GStrv) colors = NULL;
    guint size, i;
    GdkRGBA *palette;

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

    palette = g_new(GdkRGBA, size);

    for (i = 0 ; i < size ; ++i)
        gdk_rgba_parse (&palette[i], colors[i]);

    *palette_size = size;

    return palette;
}

static void
update_colors (GSettings   *settings,
               const gchar *key,
               gpointer     user_data)
{
    GerminalTerminalPrivate *priv = germinal_terminal_get_instance_private (user_data);

    if (strcmp (key, BACKCOLOR_KEY) == 0)
    {
        g_autofree gchar *backcolor_str = get_setting (priv->settings, BACKCOLOR_KEY);

        gdk_rgba_parse (&priv->backcolor, backcolor_str);
    }
    else if (strcmp (key, FORECOLOR_KEY) == 0)
    {
        g_autofree gchar *forecolor_str = get_setting (priv->settings, FORECOLOR_KEY);

        gdk_rgba_parse (&priv->forecolor, forecolor_str);
    }
    else if (strcmp (key, PALETTE_KEY) == 0)
    {
        g_free (priv->palette);
        priv->palette = get_palette(priv->settings, PALETTE_KEY, &priv->palette_size);
    }

    if (priv->palette)
    {
        vte_terminal_set_colors (VTE_TERMINAL (user_data),
                                 &priv->forecolor,
                                 &priv->backcolor,
                                 priv->palette,
                                 priv->palette_size);
    }
}

gboolean
germinal_terminal_is_zero (GerminalTerminal *self,
                           guint             keycode)
{
    GerminalTerminalPrivate *priv = germinal_terminal_get_instance_private (self);

    for (guint i = 0; i < priv->n_zero_keycodes; ++i)
    {
        if (priv->zero_keycodes[i] == keycode)
            return TRUE;
    }

    return FALSE;
}

gchar *
germinal_terminal_get_url (GerminalTerminal *self,
                           GdkEventButton   *button_event)
{
    GerminalTerminalPrivate *priv = germinal_terminal_get_instance_private (self);

    if (button_event) /* only access to cached url if no button_event available */
    {
        g_clear_pointer (&priv->url, g_free); /* free previous url */
        gint tag; /* avoid stupid vte segv (said to be optional) */

        priv->url = vte_terminal_match_check_event (VTE_TERMINAL (self), (GdkEvent *) button_event, &tag);
    }

    return priv->url;
}

gboolean
germinal_terminal_open_url (GerminalTerminal *self,
                            GdkEventButton   *button_event)
{
    gchar *url = germinal_terminal_get_url (self, button_event);

    if (!url)
        return FALSE;

    g_autoptr (GError) error = NULL;
    /* Always strdup because we free later */
    g_autofree gchar *browser = g_strdup (g_getenv ("BROWSER"));

    /* If BROWSER is not in env, try xdg-open or fallback to firefox */
    if (!browser)
        browser = g_find_program_in_path ("xdg-open");
    if (!browser)
        browser = g_strdup ("firefox");

    gchar *cmd[] = {browser, url, NULL};

    if (!germinal_terminal_spawn (self, cmd, &error))
        g_warning ("%s \"%s %s\": %s", _("Couldn't exec"), browser, url, error->message);

    return TRUE;
}

gboolean
germinal_terminal_spawn (GerminalTerminal *self G_GNUC_UNUSED,
                         gchar           **cmd,
                         GError          **error)
{
    g_auto (GStrv) env = g_get_environ ();

    return g_spawn_async (g_get_home_dir (),
                          cmd,
                          env,
                          G_SPAWN_SEARCH_PATH,
                          NULL, /* child setup */
                          NULL, /* child setup data */
                          NULL, /* child pid */
                          error);
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

    GERMINAL_FONT_CLEANUP PangoFontDescription *font = pango_font_description_copy (vte_terminal_get_font (terminal));
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

void
germinal_terminal_zoom (GerminalTerminal *self)
{
    update_font_size (VTE_TERMINAL (self), FONT_SIZE_DELTA_INC);
}

void
germinal_terminal_dezoom (GerminalTerminal *self)
{
    update_font_size (VTE_TERMINAL (self), FONT_SIZE_DELTA_DEC);
}

void
germinal_terminal_reset_zoom (GerminalTerminal *self)
{
    update_font_size (VTE_TERMINAL (self), FONT_SIZE_DELTA_RESET);
}

void
germinal_terminal_update_font (GerminalTerminal *self,
                               const gchar      *font_str)
{
    GERMINAL_FONT_CLEANUP PangoFontDescription *font = pango_font_description_from_string (font_str);
    VteTerminal *terminal = VTE_TERMINAL (self);

    vte_terminal_set_font (terminal, font);
    update_font_size (terminal, FONT_SIZE_DELTA_SET_DEFAULT);
}

static void
on_terminal_command_spawned (VteTerminal *terminal G_GNUC_UNUSED,
                             GPid         pid      G_GNUC_UNUSED,
                             GError      *error,
                             gpointer     user_data)
{
    if (error)
    {
        g_critical ("%s", error->message);
        g_error_free (error);
        exit (EXIT_FAILURE);
    }
}

void
germinal_terminal_spawn_command (GerminalTerminal *self,
                                 GStrv             command)
{
    g_auto (GStrv) _free_command = command;
    GerminalTerminalPrivate *priv = germinal_terminal_get_instance_private (self);

    if (G_LIKELY (!command))
    {
        g_autofree gchar *setting = get_setting (priv->settings, STARTUP_COMMAND_KEY);
        g_autoptr (GError) error = NULL;

        if (!g_shell_parse_argv (setting, NULL, &command, &error))
        {
            g_critical ("%s", error->message);
            exit (EXIT_FAILURE);
        }

        _free_command = command;
    }

    /* Override TERM */
    g_auto (GStrv) envp = g_environ_setenv (g_get_environ (), "TERM", get_setting (priv->settings, TERM_KEY), TRUE);

    /* Spawn our command */
    vte_terminal_spawn_async ((VteTerminal *) self, VTE_PTY_DEFAULT, g_get_home_dir (), command, envp, G_SPAWN_SEARCH_PATH,
                              NULL,  /* child_setup */
                              NULL,  /* child_setup_data */
                              NULL,  /* child_setup_data_destroy */
                              -1,    /* timeout */
                              NULL,  /* cancellable */
                              on_terminal_command_spawned,
                              NULL);
}

typedef enum {
    DO_NOTHING,
    ZOOM,
    DEZOOM
} ZoomAction;

static gboolean
on_scroll (GtkWidget      *widget,
           GdkEventScroll *event)
{
    GerminalTerminal *self = GERMINAL_TERMINAL (widget);
    GerminalTerminalPrivate *priv = germinal_terminal_get_instance_private (self);

    if (event->state & GDK_CONTROL_MASK)
    {
        ZoomAction zoom_action = DO_NOTHING;
        gboolean natural_scroll = FALSE;
        GdkScrollDirection direction;
        gdouble y;

        switch (gdk_device_get_source (gdk_event_get_source_device ((GdkEvent *) event)))
        {
            case GDK_SOURCE_MOUSE:
                natural_scroll = g_settings_get_boolean (priv->mouse_settings, "natural-scroll");
                break;
            case GDK_SOURCE_TOUCHPAD:
                natural_scroll = g_settings_get_boolean (priv->touchpad_settings, "natural-scroll");
                break;
        }

        if (gdk_event_get_scroll_direction ((GdkEvent *) event, &direction))
        {
            switch (direction)
            {
                case GDK_SCROLL_UP:
                    zoom_action = ZOOM;
                    break;
                case GDK_SCROLL_DOWN:
                    zoom_action = DEZOOM;
                    break;
            }
        }
        else if (gdk_event_get_scroll_deltas ((GdkEvent*) event, NULL, &y))
        {
            if (y < 0)
                zoom_action = ZOOM;
            else
                zoom_action = DEZOOM;
        }

        if (natural_scroll)
        {
            switch (zoom_action)
            {
                case ZOOM:
                    zoom_action = DEZOOM;
                    break;
                case DEZOOM:
                    zoom_action = ZOOM;
                    break;
            }
        }

        switch (zoom_action)
        {
            case ZOOM:
                germinal_terminal_zoom (self);
                return GDK_EVENT_STOP;
            case DEZOOM:
                germinal_terminal_dezoom (self);
                return GDK_EVENT_STOP;
        }
    }

    return GTK_WIDGET_CLASS (germinal_terminal_parent_class)->scroll_event (widget, event);
}

static void
germinal_terminal_dispose (GObject *object)
{
    GerminalTerminalPrivate *priv = germinal_terminal_get_instance_private (GERMINAL_TERMINAL (object));

    if (priv->settings)
    {
        SETTING_SIGNAL_CLEANUP (AUDIBLE_BELL);
        SETTING_SIGNAL_CLEANUP (BACKCOLOR);
        SETTING_SIGNAL_CLEANUP (FORECOLOR);
        SETTING_SIGNAL_CLEANUP (PALETTE);
        SETTING_SIGNAL_CLEANUP (FONT);
        SETTING_SIGNAL_CLEANUP (SCROLLBACK);
        SETTING_SIGNAL_CLEANUP (WORD_CHAR_EXCEPTIONS);
        g_clear_object (&priv->settings);
    }

    g_clear_object (&priv->mouse_settings);
    g_clear_object (&priv->touchpad_settings);

    G_OBJECT_CLASS (germinal_terminal_parent_class)->dispose (object);
}

static void
germinal_terminal_finalize (GObject *object)
{
    GerminalTerminalPrivate *priv = germinal_terminal_get_instance_private (GERMINAL_TERMINAL (object));

    g_clear_pointer (&priv->palette, g_free);
    g_clear_pointer (&priv->url, g_free);
    priv->n_zero_keycodes = 0;
    g_clear_pointer (&priv->zero_keycodes, g_free);

    G_OBJECT_CLASS (germinal_terminal_parent_class)->finalize (object);
}

static void
germinal_terminal_init (GerminalTerminal *self)
{
    GerminalTerminalPrivate *priv = germinal_terminal_get_instance_private (self);
    GdkKeymap *keymap = gdk_keymap_get_for_display (gdk_display_get_default ());
    g_autofree GdkKeymapKey *zero_keys = NULL;
    g_autoptr (GError) error = NULL;

    GSettings *settings = priv->settings = germinal_settings_new ();
    priv->mouse_settings = g_settings_new ("org.gnome.desktop.peripherals.mouse");
    priv->touchpad_settings = g_settings_new ("org.gnome.desktop.peripherals.touchpad");

    priv->url = NULL;

    SETTING_SIGNAL (AUDIBLE_BELL,         bell);
    SETTING_SIGNAL (BACKCOLOR,            colors);
    SETTING_SIGNAL (FORECOLOR,            colors);
    SETTING_SIGNAL (PALETTE,              colors);
    SETTING_SIGNAL (FONT,                 font);
    SETTING_SIGNAL (SCROLLBACK,           scrollback);
    SETTING_SIGNAL (WORD_CHAR_EXCEPTIONS, word_char_exceptions);

    /* Init settings */
    update_bell                 (settings, AUDIBLE_BELL_KEY,         self);
    update_colors               (settings, BACKCOLOR_KEY,            self);
    update_colors               (settings, FORECOLOR_KEY,            self);
    update_colors               (settings, PALETTE_KEY,              self);
    update_font                 (settings, FONT_KEY,                 self);
    update_scrollback           (settings, SCROLLBACK_KEY,           self);
    update_word_char_exceptions (settings, WORD_CHAR_EXCEPTIONS_KEY, self);

    if (gdk_keymap_get_entries_for_keyval (keymap, GDK_KEY_0, &zero_keys, &priv->n_zero_keycodes))
    {
        priv->zero_keycodes = g_new (guint, priv->n_zero_keycodes);

        for (guint i = 0; i < priv->n_zero_keycodes; ++i)
            priv->zero_keycodes[i] = zero_keys[i].keycode;
    }
    else
    {
        priv->zero_keycodes = NULL;
        priv->n_zero_keycodes = 0;
    }

    VteTerminal *term = (VteTerminal *) self;

    vte_terminal_set_mouse_autohide      (term, TRUE);
    vte_terminal_set_scroll_on_output    (term, FALSE);
    vte_terminal_set_scroll_on_keystroke (term, TRUE);

    /* Url matching stuff */
    g_autoptr (VteRegex) url_regexp = vte_regex_new_for_match (URL_REGEXP,
                                                              strlen (URL_REGEXP),
                                                              PCRE2_CASELESS | PCRE2_NOTEMPTY | PCRE2_MULTILINE,
                                                              &error);

    if (error)
    {
        g_critical ("%s", error->message);
        exit (EXIT_FAILURE);
    }

    vte_terminal_match_add_regex (term, url_regexp, 0);
}

static void
germinal_terminal_class_init (GerminalTerminalClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

    gobject_class->dispose = germinal_terminal_dispose;
    gobject_class->finalize = germinal_terminal_finalize;
    GTK_WIDGET_CLASS (klass)->scroll_event = on_scroll;
}

GtkWidget *
germinal_terminal_new (void)
{
    return g_object_new (GERMINAL_TYPE_TERMINAL, NULL);
}
