// SPDX-FileCopyrightText: 2013-2026 Marc-Antoine Perennou <Marc-Antoine@Perennou.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "germinal-util.h"

#include <assert.h>

#define STRV_FOREACH(strv, name) for (const gchar **name = strv; *name; ++name)

#define FAIL(str) assert (!g_regex_match (url_regexp, str, 0, NULL))

#define CHECK_FULL(str, expected)                            \
    G_STMT_START {                                           \
        g_autoptr (GMatchInfo) info = NULL;                  \
        assert (g_regex_match (url_regexp, str, 0, &info));  \
        g_autofree gchar *_s = g_match_info_fetch (info, 0); \
        assert (!g_strcmp0 (expected, _s));                  \
    } G_STMT_END

#define CHECK(str) CHECK_FULL (str, str)

gint
main (G_GNUC_UNUSED gint argc,
      G_GNUC_UNUSED gchar *argv[])
{
    g_autoptr (GError) error = NULL;
    g_autoptr (GRegex) url_regexp = g_regex_new (URL_REGEXP,
                                                  G_REGEX_CASELESS | G_REGEX_OPTIMIZE,
                                                  G_REGEX_MATCH_NOTEMPTY,
                                                  &error);
    assert (!error);
    assert (url_regexp);

    /* Should not match */
    const gchar *expect_failure[] = {
        "foobar",     /* no scheme */
        "82:/foo",    /* digit-only scheme */
        "://foo.com", /* empty scheme */
        "http://",    /* no host or path */
        NULL
    };

    /* Should match the whole string — one case per regex branch */
    const gchar *expect_self[] = {
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

    /* Should match only a substring — verifies correct boundary detection */
    const gchar *expect_subpart[][2] = {
        /* URL followed by trailing context */
        { "Google <http://google.com/>",             "http://google.com/"       },
        /* SQUARE_BRACED_TEXT: bracket segment; trailing period not stripped */
        { "foo:http://foo.bar:1234/[42] .",          "http://foo.bar:1234/[42]" },
        /* trailing comma stripped */
        { "Visit http://example.com/path, please",   "http://example.com/path"  },
        /* trailing single-quote stripped */
        { "Visit http://example.com/path' please",   "http://example.com/path"  },
        /* parenthesised URL in prose (Wikipedia-style) */
        { "see http://en.wikipedia.org/wiki/Foo_(bar) for more",
          "http://en.wikipedia.org/wiki/Foo_(bar)"                               },
        { NULL }
    };

    STRV_FOREACH (expect_failure, txt)
    {
        FAIL (*txt);
    }

    STRV_FOREACH (expect_self, txt)
    {
        CHECK (*txt);
    }

    for (guint i = 0; expect_subpart[i][0]; ++i)
    {
        CHECK_FULL (expect_subpart[i][0], expect_subpart[i][1]);
    }

    return 0;
}
