#include "germinal-util.h"

#include <assert.h>

#define CHECK(str) assert (g_regex_match (url_regexp, str, 0, NULL))
#define FAIL(str) assert (!g_regex_match (url_regexp, str, 0, NULL))

gint
main (G_GNUC_UNUSED gint argc,
      G_GNUC_UNUSED gchar *argv[])
{
    GRegex GERMINAL_REGEX_CLEANUP *url_regexp = g_regex_new (URL_REGEXP,
                                                             G_REGEX_CASELESS | G_REGEX_OPTIMIZE,
                                                             G_REGEX_MATCH_NOTEMPTY,
                                                             NULL); /* error */
    CHECK ("https://google.com/");
    FAIL  ("foobar");
    return 0;
}
