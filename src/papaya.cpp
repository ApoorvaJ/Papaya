#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <math.h>

internal ImTextureID LoadAndBindImage(char* Path)
{
	uint8* Image;
	int32 ImageWidth, ImageHeight, ComponentsPerPixel;
	Image = stbi_load(Path, &ImageWidth, &ImageHeight, &ComponentsPerPixel, 0);

	// Create texture
	GLuint Id_GLuint;
	glGenTextures(1, &Id_GLuint);
	glBindTexture(GL_TEXTURE_2D, Id_GLuint);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, ImageWidth, ImageHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, Image);

	// Store our identifier
	free(Image);
	return (void *)(intptr_t)Id_GLuint;
}

internal void LoadImageIntoDocument(char* Path, PapayaDocument* Document)
{
	Document->Texture = stbi_load(Path, &Document->Width, &Document->Height, &Document->ComponentsPerPixel, 0);

	// Create texture
	glGenTextures(1, &Document->TextureID);
	glBindTexture(GL_TEXTURE_2D, Document->TextureID);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, Document->Width, Document->Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, Document->Texture);
}

void Papaya_Initialize(PapayaMemory* Memory)
{
	Memory->InterfaceTextureIDs[PapayaInterfaceTexture_TitleBarButtons] = (uint32)LoadAndBindImage("../res/img/win32_titlebar_buttons.png");
	Memory->InterfaceTextureIDs[PapayaInterfaceTexture_TitleBarIcon] = (uint32)LoadAndBindImage("../res/img/win32_titlebar_icon.png");

	Memory->Documents = (PapayaDocument*)malloc(sizeof(PapayaDocument));
	LoadImageIntoDocument("C:\\Users\\Apoorva\\Pictures\\ImageTest\\Fruits.png", Memory->Documents);
}

void Papaya_Shutdown(PapayaMemory* Memory)
{
	//TODO: Free document texture
	free(Memory->Documents);
}

void Papaya_UpdateAndRender(PapayaMemory* Memory)
{
	local_persist float TimeX;
	TimeX += 0.1f;
	for (int32 i = 0; i < Memory->Documents[0].Width * Memory->Documents[0].Width; i++)
	{
		*((uint8*)(Memory->Documents[0].Texture + i*4)) = 128 + (uint8)(128.0f * sin(TimeX));
	}
	glBindTexture(GL_TEXTURE_2D, Memory->Documents[0].TextureID);

	int32 XOffset = 128, YOffset = 128;
	int32 UpdateWidth = 256, UpdateHeight = 256;

	glPixelStorei(GL_UNPACK_ROW_LENGTH, Memory->Documents[0].Width);
	glPixelStorei(GL_UNPACK_SKIP_PIXELS, XOffset);
	glPixelStorei(GL_UNPACK_SKIP_ROWS, YOffset);
	//glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, Memory->Documents[0].Width, Memory->Documents[0].Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, Memory->Documents[0].Texture);
	glTexSubImage2D(GL_TEXTURE_2D, 0, XOffset, YOffset, UpdateWidth, UpdateHeight, GL_RGBA, GL_UNSIGNED_BYTE, Memory->Documents[0].Texture);

	#pragma region Render canvas
	{
		glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_TRANSFORM_BIT);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDisable(GL_CULL_FACE);
		glDisable(GL_DEPTH_TEST);
		glEnable(GL_SCISSOR_TEST);
		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glEnableClientState(GL_COLOR_ARRAY);
		glEnable(GL_TEXTURE_2D);

		// Setup orthographic projection matrix
		const float width = ImGui::GetIO().DisplaySize.x;
		const float height = ImGui::GetIO().DisplaySize.y;
		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();

		// TODO: Adjust this if required to try and reduce font blurriness
		float offset = 0.0f;
		glOrtho(0.0f+offset, width+offset, height+offset, 0.0f+offset, -1.0f, +1.0f);
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glLoadIdentity();

		ImVec2 Position = ImVec2((width - 512.0f)/2.0f, (height - 512.0f)/2.0f);
		ImVec2 Vertices[]  = 
		{ 
			ImVec2(Position.x, Position.y), ImVec2(512.0f + Position.x, Position.y), ImVec2(512.0f + Position.x, 512.0f + Position.y),
			ImVec2(Position.x, Position.y), ImVec2(512.0f + Position.x, 512.0f + Position.y), ImVec2(Position.x, 512.0f + Position.y) 
		};
		ImVec2 UVs[]  = { ImVec2(0.0f, 0.0f), ImVec2(1.0f, 0.0f), ImVec2(1.0f, 1.0f), ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f), ImVec2(0.0f, 1.0f) };
		uint32 Cols[] = { 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff };
		glVertexPointer(2, GL_FLOAT, sizeof(ImVec2), (void*)Vertices);
		glTexCoordPointer(2, GL_FLOAT, sizeof(ImVec2), (void*)UVs);
		glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(uint32), (void*)Cols);

		glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)Memory->Documents[0].TextureID);
		glScissor(0, 0, 2000, 2000);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		// Restore modified state
		glDisableClientState(GL_COLOR_ARRAY);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glDisableClientState(GL_VERTEX_ARRAY);
		glBindTexture(GL_TEXTURE_2D, 0);
		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();
		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
		glPopAttrib();
	}
	#pragma endregion
}
