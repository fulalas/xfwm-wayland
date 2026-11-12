#include <string.h>

#include <glib.h>

#include "check.h"
#include "mysvgsnap.h"

static const SvgCanvas canvas = { { 10, 20, 2.0 }, { 10, 20, 2.0 } };

static void check_snap(const char *svg, const char *expected)
{
    char *snapped = svgSnap(svg, &canvas);

    CHECK(strcmp(snapped, expected) == 0);
    g_free(snapped);
}

static void test_arc_compact_flags(void)
{
    check_snap("<path d=\"M0 0A5 5 0 0110 10\"/>", "<path d=\"M0 0A5 5 0 0 1 10 10\"/>");
}

static void test_arc_bad_flag_keeps_path(void)
{
    check_snap("<path d=\"M0 0A5 5 0 2 1 10 10\"/>", "<path d=\"M0 0A5 5 0 2 1 10 10\"/>");
}

static void test_group_transform_keeps_document(void)
{
    static const char svg[] = "<g transform=\"scale(2)\"><path d=\"M0,0L5,5\"/></g>";

    check_snap(svg, svg);
}

static void test_path_without_transform_is_rewritten(void)
{
    check_snap("<g><path d=\"M0,0L5,5\"/></g>", "<g><path d=\"M0 0L5 5\"/></g>");
}

int main(void)
{
    static const check_case cases[] = {
        { "arc_compact_flags", test_arc_compact_flags },
        { "arc_bad_flag_keeps_path", test_arc_bad_flag_keeps_path },
        { "group_transform_keeps_document", test_group_transform_keeps_document },
        { "path_without_transform_is_rewritten", test_path_without_transform_is_rewritten },
    };

    return check_run(cases, sizeof cases / sizeof cases[0]);
}
