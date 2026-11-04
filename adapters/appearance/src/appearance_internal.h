#ifndef APPEARANCE_APPEARANCE_INTERNAL_H
#define APPEARANCE_APPEARANCE_INTERNAL_H

#include "appearance/appearance.h"
#include "frame_painter.h"
#include "ui_text.h"

frame_painter appearance_frame_painter(const appearance *appearance);
ui_text appearance_ui_text(const appearance *appearance);
const title_style *appearance_title_style(const appearance *appearance);

#endif
