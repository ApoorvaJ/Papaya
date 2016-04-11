
#pragma warning (disable: 4312) // Warning C4312 during 64 bit compilation: 'type cast': conversion from 'uint32' to 'void *' of greater size
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#pragma warning (default: 4312)

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

 // EasyTab doesn't have support for all platforms yet, e.g. OS X
#ifndef PAPAYA_NO_EASYTAB_SUPPORT
#define EASYTAB_IMPLEMENTATION
#include "easytab.h"
#endif // PAPAYA_NO_EASYTAB_SUPPORT

#include "glew/glew.h"
#include "imgui/imgui.h"
#define GL_IMPLEMENTATION
#include "gl.h"

#define MATHLIB_IMPLEMENTATION
#include "mathlib.h"

#define TIMER_IMPLEMENTATION
#include "timer.h"

