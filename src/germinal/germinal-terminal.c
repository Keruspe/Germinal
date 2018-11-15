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

struct _GerminalTerminal
{
    VteTerminal parent_instance;
};

typedef struct
{
    gchar *url;
    guint *zero_keycodes;
    guint n_zero_keycodes;
} GerminalTerminalPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (GerminalTerminal, germinal_terminal, VTE_TYPE_TERMINAL)

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

static gboolean
on_scroll (GtkWidget      *widget,
           GdkEventScroll *event)
{
    GerminalTerminal *self = GERMINAL_TERMINAL (widget);

    if (event->state & GDK_CONTROL_MASK)
    {
        GdkScrollDirection direction;
        gdouble y;
        if (gdk_event_get_scroll_direction ((GdkEvent *) event, &direction))
        {
            switch (direction)
            {
                case GDK_SCROLL_UP:
                    germinal_terminal_zoom (self);
                    return GDK_EVENT_STOP;
                case GDK_SCROLL_DOWN:
                    germinal_terminal_dezoom (self);
                    return GDK_EVENT_STOP;
            }
        }
        else if (gdk_event_get_scroll_deltas ((GdkEvent*) event, NULL, &y))
        {
            if (y < 0)
            {
                germinal_terminal_zoom (self);
                return GDK_EVENT_STOP;
            }
            else
            {
                germinal_terminal_dezoom (self);
                return GDK_EVENT_STOP;
            }
        }
    }

    return GTK_WIDGET_CLASS (germinal_terminal_parent_class)->scroll_event (widget, event);
}

static void
germinal_terminal_finalize (GObject *object)
{
    GerminalTerminalPrivate *priv = germinal_terminal_get_instance_private (GERMINAL_TERMINAL (object));

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

    priv->url = NULL;

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
}

static void
germinal_terminal_class_init (GerminalTerminalClass *klass)
{
    G_OBJECT_CLASS (klass)->finalize = germinal_terminal_finalize;
    GTK_WIDGET_CLASS (klass)->scroll_event = on_scroll;
}

GtkWidget *
germinal_terminal_new (void)
{
    return g_object_new (GERMINAL_TYPE_TERMINAL, NULL);
}
