#include "display_off.h"

#include "config_values.h"

enum {
    MS_PER_MINUTE = 60000,
    LONGEST_MINUTES = 35000,
};

typedef struct display_times {
    const char *sleep_key;
    int sleep_minutes;
    const char *off_key;
    int off_minutes;
} display_times;

static const display_times ac_times = { "dpms-on-ac-sleep", 10, "dpms-on-ac-off", 15 };
static const display_times battery_times = { "dpms-on-battery-sleep", 5, "dpms-on-battery-off",
                                             10 };

static int read_minutes(const setting_source *settings, const char *key, int fallback);
static int earliest(int sleep_minutes, int off_minutes);
static bool is_set(const setting_source *settings, const char *key, bool fallback);

uint32_t display_off_ms_from(const setting_source *settings, bool battery)
{
    const display_times *times = battery ? &battery_times : &ac_times;
    if (!is_set(settings, "dpms-enabled", true) || is_set(settings, "presentation-mode", false)) {
        return 0;
    }
    int sleep_minutes = read_minutes(settings, times->sleep_key, times->sleep_minutes);
    int off_minutes = read_minutes(settings, times->off_key, times->off_minutes);
    int minutes = earliest(sleep_minutes, off_minutes);
    return (uint32_t) (minutes < LONGEST_MINUTES ? minutes : LONGEST_MINUTES) * MS_PER_MINUTE;
}

static bool is_set(const setting_source *settings, const char *key, bool fallback)
{
    const char *text = setting_source_lookup(settings, key);

    return text != NULL ? config_bool(text) : fallback;
}

static int read_minutes(const setting_source *settings, const char *key, int fallback)
{
    const char *text = setting_source_lookup(settings, key);

    return text != NULL ? config_int(text) : fallback;
}

static int earliest(int sleep_minutes, int off_minutes)
{
    if (sleep_minutes <= 0) {
        return off_minutes > 0 ? off_minutes : 0;
    }
    if (off_minutes <= 0) {
        return sleep_minutes;
    }
    return sleep_minutes < off_minutes ? sleep_minutes : off_minutes;
}
