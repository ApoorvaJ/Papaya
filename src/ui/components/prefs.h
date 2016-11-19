
#pragma once

#include "libs/types.h"

struct ColorPanel;
struct Layout;

namespace prefs {
    void show_panel(ColorPanel* color_panel, Color* colors, Layout& layout);
}
