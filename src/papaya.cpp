#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

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
	Document->Texture = stbi_load(Path, &Document->Width, &Document->Height, &Document->ComponentsPerPixel, 4);

	// Create texture
	glGenTextures(1, &Document->TextureID);
	glBindTexture(GL_TEXTURE_2D, Document->TextureID);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, Document->Width, Document->Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, Document->Texture);
}

void Papaya_Initialize(PapayaMemory* Memory)
{
	// Texture IDs
	Memory->InterfaceTextureIDs[PapayaInterfaceTexture_TitleBarButtons] = (uint32)LoadAndBindImage("../../img/win32_titlebar_buttons.png");
	Memory->InterfaceTextureIDs[PapayaInterfaceTexture_TitleBarIcon] = (uint32)LoadAndBindImage("../../img/win32_titlebar_icon.png");
	Memory->InterfaceTextureIDs[PapayaInterfaceTexture_InterfaceIcons] = (uint32)LoadAndBindImage("../../img/interface_icons.png");

	// Colors
#if 1
	Memory->InterfaceColors[PapayaInterfaceColor_Clear]			= Color(45,45,48); // Dark grey
#else
	Memory->InterfaceColors[PapayaInterfaceColor_Clear]			= Color(114,144,154); // Light blue
#endif
	Memory->InterfaceColors[PapayaInterfaceColor_Transparent]	= Color(0,0,0,0);
	Memory->InterfaceColors[PapayaInterfaceColor_ButtonHover]	= Color(64,64,64);
	Memory->InterfaceColors[PapayaInterfaceColor_ButtonActive]	= Color(0,122,204);

	Memory->Documents = (PapayaDocument*)malloc(sizeof(PapayaDocument));
	//LoadImageIntoDocument("C:\\Users\\Apoorva\\Pictures\\ImageTest\\fruits.png", Memory->Documents);
	LoadImageIntoDocument("C:\\Users\\Apoorva\\Pictures\\ImageTest\\test4k.jpg", Memory->Documents);
	Memory->Documents[0].CanvasPosition = Vec2((Memory->Window.Width - 512.0f)/2.0f, (Memory->Window.Height - 512.0f)/2.0f); // TODO: Center image on init
	Memory->Documents[0].CanvasZoom = 1.0f;

	glGenBuffers(1, &Memory->VertexBuffers[PapayaVertexBuffer_RenderToTexture].VboHandle);
	glBindBuffer(GL_ARRAY_BUFFER, Memory->VertexBuffers[PapayaVertexBuffer_RenderToTexture].VboHandle);
	glGenVertexArrays(1, &Memory->VertexBuffers[PapayaVertexBuffer_RenderToTexture].VaoHandle);
	glBindVertexArray(Memory->VertexBuffers[PapayaVertexBuffer_RenderToTexture].VaoHandle);
	glEnableVertexAttribArray(Memory->Shaders[PapayaShader_Brush].Attributes[0]); // Position attribute
	glEnableVertexAttribArray(Memory->Shaders[PapayaShader_Brush].Attributes[1]); // UV attribute
	glEnableVertexAttribArray(Memory->Shaders[PapayaShader_Brush].Attributes[2]); // Color attribute
#define OFFSETOF(TYPE, ELEMENT) ((size_t)&(((TYPE *)0)->ELEMENT))
	glVertexAttribPointer(Memory->Shaders[PapayaShader_Brush].Attributes[0], 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)OFFSETOF(ImDrawVert, pos));		 // Position attribute
	glVertexAttribPointer(Memory->Shaders[PapayaShader_Brush].Attributes[1], 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)OFFSETOF(ImDrawVert, uv));			 // UV attribute
	glVertexAttribPointer(Memory->Shaders[PapayaShader_Brush].Attributes[2], 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(ImDrawVert), (GLvoid*)OFFSETOF(ImDrawVert, col));	 // Color attribute
#undef OFFSETOF
	Vec2 Position = Vec2(0,0);
	Vec2 Size = Vec2((float)Memory->Documents[0].Width, (float)Memory->Documents[0].Height);
	ImDrawVert Verts[6];
	Verts[0].pos = Vec2(Position.x, Position.y);					Verts[0].uv = Vec2(0.0f, 1.0f); Verts[0].col = 0xffffffff;
	Verts[1].pos = Vec2(Size.x + Position.x, Position.y);			Verts[1].uv = Vec2(1.0f, 1.0f); Verts[1].col = 0xffffffff;
	Verts[2].pos = Vec2(Size.x + Position.x, Size.y + Position.y);	Verts[2].uv = Vec2(1.0f, 0.0f); Verts[2].col = 0xffffffff;
	Verts[3].pos = Vec2(Position.x, Position.y);					Verts[3].uv = Vec2(0.0f, 1.0f); Verts[3].col = 0xffffffff;
	Verts[4].pos = Vec2(Size.x + Position.x, Size.y + Position.y);	Verts[4].uv = Vec2(1.0f, 0.0f); Verts[4].col = 0xffffffff;
	Verts[5].pos = Vec2(Position.x, Size.y + Position.y);			Verts[5].uv = Vec2(0.0f, 0.0f); Verts[5].col = 0xffffffff;
	glBufferData(GL_ARRAY_BUFFER, sizeof(Verts), Verts, GL_STATIC_DRAW);
	//glBindVertexArray(0);
	//glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void Papaya_Shutdown(PapayaMemory* Memory)
{
	//TODO: Free document texture
	free(Memory->Documents);
}

internal void PaintPixel(int32 x, int32 y, uint32 Color, PapayaMemory* Memory)
{
	if (x >= 0 && x < Memory->Documents[0].Width &&
		y >= 0 && y < Memory->Documents[0].Height)
	{
		*((uint32*)(Memory->Documents[0].Texture + (Memory->Documents[0].Width * y * sizeof(uint32)) + x * sizeof(uint32))) = Color; // 0xAABBGGRR
	}
}

internal void PaintCircle(int32 x0, int32 y0, int32 Diameter, uint32 Color, PapayaMemory* Memory)
{
	int32 Min = -Diameter/2; 
	int32 Max =  Diameter/2 - (Diameter%2 == 0 ? 1 : 0);
	Vec2 Center = Vec2((float)x0 - (Diameter%2 == 0 ? 0.5f : 0.0f),
		               (float)y0 - (Diameter%2 == 0 ? 0.5f : 0.0f));
	float Range = (float)Diameter/2.0f;

	for (int32 i = x0 + Min; i <= x0 + Max; i++)
	{
		for (int32 j = y0 + Min; j <= y0 + Max; j++)
		{
			Vec2 Current = Vec2((float)i, (float)j);

			if(Math::DistanceSquared(Center, Current) <= Range * Range)
			{
				PaintPixel(i, j, Color, Memory);
			}
		}
	}
}

internal void RefreshTexture(PapayaMemory* Memory)
{
	glBindTexture(GL_TEXTURE_2D, Memory->Documents[0].TextureID);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, Memory->Documents[0].Width, Memory->Documents[0].Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, Memory->Documents[0].Texture);
}

void Papaya_UpdateAndRender(PapayaMemory* Memory, PapayaDebugMemory* DebugMemory)
{
	#pragma region Current mouse info
	{
		Memory->Mouse.Pos = ImGui::GetMousePos();
		Memory->Mouse.IsDown[0] = ImGui::IsMouseDown(0);
		Memory->Mouse.IsDown[1] = ImGui::IsMouseDown(1);
		Memory->Mouse.IsDown[2] = ImGui::IsMouseDown(2);
		Vec2 MousePixelPos = Vec2(Math::Floor((Memory->Mouse.Pos.x - Memory->Documents[0].CanvasPosition.x) / Memory->Documents[0].CanvasZoom),
								  Math::Floor((Memory->Mouse.Pos.y - Memory->Documents[0].CanvasPosition.y) / Memory->Documents[0].CanvasZoom));
		Memory->Mouse.UV = Vec2(MousePixelPos.x / (float) Memory->Documents[0].Width,
								MousePixelPos.y / (float) Memory->Documents[0].Height);
	}
	#pragma endregion

	local_persist int32 BrushSize = 1;
	local_persist Color BrushCol = Color(0.0f,1.0f,0.0f);

	if (ImGui::IsKeyPressed(VK_UP, false))
	{
		BrushSize++;
	}
	if (ImGui::IsKeyPressed(VK_DOWN, false))
	{
		BrushSize--;
	}


	if (Memory->Mouse.IsDown[0] || Memory->Mouse.IsDown[1])
	{
		BrushCol = (Memory->Mouse.IsDown[0]) ? Color(0.0f,1.0f,0.0f) : Color(1.0f,0.0f,0.0f);

		Util::StartTime(TimerScope_CPU_BRESENHAM, DebugMemory);

		glBindFramebuffer(GL_FRAMEBUFFER, Memory->FrameBufferObject);
		glViewport(0, 0, Memory->Documents[0].Width, Memory->Documents[0].Height);
		GLfloat col[4] = { 1.0, 0.0, 0.0, 1.0 };
		glClearBufferfv(GL_COLOR, 0, col);

		glDisable(GL_SCISSOR_TEST);

		// Setup orthographic projection matrix
		const float width = (float)Memory->Documents[0].Width;
		const float height = (float)Memory->Documents[0].Height;
		const float ortho_projection[4][4] =
		{
			{ 2.0f/width,	0.0f,			0.0f,		0.0f },
			{ 0.0f,			2.0f/-height,	0.0f,		0.0f },
			{ 0.0f,			0.0f,			-1.0f,		0.0f },
			{ -1.0f,		1.0f,			0.0f,		1.0f },
		};
		glUseProgram(Memory->Shaders[PapayaShader_Brush].Handle);
		
		Vec2 CorrectedPos		= Memory->Mouse.UV     + (BrushSize % 2 == 0 ? Vec2() : Vec2(0.5f/width, 0.5f/height));
		Vec2 CorrectedLastPos	= Memory->Mouse.LastUV + (BrushSize % 2 == 0 ? Vec2() : Vec2(0.5f/width, 0.5f/height));

		glUniformMatrix4fv(Memory->Shaders[PapayaShader_Brush].Uniforms[0], 1, GL_FALSE, &ortho_projection[0][0]);
		glUniform1i(Memory->Shaders[PapayaShader_Brush].Uniforms[1], 0); // Texture uniform
		glUniform2f(Memory->Shaders[PapayaShader_Brush].Uniforms[2], CorrectedPos.x, CorrectedPos.y); // Pos uniform
		glUniform2f(Memory->Shaders[PapayaShader_Brush].Uniforms[3], CorrectedLastPos.x, CorrectedLastPos.y); // Lastpos uniform
		glUniform1f(Memory->Shaders[PapayaShader_Brush].Uniforms[4], (float)BrushSize);
		glUniform4f(Memory->Shaders[PapayaShader_Brush].Uniforms[5], BrushCol.r, BrushCol.g, BrushCol.b, BrushCol.a);

		glBindBuffer(GL_ARRAY_BUFFER, Memory->VertexBuffers[PapayaVertexBuffer_RenderToTexture].VboHandle);
		glBindVertexArray(Memory->VertexBuffers[PapayaVertexBuffer_RenderToTexture].VaoHandle);
		
		glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)Memory->Documents[0].TextureID);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		uint32 Temp = Memory->FboColorTexture;
		Memory->FboColorTexture = Memory->Documents[0].TextureID;
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, Memory->FboColorTexture, 0);
		Memory->Documents[0].TextureID = Temp;

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0, 0, (int)ImGui::GetIO().DisplaySize.x, (int)ImGui::GetIO().DisplaySize.y);

		Util::StopTime(TimerScope_CPU_BRESENHAM, DebugMemory);
	}
	if (!Memory->Mouse.IsDown[0] && Memory->Mouse.WasDown[0])
	{
		glBindTexture(GL_TEXTURE_2D, Memory->Documents[0].TextureID);
		glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, Memory->Documents[0].Texture);
	}

	/*if (Memory->Mouse.IsDown[1])
	{
		Util::StartTime(TimerScope_CPU_EFLA, DebugMemory);
		Vec2 MCurr = (Memory->Mouse.Pos - Memory->Documents[0].CanvasPosition) * (1.0f / Memory->Documents[0].CanvasZoom);
		PaintCircle((int32)MCurr.x, (int32)MCurr.y, BrushSize, 0xff0000ff, Memory);
		Util::StopTime(TimerScope_CPU_EFLA, DebugMemory);
		RefreshTexture(Memory);
	}*/

	//Util::DisplayTimes(DebugMemory);

	#pragma region Canvas zooming and panning
	{
		// Panning
		Memory->Documents[0].CanvasPosition += ImGui::GetMouseDragDelta(2);
		ImGui::ResetMouseDragDelta(2);

		// Zooming
		if (!ImGui::IsMouseDown(2) && ImGui::GetIO().MouseWheel)
		{
			float MinZoom = 0.01f, MaxZoom = 32.0f;
			float ZoomSpeed = 0.2f * Memory->Documents[0].CanvasZoom;
			float ScaleDelta = min(MaxZoom - Memory->Documents[0].CanvasZoom, ImGui::GetIO().MouseWheel * ZoomSpeed);
			Vec2 OldCanvasZoom = Vec2((float)Memory->Documents[0].Width, (float)Memory->Documents[0].Height) * Memory->Documents[0].CanvasZoom;

			Memory->Documents[0].CanvasZoom += ScaleDelta;
			if (Memory->Documents[0].CanvasZoom < MinZoom) { Memory->Documents[0].CanvasZoom = MinZoom; } // TODO: Dynamically clamp min such that fully zoomed out image is 2x2 pixels?
			Vec2 NewCanvasSize = Vec2((float)Memory->Documents[0].Width, (float)Memory->Documents[0].Height) * Memory->Documents[0].CanvasZoom;
		
			if ((NewCanvasSize.x > Memory->Window.Width || NewCanvasSize.y > Memory->Window.Height))
			{
				Vec2 PreScaleMousePos = (Memory->Mouse.Pos - Memory->Documents[0].CanvasPosition) / OldCanvasZoom;
				Memory->Documents[0].CanvasPosition -= Vec2(PreScaleMousePos.x * ScaleDelta * (float)Memory->Documents[0].Width, PreScaleMousePos.y * ScaleDelta * (float)Memory->Documents[0].Height);
			}
			else // TODO: Maybe disable centering on zoom out. Needs more usability testing.
			{
				Vec2 WindowSize = Vec2((float)Memory->Window.Width, (float)Memory->Window.Height);
				Memory->Documents[0].CanvasPosition = (WindowSize - NewCanvasSize) * 0.5f;
			}
		}
	}
	#pragma endregion

	#pragma region Render canvas
	{
		// Setup render state: alpha-blending enabled, no face culling, no depth testing, scissor enabled
		GLint last_program, last_texture;
		glGetIntegerv(GL_CURRENT_PROGRAM, &last_program);
		glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
		glEnable(GL_BLEND);
		glBlendEquation(GL_FUNC_ADD);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDisable(GL_CULL_FACE);
		glDisable(GL_DEPTH_TEST);
		glEnable(GL_SCISSOR_TEST);
		glActiveTexture(GL_TEXTURE0);
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (Memory->Documents[0].CanvasZoom >= 2.0f) ? GL_NEAREST : GL_NEAREST);// : GL_LINEAR);

		// Setup orthographic projection matrix
		const float width = ImGui::GetIO().DisplaySize.x;
		const float height = ImGui::GetIO().DisplaySize.y;
		const float ortho_projection[4][4] =
		{
			{ 2.0f/width,	0.0f,			0.0f,		0.0f },
			{ 0.0f,			2.0f/-height,	0.0f,		0.0f },
			{ 0.0f,			0.0f,			-1.0f,		0.0f },
			{ -1.0f,		1.0f,			0.0f,		1.0f },
		};
		glUseProgram(Memory->Shaders[PapayaShader_ImGui].Handle);

		glUniformMatrix4fv(Memory->Shaders[PapayaShader_ImGui].Uniforms[0], 1, GL_FALSE, &ortho_projection[0][0]); // Projection matrix uniform
		glUniform1i(Memory->Shaders[PapayaShader_ImGui].Uniforms[1], 0); // Texture uniform

		// Grow our buffer according to what we need
		glBindBuffer(GL_ARRAY_BUFFER, Memory->VertexBuffers[PapayaVertexBuffer_ImGui].VboHandle);
		size_t needed_vtx_size = 6 * sizeof(ImDrawVert);
		if (Memory->VertexBuffers[PapayaVertexBuffer_ImGui].VboSize < needed_vtx_size)
		{
			Memory->VertexBuffers[PapayaVertexBuffer_ImGui].VboSize = needed_vtx_size + 5000 * sizeof(ImDrawVert);  // Grow buffer
			glBufferData(GL_ARRAY_BUFFER, Memory->VertexBuffers[PapayaVertexBuffer_ImGui].VboSize, NULL, GL_STREAM_DRAW);
		}

		// Copy and convert all vertices into a single contiguous buffer
		Vec2 Position = Memory->Documents[0].CanvasPosition;
		Vec2 Size = Vec2(Memory->Documents[0].Width * Memory->Documents[0].CanvasZoom, Memory->Documents[0].Height * Memory->Documents[0].CanvasZoom);
		ImDrawVert Verts[6];
		Verts[0].pos = Vec2(Position.x, Position.y);					Verts[0].uv = Vec2(0.0f, 0.0f); Verts[0].col = 0xffffffff;
		Verts[1].pos = Vec2(Size.x + Position.x, Position.y);			Verts[1].uv = Vec2(1.0f, 0.0f); Verts[1].col = 0xffffffff;
		Verts[2].pos = Vec2(Size.x + Position.x, Size.y + Position.y);	Verts[2].uv = Vec2(1.0f, 1.0f); Verts[2].col = 0xffffffff;
		Verts[3].pos = Vec2(Position.x, Position.y);					Verts[3].uv = Vec2(0.0f, 0.0f); Verts[3].col = 0xffffffff;
		Verts[4].pos = Vec2(Size.x + Position.x, Size.y + Position.y);	Verts[4].uv = Vec2(1.0f, 1.0f); Verts[4].col = 0xffffffff;
		Verts[5].pos = Vec2(Position.x, Size.y + Position.y);			Verts[5].uv = Vec2(0.0f, 1.0f); Verts[5].col = 0xffffffff;

		unsigned char* buffer_data = (unsigned char*)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
		memcpy(buffer_data, Verts, 6 * sizeof(ImDrawVert)); //TODO: Profile this.
		buffer_data += 6 * sizeof(ImDrawVert);
		glUnmapBuffer(GL_ARRAY_BUFFER);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(Memory->VertexBuffers[PapayaVertexBuffer_ImGui].VaoHandle);

		glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)Memory->Documents[0].TextureID);
		glScissor(34 + (int)Memory->Window.MaximizeOffset, 
				  1 + (int)Memory->Window.MaximizeOffset, 
				  (int)Memory->Window.Width - 68 - (2 * (int)Memory->Window.MaximizeOffset), 
				  (int)Memory->Window.Height - 34 - (2 * (int)Memory->Window.MaximizeOffset));
		glDrawArrays(GL_TRIANGLES, 0, 6);


		// Restore modified state
		glBindVertexArray(0);
		glUseProgram(last_program);
		glDisable(GL_SCISSOR_TEST);
		glBindTexture(GL_TEXTURE_2D, last_texture);
	}
	#pragma endregion

	#pragma region Left toolbar
	{
		ImGui::SetNextWindowSize(ImVec2(36,650));
		ImGui::SetNextWindowPos(ImVec2(1.0f + Memory->Window.MaximizeOffset, 70.0f + Memory->Window.MaximizeOffset));
			
		ImGuiWindowFlags WindowFlags = 0;
		WindowFlags |= ImGuiWindowFlags_NoTitleBar;
		WindowFlags |= ImGuiWindowFlags_NoResize;
		WindowFlags |= ImGuiWindowFlags_NoMove;
		WindowFlags |= ImGuiWindowFlags_NoScrollbar;
		WindowFlags |= ImGuiWindowFlags_NoCollapse;
		WindowFlags |= ImGuiWindowFlags_NoScrollWithMouse;

		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0,0));
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0,0));
		ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(0,0));
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0,0));

		ImGui::PushStyleColor(ImGuiCol_Button, Memory->InterfaceColors[PapayaInterfaceColor_Transparent]);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Memory->InterfaceColors[PapayaInterfaceColor_ButtonHover]);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, Memory->InterfaceColors[PapayaInterfaceColor_ButtonActive]);
		ImGui::PushStyleColor(ImGuiCol_WindowBg, Memory->InterfaceColors[PapayaInterfaceColor_Transparent]);

		bool Show = true;
		ImGui::Begin("Left toolbar", &Show, WindowFlags);

		ImGui::PushID(0);
		#define CALCUV(X, Y) ImVec2((float)X*20.0f/256.0f, (float)Y*20.0f/256.0f)
		if(ImGui::ImageButton((void*)Memory->InterfaceTextureIDs[PapayaInterfaceTexture_InterfaceIcons], ImVec2(20,20), CALCUV(0,0), CALCUV(1,1), 6, ImVec4(0,0,0,0)))
		{
			
		}
		#undef CALCUV
		ImGui::PopID();

		ImGui::End();

		ImGui::PopStyleVar(5);
		ImGui::PopStyleColor(4);
	}
	#pragma endregion

	#pragma region Last mouse info
	{
		Memory->Mouse.LastPos = ImGui::GetMousePos();
		Memory->Mouse.LastUV = Memory->Mouse.UV;
		Memory->Mouse.WasDown[0] = ImGui::IsMouseDown(0);
		Memory->Mouse.WasDown[1] = ImGui::IsMouseDown(1);
		Memory->Mouse.WasDown[2] = ImGui::IsMouseDown(2);
	}
	#pragma endregion
}
