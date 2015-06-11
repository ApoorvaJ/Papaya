#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_TRUETYPE_IMPLEMENTATION 
#include "stb_truetype.h"

#include <math.h>

global_variable uint8 *Image;
global_variable uint8 ttf_buffer[1<<25];

internal void RenderWeirdGradient(
	game_offscreen_buffer *Buffer, 
	int BlueOffset, 
	int GreenOffset)
{
	local_persist int ImageWidth, ImageHeight, ComponentsPerPixel, XOff, YOff;
	if (!Image)
	{
		//Image =	stbi_load("C:\\Users\\Apoorva\\Pictures\\ImageTest\\lenna.png", &ImageWidth, &ImageHeight, &ComponentsPerPixel, 0);

		stbtt_fontinfo font;

		int c = 0x0024F, s = 50;

		fread(ttf_buffer, 1, 1<<25, fopen("c:/windows/fonts/arialbd.ttf", "rb"));

		stbtt_InitFont(&font, ttf_buffer, stbtt_GetFontOffsetForIndex(ttf_buffer,0));
		Image = stbtt_GetCodepointBitmap(&font, 0.0f,stbtt_ScaleForPixelHeight(&font, (float)s), c, &ImageWidth, &ImageHeight, &XOff, &YOff);

	}
	else
	{
		uint8 *Row = (uint8 *)Buffer->Memory;
		uint8 *ImageCursor = Image;
		uint32 CurrentPixel = 0;

		uint8 FinalRed, FinalGreen, FinalBlue;

		for (int Y = 0;
			Y < Buffer->Height;
			++Y)
		{
			uint32 *Pixel = (uint32 *)Row;
			for (int X = 0;
				X < Buffer->Width;
				++X)
			{
				if (CurrentPixel / Buffer->Width >= (uint32)ImageHeight || CurrentPixel % Buffer->Width >= (uint32)ImageWidth)
				{
					FinalRed = 0;
					FinalGreen = 0;
					FinalBlue = 0;
				}
				else
				{
					uint8 Red = *ImageCursor++;
					uint8 Green = *ImageCursor++;
					uint8 Blue =  *ImageCursor++;

					uint8 GreyValue = (uint8)(0.299f * (float)Red + 0.587f * (float)Green + 0.114f * (float)Blue);

					float BlendFactor = (sinf(BlueOffset * 0.1f) + 1.0f) * 0.5f;
					FinalRed =		(uint8)((float)GreyValue * BlendFactor + (float)Red * (1.0f - (float)BlendFactor));
					FinalGreen =	(uint8)((float)GreyValue * BlendFactor + (float)Green * (1.0f - (float)BlendFactor));
					FinalBlue =		(uint8)((float)GreyValue * BlendFactor + (float)Blue * (1.0f - (float)BlendFactor));
				}

				*Pixel++ = ( ( FinalRed << 16) | (FinalGreen << 8) | FinalBlue );
				CurrentPixel++;
			}

			Row += Buffer->Pitch;
		}
	}
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
