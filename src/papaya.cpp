
internal void RenderWeirdGradient(
	game_offscreen_buffer *Buffer, 
	int BlueOffset, 
	int GreenOffset)
{
	// TODO: Let's see what the optimizer does

	uint8 *Row = (uint8 *)Buffer->Memory;
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

			*Pixel++ = ((Green << 8) | Blue);
		}

		Row += Buffer->Pitch;
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
