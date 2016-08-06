
#pragma once

#include "libs/types.h"

struct Picker;
struct Layout;

namespace prefs {
    void show_panel(Picker* picker, Color* colors, Layout& layout);
}
