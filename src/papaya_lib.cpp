
#include "papaya_lib.h"

#pragma warning (disable: 4312) // Warning C4312 during 64 bit compilation: 'type cast': conversion from 'uint32' to 'void *' of greater size
#define STB_IMAGE_IMPLEMENTATION
#include "lib/stb_image.h"
#pragma warning (default: 4312)

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "lib/stb_image_write.h"