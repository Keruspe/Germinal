#include "germinal-util.h"

#include <assert.h>

#define STRV_FOREACH(strv, name) for (const gchar **name = strv; *name; ++name)

#define FAIL(str) assert (!g_regex_match (url_regexp, str, 0, NULL))

#define CHECK_FULL(str, expected)                        \
    g_autoptr (GMatchInfo) info = NULL;                  \
    assert (g_regex_match (url_regexp, str, 0, &info));  \
    g_autofree gchar *_s = g_match_info_fetch (info, 0); \
    assert (!g_strcmp0 (expected, _s))
#define CHECK(str) CHECK_FULL (str, str)

gint
main (G_GNUC_UNUSED gint argc,
      G_GNUC_UNUSED gchar *argv[])
{
    const gchar *expect_failure[] = {
        "foobar",
        "82:/foo",
        NULL
    };

    const gchar *expect_self[] = {
        "http://google.com/",
        NULL
    };

    const gchar *expect_subpart[][2] = {
        { "Google <http://google.com/>",    "http://google.com/"       },
        { "foo:http://foo.bar:1234/[42] .", "http://foo.bar:1234/[42]" },
        { NULL }
    };

    g_autoptr (GRegex) url_regexp = g_regex_new (URL_REGEXP,
                                                             G_REGEX_CASELESS | G_REGEX_OPTIMIZE,
                                                             G_REGEX_MATCH_NOTEMPTY,
                                                             NULL); /* error */

    STRV_FOREACH (expect_failure, txt)
        FAIL  (*txt);

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
