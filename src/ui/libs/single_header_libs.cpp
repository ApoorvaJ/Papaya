
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

#include "imgui/imgui.h"

 // EasyKey doesn't have support for all platforms yet, e.g. OS X
#ifndef PAPAYA_NO_EASYKEY_SUPPORT
#define EASYKEY_IMPLEMENTATION
#define EASYKEY_IMGUI
#include "easykey.h"
#endif // PAPAYA_NO_EASYKEY_SUPPORT

#define GL_UTIL_IMPLEMENTATION
#include "gl_util.h"

#define GL_LITE_IMPLEMENTATION
#include "gl_lite.h"


#define MATHLIB_IMPLEMENTATION
#include "mathlib.h"

#define TIMER_IMPLEMENTATION
#include "timer.h"

