// SPDX-FileCopyrightText: 2013-2026 Marc-Antoine Perennou <Marc-Antoine@Perennou.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "germinal-util.h"

#include <glib.h>

static GRegex *
make_regexp (void)
{
    g_autoptr (GError) error = NULL;
    GRegex *re = g_regex_new (URL_REGEXP,
                              G_REGEX_CASELESS | G_REGEX_OPTIMIZE,
                              G_REGEX_MATCH_NOTEMPTY,
                              &error);
    g_assert_no_error (error);
    g_assert_nonnull (re);
    return re;
}

static void
test_no_match (void)
{
    g_autoptr (GRegex) re = make_regexp ();
    const gchar *cases[] = {
        "foobar",
        "82:/foo",
        "://foo.com",
        "http://",
        NULL
    };
    for (const gchar **c = cases; *c; ++c)
        g_assert_false (g_regex_match (re, *c, 0, NULL));
}

static void
test_full_match (void)
{
    g_autoptr (GRegex) re = make_regexp ();
    const gchar *cases[] = {
        /* STRAIGHT_TEXT_ONLY: plain URLs */
        "http://google.com/",
        "https://example.com/",
        "ftp://ftp.example.com/file",
        "http://example.com:8080/path",
        "https://example.com/path?query=value&other=123",
        "https://example.com/path#fragment",
        /* PAREN_TEXT: parentheses in path */
        "http://en.wikipedia.org/wiki/Foo_(bar)",
        /* DUMB_USERS_TEXT: angle-bracket segment in path */
        "http://example.com/<tag>/rest",
        /* QUOTED_TEXT: quoted segment in path */
        "http://example.com/\"path\"",
        NULL
    };
    for (const gchar **c = cases; *c; ++c)
    {
        g_autoptr (GMatchInfo) info = NULL;
        g_assert_true (g_regex_match (re, *c, 0, &info));
        g_autofree gchar *match = g_match_info_fetch (info, 0);
        g_assert_cmpstr (match, ==, *c);
    }
}

static void
test_partial_match (void)
{
    g_autoptr (GRegex) re = make_regexp ();
    const struct { const gchar *input; const gchar *expected; } cases[] = {
        /* URL followed by trailing context */
        { "Google <http://google.com/>",                          "http://google.com/"                     },
        /* SQUARE_BRACED_TEXT: bracket segment; trailing period not stripped */
        { "foo:http://foo.bar:1234/[42] .",                       "http://foo.bar:1234/[42]"               },
        /* trailing comma stripped */
        { "Visit http://example.com/path, please",                "http://example.com/path"                },
        /* trailing single-quote stripped */
        { "Visit http://example.com/path' please",                "http://example.com/path"                },
        /* parenthesised URL in prose (Wikipedia-style) */
        { "see http://en.wikipedia.org/wiki/Foo_(bar) for more",  "http://en.wikipedia.org/wiki/Foo_(bar)" },
        { NULL, NULL }
    };
    for (guint i = 0; cases[i].input; ++i)
    {
        g_autoptr (GMatchInfo) info = NULL;
        g_assert_true (g_regex_match (re, cases[i].input, 0, &info));
        g_autofree gchar *match = g_match_info_fetch (info, 0);
        g_assert_cmpstr (match, ==, cases[i].expected);
    }
}

gint
main (gint argc, gchar *argv[])
{
    g_test_init (&argc, &argv, NULL);

    g_test_add_func ("/regexp/no-match",      test_no_match);
    g_test_add_func ("/regexp/full-match",    test_full_match);
    g_test_add_func ("/regexp/partial-match", test_partial_match);

    return g_test_run ();
}
