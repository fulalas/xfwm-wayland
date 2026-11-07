#include "device_settings.h"

device_setting device_setting_choice(uint32_t supported, uint32_t fallback, uint32_t current)
{
    return (device_setting){.available = true, .words = {supported, fallback, current}};
}

device_setting device_setting_pair(uint32_t fallback, uint32_t current)
{
    return (device_setting){.available = true, .words = {fallback, current}};
}

bool device_setting_equal(const device_setting *a, const device_setting *b)
{
    if (a->available != b->available) {
        return false;
    }
    for (int i = 0; i < SETTING_WORDS; i++) {
        if (a->words[i] != b->words[i]) {
            return false;
        }
    }
    for (int i = 0; i < SETTING_NUMBERS; i++) {
        if (a->numbers[i] != b->numbers[i]) {
            return false;
        }
    }
    return true;
}
