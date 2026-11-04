#include "accessibility_resolve.h"

#include "input_text.h"

static const char find_cursor_key[] = "FindCursor";

accessibility_prefs accessibility_prefs_from(const setting_source *source)
{
    const char *text = setting_source_lookup(source, find_cursor_key);
    optional_bool find_cursor = input_bool_from_text(text);

    return (accessibility_prefs) { .find_cursor = !find_cursor.is_set || find_cursor.value };
}
