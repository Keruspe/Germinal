// SPDX-FileCopyrightText: 2018-2026 Marc-Antoine Perennou <Marc-Antoine@Perennou.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "germinal-terminal.h"
#include "germinal-settings.h"
#include "germinal-util.h"

#define PCRE2_CODE_UNIT_WIDTH 0
#include <pcre2.h>

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

    GSignalGroup *settings_signals;
} GerminalTerminalPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (GerminalTerminal, germinal_terminal, VTE_TYPE_TERMINAL)

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
    g_autofree gchar *setting = g_settings_get_string (settings, key);

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
    g_autofree gchar *setting = g_settings_get_string (settings, key);

    germinal_terminal_update_font (GERMINAL_TERMINAL (user_data), setting);
}

static void
update_colors (GSettings   *settings,
               const gchar *key,
               gpointer     user_data)
{
    GerminalTerminalPrivate *priv = germinal_terminal_get_instance_private (user_data);

    if (g_strcmp0 (key, BACKCOLOR_KEY) == 0)
    {
        g_autofree gchar *backcolor_str = g_settings_get_string (settings, BACKCOLOR_KEY);

        gdk_rgba_parse (&priv->backcolor, backcolor_str);
    }
    else if (g_strcmp0 (key, FORECOLOR_KEY) == 0)
    {
        g_autofree gchar *forecolor_str = g_settings_get_string (settings, FORECOLOR_KEY);

        gdk_rgba_parse (&priv->forecolor, forecolor_str);
    }
    else if (g_strcmp0 (key, PALETTE_KEY) == 0)
    {
        g_clear_pointer (&priv->palette, g_free);
        priv->palette = germinal_settings_get_palette (settings, &priv->palette_size);
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

const gchar *
germinal_terminal_get_url (GerminalTerminal *self)
{
    GerminalTerminalPrivate *priv = germinal_terminal_get_instance_private (self);

    return priv->url;
}

void
germinal_terminal_update_url (GerminalTerminal *self,
                               gdouble           x,
                               gdouble           y)
{
    GerminalTerminalPrivate *priv = germinal_terminal_get_instance_private (self);
    gint tag; /* avoid stupid vte segv (said to be optional) */

    g_clear_pointer (&priv->url, g_free);
    priv->url = vte_terminal_check_match_at (VTE_TERMINAL (self), x, y, &tag);
}

gboolean
germinal_terminal_open_url (GerminalTerminal *self)
{
    const gchar *url = germinal_terminal_get_url (self);

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

    gchar *cmd[] = {browser, (gchar *) url, NULL};

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

#define ZOOM_FACTOR 1.2

void
germinal_terminal_zoom_in (GerminalTerminal *self)
{
    VteTerminal *terminal = VTE_TERMINAL (self);

    vte_terminal_set_font_scale (terminal, CLAMP (vte_terminal_get_font_scale (terminal) * ZOOM_FACTOR, 0.25, 4.0));
}

void
germinal_terminal_zoom_out (GerminalTerminal *self)
{
    VteTerminal *terminal = VTE_TERMINAL (self);

    vte_terminal_set_font_scale (terminal, CLAMP (vte_terminal_get_font_scale (terminal) / ZOOM_FACTOR, 0.25, 4.0));
}

void
germinal_terminal_reset_zoom (GerminalTerminal *self)
{
    vte_terminal_set_font_scale (VTE_TERMINAL (self), 1.0);
}

void
germinal_terminal_update_font (GerminalTerminal *self,
                               const gchar      *font_str)
{
    g_autoptr (PangoFontDescription) font = pango_font_description_from_string (font_str);

    vte_terminal_set_font (VTE_TERMINAL (self), font);
}

static void
on_terminal_command_spawned (VteTerminal *terminal  G_GNUC_UNUSED,
                             GPid         pid       G_GNUC_UNUSED,
                             GError      *error,
                             gpointer     user_data G_GNUC_UNUSED)
{
    if (error)
    {
        g_critical ("%s", error->message);
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
        g_autofree gchar *setting = g_settings_get_string (priv->settings, STARTUP_COMMAND_KEY);
        g_autoptr (GError) error = NULL;

        if (!g_shell_parse_argv (setting, NULL, &command, &error))
        {
            g_critical ("%s", error->message);
            exit (EXIT_FAILURE);
        }

        _free_command = command;
    }

    /* Override TERM */
    g_autofree gchar *term = g_settings_get_string (priv->settings, TERM_KEY);
    g_auto (GStrv) envp = g_environ_setenv (g_get_environ (), "TERM", term, TRUE);

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

static gboolean
on_scroll (GtkEventControllerScroll *controller,
           gdouble                   dx G_GNUC_UNUSED,
           gdouble                   dy,
           gpointer                  user_data)
{
    GerminalTerminal *self = GERMINAL_TERMINAL (user_data);
    GerminalTerminalPrivate *priv = germinal_terminal_get_instance_private (self);

    if (!(gtk_event_controller_get_current_event_state (GTK_EVENT_CONTROLLER (controller)) & GDK_CONTROL_MASK))
        return GDK_EVENT_PROPAGATE;

    if (dy == 0)
        return GDK_EVENT_PROPAGATE;

    gboolean natural_scroll = FALSE;

    GdkEvent *event = gtk_event_controller_get_current_event (GTK_EVENT_CONTROLLER (controller));
    GdkDevice *device = gdk_event_get_device (event);

    switch (gdk_device_get_source (device))
    {
        case GDK_SOURCE_MOUSE:
            natural_scroll = g_settings_get_boolean (priv->mouse_settings, "natural-scroll");
            break;
        case GDK_SOURCE_TOUCHPAD:
            natural_scroll = g_settings_get_boolean (priv->touchpad_settings, "natural-scroll");
            break;
        default:
            break;
    }

    if ((dy < 0) != natural_scroll)
        germinal_terminal_zoom_in (self);
    else
        germinal_terminal_zoom_out (self);

    return GDK_EVENT_STOP;
}

static void
germinal_terminal_dispose (GObject *object)
{
    GerminalTerminalPrivate *priv = germinal_terminal_get_instance_private (GERMINAL_TERMINAL (object));

    g_clear_object (&priv->settings_signals);
    g_clear_object (&priv->settings);
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
    g_clear_pointer (&priv->zero_keycodes, g_free);

    G_OBJECT_CLASS (germinal_terminal_parent_class)->finalize (object);
}

static gboolean on_key_pressed (GtkEventControllerKey *controller, guint keyval, guint keycode, GdkModifierType state, gpointer user_data);

static void
germinal_terminal_init (GerminalTerminal *self)
{
    GerminalTerminalPrivate *priv = germinal_terminal_get_instance_private (self);
    g_autofree GdkKeymapKey *zero_keys = NULL;
    g_autoptr (GError) error = NULL;
    gint n_keys;

    GSettings *settings = priv->settings = germinal_settings_new ();
    priv->mouse_settings = g_settings_new ("org.gnome.desktop.peripherals.mouse");
    priv->touchpad_settings = g_settings_new ("org.gnome.desktop.peripherals.touchpad");

    priv->settings_signals = g_signal_group_new (G_TYPE_SETTINGS);
    g_signal_group_connect (priv->settings_signals, "changed::" AUDIBLE_BELL_KEY,         G_CALLBACK (update_bell),                self);
    g_signal_group_connect (priv->settings_signals, "changed::" BACKCOLOR_KEY,            G_CALLBACK (update_colors),              self);
    g_signal_group_connect (priv->settings_signals, "changed::" FORECOLOR_KEY,            G_CALLBACK (update_colors),              self);
    g_signal_group_connect (priv->settings_signals, "changed::" PALETTE_KEY,              G_CALLBACK (update_colors),              self);
    g_signal_group_connect (priv->settings_signals, "changed::" FONT_KEY,                 G_CALLBACK (update_font),                self);
    g_signal_group_connect (priv->settings_signals, "changed::" SCROLLBACK_KEY,           G_CALLBACK (update_scrollback),          self);
    g_signal_group_connect (priv->settings_signals, "changed::" WORD_CHAR_EXCEPTIONS_KEY, G_CALLBACK (update_word_char_exceptions), self);
    g_signal_group_set_target (priv->settings_signals, settings);

    /* Init settings */
    update_bell                 (settings, AUDIBLE_BELL_KEY,         self);
    update_colors               (settings, BACKCOLOR_KEY,            self);
    update_colors               (settings, FORECOLOR_KEY,            self);
    update_colors               (settings, PALETTE_KEY,              self);
    update_font                 (settings, FONT_KEY,                 self);
    update_scrollback           (settings, SCROLLBACK_KEY,           self);
    update_word_char_exceptions (settings, WORD_CHAR_EXCEPTIONS_KEY, self);

    if (gdk_display_map_keyval (gdk_display_get_default (), GDK_KEY_0, &zero_keys, &n_keys))
    {
        priv->n_zero_keycodes = (guint) n_keys;
        priv->zero_keycodes = g_new (guint, priv->n_zero_keycodes);

        for (guint i = 0; i < priv->n_zero_keycodes; ++i)
            priv->zero_keycodes[i] = zero_keys[i].keycode;
    }

    GtkEventController *key_ctrl = gtk_event_controller_key_new ();
    gtk_event_controller_set_propagation_phase (key_ctrl, GTK_PHASE_CAPTURE);
    g_signal_connect (key_ctrl, "key-pressed", G_CALLBACK (on_key_pressed), self);
    gtk_widget_add_controller (GTK_WIDGET (self), key_ctrl);

    GtkEventController *scroll_ctrl = gtk_event_controller_scroll_new (GTK_EVENT_CONTROLLER_SCROLL_VERTICAL);
    gtk_event_controller_set_propagation_phase (scroll_ctrl, GTK_PHASE_CAPTURE);
    g_signal_connect (scroll_ctrl, "scroll", G_CALLBACK (on_scroll), self);
    gtk_widget_add_controller (GTK_WIDGET (self), scroll_ctrl);

    VteTerminal *term = (VteTerminal *) self;

    vte_terminal_set_mouse_autohide      (term, TRUE);
    vte_terminal_set_scroll_on_output    (term, FALSE);
    vte_terminal_set_scroll_on_keystroke (term, TRUE);
    vte_terminal_search_set_wrap_around  (term, TRUE);

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
copy_text (const gchar *text)
{
    GdkDisplay *display = gdk_display_get_default ();

    gdk_clipboard_set_text (gdk_display_get_clipboard (display), text);
    gdk_clipboard_set_text (gdk_display_get_primary_clipboard (display), text);
}

void
germinal_terminal_copy (GerminalTerminal *self)
{
    vte_terminal_copy_clipboard_format (VTE_TERMINAL (self), VTE_FORMAT_TEXT);
}

void
germinal_terminal_copy_html (GerminalTerminal *self)
{
    vte_terminal_copy_clipboard_format (VTE_TERMINAL (self), VTE_FORMAT_HTML);
}

void
germinal_terminal_paste (GerminalTerminal *self)
{
    vte_terminal_paste_clipboard (VTE_TERMINAL (self));
}

gboolean
germinal_terminal_copy_url (GerminalTerminal *self)
{
    const gchar *url = germinal_terminal_get_url (self);

    if (!url)
        return FALSE;

    copy_text (url);

    return TRUE;
}

static void
launch_cmd (GerminalTerminal *self,
            const gchar      *_cmd)
{
    g_auto (GStrv) cmd = NULL;
    g_autoptr (GError) error = NULL;

    if (!g_shell_parse_argv (_cmd, NULL, &cmd, &error))
    {
        g_warning ("%s", error->message);
        return;
    }

    if (!germinal_terminal_spawn (self, cmd, &error))
        g_warning ("%s", error->message);
}

static gboolean
on_key_pressed (GtkEventControllerKey *controller G_GNUC_UNUSED,
                guint                  keyval,
                guint                  keycode,
                GdkModifierType        state,
                gpointer               user_data)
{
    GerminalTerminal *self = GERMINAL_TERMINAL (user_data);

    if (!(state & GDK_CONTROL_MASK))
        return GDK_EVENT_PROPAGATE;

    switch (keyval)
    {
    /* Clipboard */
    case GDK_KEY_C:
        germinal_terminal_copy (self);
        return GDK_EVENT_STOP;
    case GDK_KEY_V:
        germinal_terminal_paste (self);
        return GDK_EVENT_STOP;
    /* Zoom */
    case GDK_KEY_KP_Add:
    case GDK_KEY_plus:
        germinal_terminal_zoom_in (self);
        return GDK_EVENT_STOP;
    case GDK_KEY_KP_Subtract:
    case GDK_KEY_minus:
        germinal_terminal_zoom_out (self);
        return GDK_EVENT_STOP;
    case GDK_KEY_KP_0:
    case GDK_KEY_0:
        germinal_terminal_reset_zoom (self);
        return GDK_EVENT_STOP;
    /* Quit */
    case GDK_KEY_Q:
        gtk_window_close (GTK_WINDOW (gtk_widget_get_root (GTK_WIDGET (self))));
        return GDK_EVENT_STOP;
    /* Window split (inspired by terminator) */
    case GDK_KEY_O:
        launch_cmd (self, "tmux split-window -v");
        return GDK_EVENT_STOP;
    case GDK_KEY_E:
        launch_cmd (self, "tmux split-window -h");
        return GDK_EVENT_STOP;
    /* Next/Previous window (tab) */
    case GDK_KEY_Tab:
        launch_cmd (self, "tmux next-window");
        return GDK_EVENT_STOP;
    case GDK_KEY_ISO_Left_Tab:
        launch_cmd (self, "tmux previous-window");
        return GDK_EVENT_STOP;
    /* New window (tab) */
    case GDK_KEY_T:
        launch_cmd (self, "tmux new-window");
        return GDK_EVENT_STOP;
    /* Next/Previous pane */
    case GDK_KEY_N:
        launch_cmd (self, "tmux select-pane -t :.+");
        return GDK_EVENT_STOP;
    case GDK_KEY_P:
        launch_cmd (self, "tmux select-pane -t :.-");
        return GDK_EVENT_STOP;
    /* Close current pane */
    case GDK_KEY_W:
        launch_cmd (self, "tmux kill-pane");
        return GDK_EVENT_STOP;
    /* Resize current pane */
    case GDK_KEY_X:
        launch_cmd (self, "tmux resize-pane -Z");
        return GDK_EVENT_STOP;
    }

    if (germinal_terminal_is_zero (self, keycode))
    {
        germinal_terminal_reset_zoom (self);
        return GDK_EVENT_STOP;
    }

    return GDK_EVENT_PROPAGATE;
}

gboolean
germinal_terminal_search (GerminalTerminal *self,
                          const gchar      *text)
{
    g_autoptr (GError) error = NULL;
    g_autoptr (VteRegex) regex = vte_regex_new_for_search (text, -1, PCRE2_CASELESS | PCRE2_MULTILINE, &error);

    if (error)
    {
        vte_terminal_search_set_regex (VTE_TERMINAL (self), NULL, 0);
        return FALSE;
    }

    vte_terminal_search_set_regex (VTE_TERMINAL (self), regex, 0);
    return vte_terminal_search_find_next (VTE_TERMINAL (self));
}

gboolean
germinal_terminal_search_next (GerminalTerminal *self)
{
    return vte_terminal_search_find_next (VTE_TERMINAL (self));
}

gboolean
germinal_terminal_search_prev (GerminalTerminal *self)
{
    return vte_terminal_search_find_previous (VTE_TERMINAL (self));
}

void
germinal_terminal_search_stop (GerminalTerminal *self)
{
    vte_terminal_search_set_regex (VTE_TERMINAL (self), NULL, 0);
}

static void
germinal_terminal_class_init (GerminalTerminalClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

    gobject_class->dispose = germinal_terminal_dispose;
    gobject_class->finalize = germinal_terminal_finalize;
}

GtkWidget *
germinal_terminal_new (void)
{
    return g_object_new (GERMINAL_TYPE_TERMINAL, NULL);
}
