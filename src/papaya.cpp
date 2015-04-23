#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

global_variable uint8 *Image;

internal void RenderWeirdGradient(
	game_offscreen_buffer *Buffer, 
	int BlueOffset, 
	int GreenOffset)
{
	local_persist int ImageWidth, ImageHeight, ComponentsPerPixel;
	if (!Image)
	{
		Image =	stbi_load("C:\\Users\\Apoorva\\Pictures\\Camera Roll\\picture000.jpg", &ImageWidth, &ImageHeight, &ComponentsPerPixel, 0);
	}
	else
	{
		uint8 *Row = (uint8 *)Buffer->Memory;
		uint8 *ImageCursor = Image;

		for (int Y = 0;
			Y < Buffer->Height;
			++Y)
		{
			uint32 *Pixel = (uint32 *)Row;
			for (int X = 0;
				X < Buffer->Width;
				++X)
			{
				uint8 Red = *ImageCursor++;
				uint8 Blue =  *ImageCursor++ + BlueOffset;
				uint8 Green = *ImageCursor++ + GreenOffset;



				*Pixel++ = ( (Red << 16) | (Green << 8) | Blue);
			}

			Row += Buffer->Pitch;
		}
	}

	// TODO: Let's see what the optimizer does

	/*uint8 *Row = (uint8 *)Buffer->Memory;
	for (int Y = 0;
		Y < Buffer->Height;
		++Y)
	{
		uint32 *Pixel = (uint32 *)Row;
		for (int X = 0;
			X < Buffer->Width;
			++X)
		{
			uint8 Blue = (X + BlueOffset);
			uint8 Green = (Y + GreenOffset);

			*Pixel++ = ((Green << 16) | Blue);
		}

		Row += Buffer->Pitch;
	}*/
}

void GameUpdateAndRender(
	game_memory *Memory,  
	game_offscreen_buffer *Buffer)
{
	game_state *GameState = (game_state *)Memory->PermanentStorage;
	if (!Memory->IsInitialized)
	{
		// TODO: This may be more appropriate to do in the platform layer
		Memory->IsInitialized = true;
	}

	GameState->BlueOffset += 1;
	GameState->GreenOffset += 1;
	RenderWeirdGradient(Buffer, GameState->BlueOffset, GameState->GreenOffset);
}
