#ifndef XFCE_VALUE_TEXT_H
#define XFCE_VALUE_TEXT_H

#include <glib-object.h>

char *value_text_of(const GValue *value);
char *value_text_of_double(double value);

#endif
