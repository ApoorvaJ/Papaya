#pragma once

#define Kilobytes(Value) ((Value)*1024LL)
#define Megabytes(Value) (Kilobytes(Value)*1024LL)
#define Gigabytes(Value) (Megabytes(Value)*1024LL)
#define Terabytes(Value) (Gigabytes(Value)*1024LL)

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))


struct game_offscreen_buffer
{
	// Pixels are alwasy 32-bits wide, Memory Order BB GG RR XX
	void *Memory;
	int Width;
	int Height;
	int Pitch;
};

struct game_memory
{
	bool32 IsInitialized;

	uint64 PermanentStorageSize;
	void *PermanentStorage; // REQUIRED to be cleared to zero at startup

	uint64 TransientStorageSize;
	void *TransientStorage; // REQUIRED to be cleared to zero at startup
};

void GameUpdateAndRender(
	game_memory *Memory, 
	game_offscreen_buffer *Buffer);

//
//
//

struct game_state
{
	int GreenOffset;
	int BlueOffset;
};