#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <math.h>

global_variable uint8 *Image;

void AppUpdateAndRender(
	game_memory *Memory,  
	game_offscreen_buffer *Buffer)
{
	game_state *GameState = (game_state *)Memory->PermanentStorage;
	if (!Memory->IsInitialized)
	{
		// TODO: This may be more appropriate to do in the platform layer
		Memory->IsInitialized = true;
	}

	
}


//Image =	stbi_load("C:\\Users\\Apoorva\\Pictures\\ImageTest\\lenna.png", &ImageWidth, &ImageHeight, &ComponentsPerPixel, 0);
