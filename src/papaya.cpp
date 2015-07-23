#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

internal uint32 AllocateEmptyTexture(int32 Width, int32 Height)
{
	uint32 Tex;
	glGenTextures(1, &Tex);
	glBindTexture(GL_TEXTURE_2D, Tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, Width, Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	return Tex;
}

internal uint32 LoadAndBindImage(char* Path)
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
	return (uint32)Id_GLuint;
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
	#pragma region Set up the frame buffer
	{
		// Create a framebuffer object and bind it
		glGenFramebuffers(1, &Memory->FrameBufferObject);
		glBindFramebuffer(GL_FRAMEBUFFER, Memory->FrameBufferObject);
		
		Memory->FboRenderTexture = AllocateEmptyTexture(4096, 4096);
		Memory->FboSampleTexture = AllocateEmptyTexture(4096, 4096);

		// Attach the color texture to the FBO
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, Memory->FboRenderTexture, 0);

		static const GLenum draw_buffers[] = { GL_COLOR_ATTACHMENT0 };
		glDrawBuffers(1, draw_buffers);

		if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		{
			// TODO: Log: Frame buffer not initialized correctly
			exit(1);
		}
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
	#pragma endregion

	#pragma region Set up brush shader
	{
		const GLchar *vertex_shader = 
"				#version 330																						\n"
"				uniform mat4 ProjMtx;																				\n" // Uniforms[0]
"																													\n"
"				in vec2 Position;																					\n" // Attributes[0]
"				in vec2 UV;																							\n" // Attributes[1]
"				in vec4 Color;																						\n" // Attributes[2]
"																													\n"
"				out vec2 Frag_UV;																					\n"
"				void main()																							\n"
"				{																									\n"
"					Frag_UV = UV;																					\n"
"					gl_Position = ProjMtx * vec4(Position.xy,0,1);													\n"
"				}																									\n";

		const GLchar* fragment_shader = 
"				#version 330																						\n"
"				uniform sampler2D	Texture;																		\n" // Uniforms[1]
"				uniform vec2		Pos;																			\n" // Uniforms[2]
"				uniform vec2		LastPos;																		\n" // Uniforms[3]
"				uniform float		Thickness;																		\n" // Uniforms[4]
"				uniform vec4		BrushColor;																		\n" // Uniforms[5]
"				uniform float		Opacity;																		\n" // Uniforms[6]
"				uniform float		Hardness;																		\n" // Uniforms[7]
"																													\n"
"				in vec2 Frag_UV;																					\n"
"				out vec4 Out_Color;																					\n"
"																													\n"
"				void line(vec2 p1, vec2 p2, vec2 uv, float thickness, out float distanceFromLine)					\n"
"				{																									\n"
"					if (distance(p1,p2) <= 0.0)																		\n"
"					{																								\n"
"						distanceFromLine = distance(uv, p1);														\n"
"						return;																						\n"
"					}																								\n"
"																													\n"
"					float a = abs(distance(p1, uv));																\n"
"					float b = abs(distance(p2, uv));																\n"
"					float c = abs(distance(p1, p2));																\n"
"					float d = sqrt(c*c + thickness*thickness);														\n"
"																													\n"
"					vec2 a1 = normalize(uv - p1);																	\n"
"					vec2 b1 = normalize(uv - p2);																	\n"
"					vec2 c1 = normalize(p2 - p1);																	\n"
"					if (dot(a1,c1) < 0.0)																			\n"
"					{																								\n"
"						distanceFromLine = a;																		\n"
"						return;																						\n"
"					}																								\n"
"																													\n"
"					if (dot(b1,c1) > 0.0)																			\n"
"					{																								\n"
"						distanceFromLine = b;																		\n"
"						return;																						\n"
"					}																								\n"
"																													\n"
"					float p = (a + b + c) * 0.5;																	\n"
"					float h = 2.0 / c * sqrt( p * (p-a) * (p-b) * (p-c) );											\n"
"																													\n"
"					if (isnan(h))																					\n"
"						distanceFromLine = 0.0;																		\n"
"					else																							\n"
"						distanceFromLine = h;																		\n"
"				}																									\n"
"																													\n"
"				void main()																							\n"
"				{																									\n"
"					vec4 TextureColor = texture(Texture, Frag_UV.st);												\n"
"																													\n"
"					float distanceFromLine;																			\n"
"					line(LastPos, Pos, Frag_UV, Thickness, distanceFromLine);										\n"
"					float delta = fwidth(distanceFromLine) * 2.0;													\n"
"					float alpha = smoothstep(Thickness-delta, Thickness, distanceFromLine);							\n"
"					alpha = 1.0 - alpha;																			\n"
"					if (alpha > 0.0) alpha = Opacity;																\n"
"																													\n"
"					//Out_Color = vec4(0.0,1.0,0.0,0.5);															\n"
"					//Out_Color = vec4(BrushColor.r, BrushColor.g, BrushColor.b, 0.5);								\n"
"					Out_Color = vec4(BrushColor.r, BrushColor.g, BrushColor.b, max(TextureColor.a,alpha));			\n"
"					//Out_Color = mix(TextureColor, BrushColor, 0.5);												\n"
"				}																									\n";

		Memory->Shaders[PapayaShader_Brush].Handle = glCreateProgram();
		uint32 g_VertHandle = glCreateShader(GL_VERTEX_SHADER);
		uint32 g_FragHandle = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(g_VertHandle, 1, &vertex_shader, 0);
		glShaderSource(g_FragHandle, 1, &fragment_shader, 0);
		glCompileShader(g_VertHandle);
		glCompileShader(g_FragHandle);
		glAttachShader(Memory->Shaders[PapayaShader_Brush].Handle, g_VertHandle);
		glAttachShader(Memory->Shaders[PapayaShader_Brush].Handle, g_FragHandle);
		Util::PrintGlShaderCompilationStatus(g_FragHandle);
		glLinkProgram (Memory->Shaders[PapayaShader_Brush].Handle);

		Memory->Shaders[PapayaShader_Brush].Attributes[0] = glGetAttribLocation (Memory->Shaders[PapayaShader_Brush].Handle, "Position");
		Memory->Shaders[PapayaShader_Brush].Attributes[1] = glGetAttribLocation (Memory->Shaders[PapayaShader_Brush].Handle, "UV");
		Memory->Shaders[PapayaShader_Brush].Attributes[2] = glGetAttribLocation (Memory->Shaders[PapayaShader_Brush].Handle, "Color");

		Memory->Shaders[PapayaShader_Brush].Uniforms[0]   = glGetUniformLocation(Memory->Shaders[PapayaShader_Brush].Handle, "ProjMtx");
		Memory->Shaders[PapayaShader_Brush].Uniforms[1]   = glGetUniformLocation(Memory->Shaders[PapayaShader_Brush].Handle, "Texture");
		Memory->Shaders[PapayaShader_Brush].Uniforms[2]   = glGetUniformLocation(Memory->Shaders[PapayaShader_Brush].Handle, "Pos");
		Memory->Shaders[PapayaShader_Brush].Uniforms[3]   = glGetUniformLocation(Memory->Shaders[PapayaShader_Brush].Handle, "LastPos");
		Memory->Shaders[PapayaShader_Brush].Uniforms[4]   = glGetUniformLocation(Memory->Shaders[PapayaShader_Brush].Handle, "Thickness");
		Memory->Shaders[PapayaShader_Brush].Uniforms[5]   = glGetUniformLocation(Memory->Shaders[PapayaShader_Brush].Handle, "BrushColor");
		Memory->Shaders[PapayaShader_Brush].Uniforms[6]   = glGetUniformLocation(Memory->Shaders[PapayaShader_Brush].Handle, "Opacity");
		Memory->Shaders[PapayaShader_Brush].Uniforms[7]   = glGetUniformLocation(Memory->Shaders[PapayaShader_Brush].Handle, "Hardness");
	}
	#pragma endregion

	#pragma region Set up brush cursor shader
	{
		const GLchar *vertex_shader = 
"				#version 330																						\n"
"				uniform mat4 ProjMtx;																				\n" // Uniforms[0]
"																													\n"
"				in vec2 Position;																					\n" // Attributes[0]
"				in vec2 UV;																							\n" // Attributes[1]
"																													\n"
"				out vec2 Frag_UV;																					\n"
"																													\n"
"				void main()																							\n"
"				{																									\n"
"					Frag_UV = UV;																					\n"
"					gl_Position = ProjMtx * vec4(Position.xy,0,1);													\n"
"				}																									\n";

		const GLchar* fragment_shader = 
"				#version 330																						\n"
"																													\n"
"				#define M_PI 3.1415926535897932384626433832795														\n"
"																													\n"
"				uniform vec4		BrushColor;																		\n" // Uniforms[1]
"				uniform float		Hardness;																		\n" // Uniforms[2]
"																													\n"
"				in vec2 Frag_UV;																					\n"
"																													\n"
"				out vec4 Out_Color;																					\n"
"																													\n"
"				void main()																							\n"
"				{																									\n"
"					float Scale = 1.0 / (1.0 - Hardness);															\n"
"					float Period = M_PI * Scale;																	\n"
"					float Phase = (1.0 - Scale) * M_PI * 0.5;														\n"
"					float Dist = distance(Frag_UV,vec2(0.5,0.5));													\n"
"					float Alpha = cos((Period * Dist) + Phase);														\n"
"					if (Dist < 0.5 - (0.5/Scale)) Alpha = 1.0;														\n"
"					else if (Dist > 0.5)		  Alpha = 0.0;														\n"
"					Out_Color = vec4(BrushColor.r, BrushColor.g, BrushColor.b, 	Alpha * BrushColor.a);				\n"
"				}					 																				\n";

		Memory->Shaders[PapayaShader_BrushCursor].Handle = glCreateProgram();
		uint32 g_VertHandle = glCreateShader(GL_VERTEX_SHADER);
		uint32 g_FragHandle = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(g_VertHandle, 1, &vertex_shader, 0);
		glShaderSource(g_FragHandle, 1, &fragment_shader, 0);
		glCompileShader(g_VertHandle);
		glCompileShader(g_FragHandle);
		glAttachShader(Memory->Shaders[PapayaShader_BrushCursor].Handle, g_VertHandle);
		glAttachShader(Memory->Shaders[PapayaShader_BrushCursor].Handle, g_FragHandle);
		Util::PrintGlShaderCompilationStatus(g_VertHandle);
		Util::PrintGlShaderCompilationStatus(g_FragHandle);
		glLinkProgram (Memory->Shaders[PapayaShader_BrushCursor].Handle);

		Memory->Shaders[PapayaShader_BrushCursor].Attributes[0] = glGetAttribLocation (Memory->Shaders[PapayaShader_BrushCursor].Handle, "Position");
		Memory->Shaders[PapayaShader_BrushCursor].Attributes[1] = glGetAttribLocation (Memory->Shaders[PapayaShader_BrushCursor].Handle, "UV");

		Memory->Shaders[PapayaShader_BrushCursor].Uniforms[0]   = glGetUniformLocation(Memory->Shaders[PapayaShader_BrushCursor].Handle, "ProjMtx");
		Memory->Shaders[PapayaShader_BrushCursor].Uniforms[1]   = glGetUniformLocation(Memory->Shaders[PapayaShader_BrushCursor].Handle, "BrushColor");
		Memory->Shaders[PapayaShader_BrushCursor].Uniforms[2]   = glGetUniformLocation(Memory->Shaders[PapayaShader_BrushCursor].Handle, "Hardness");

		// Vertex buffer
		glGenBuffers(1, &Memory->VertexBuffers[PapayaVertexBuffer_BrushCursor].VboHandle);
		glBindBuffer(GL_ARRAY_BUFFER, Memory->VertexBuffers[PapayaVertexBuffer_BrushCursor].VboHandle);
		glGenVertexArrays(1, &Memory->VertexBuffers[PapayaVertexBuffer_BrushCursor].VaoHandle);
		glBindVertexArray(Memory->VertexBuffers[PapayaVertexBuffer_BrushCursor].VaoHandle);
		glEnableVertexAttribArray(Memory->Shaders[PapayaShader_BrushCursor].Attributes[0]); // Position attribute
		glEnableVertexAttribArray(Memory->Shaders[PapayaShader_BrushCursor].Attributes[1]); // Position attribute
#define OFFSETOF(TYPE, ELEMENT) ((size_t)&(((TYPE *)0)->ELEMENT))
		glVertexAttribPointer(Memory->Shaders[PapayaShader_BrushCursor].Attributes[0], 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)OFFSETOF(ImDrawVert, pos));		 // Position attribute
		glVertexAttribPointer(Memory->Shaders[PapayaShader_BrushCursor].Attributes[1], 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)OFFSETOF(ImDrawVert, uv));			 // UV attribute
		//glVertexAttribPointer(Memory->Shaders[PapayaShader_Brush].Attributes[2], 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(ImDrawVert), (GLvoid*)OFFSETOF(ImDrawVert, col));  // Vertex color attribute
#undef OFFSETOF

		Vec2 Position = Vec2(40,60);
		Vec2 Size = Vec2(30,30);
		ImDrawVert Verts[6];
		Verts[0].pos = Vec2(Position.x, Position.y);					Verts[0].uv = Vec2(0.0f, 1.0f); Verts[0].col = 0xffffffff;
		Verts[1].pos = Vec2(Size.x + Position.x, Position.y);			Verts[1].uv = Vec2(1.0f, 1.0f); Verts[1].col = 0xffffffff;
		Verts[2].pos = Vec2(Size.x + Position.x, Size.y + Position.y);	Verts[2].uv = Vec2(1.0f, 0.0f); Verts[2].col = 0xffffffff;
		Verts[3].pos = Vec2(Position.x, Position.y);					Verts[3].uv = Vec2(0.0f, 1.0f); Verts[3].col = 0xffffffff;
		Verts[4].pos = Vec2(Size.x + Position.x, Size.y + Position.y);	Verts[4].uv = Vec2(1.0f, 0.0f); Verts[4].col = 0xffffffff;
		Verts[5].pos = Vec2(Position.x, Size.y + Position.y);			Verts[5].uv = Vec2(0.0f, 0.0f); Verts[5].col = 0xffffffff;
		glBufferData(GL_ARRAY_BUFFER, sizeof(Verts), Verts, GL_DYNAMIC_DRAW);
	}
	#pragma endregion

	#pragma region Setup for ImGui
	{
		// TODO: Write shader compilation wrapper
		const GLchar *vertex_shader = 
"				#version 330                                                      \n"
"				uniform mat4 ProjMtx;                                             \n" // Uniforms[0]
"																				  \n"
"				in vec2 Position;                                                 \n" // Attributes[0]
"				in vec2 UV;                                                       \n" // Attributes[1]
"				in vec4 Color;                                                    \n" // Attributes[2]
"																				  \n"
"				out vec2 Frag_UV;                                                 \n"
"				out vec4 Frag_Color;                                              \n"
"				void main()                                                       \n"
"				{                                                                 \n"
"					Frag_UV = UV;                                                 \n"
"					Frag_Color = Color;                                           \n"
"					gl_Position = ProjMtx * vec4(Position.xy,0,1);                \n"
"				}                                                                 \n";

		const GLchar* fragment_shader = 
"				#version 330													  \n"
"				uniform sampler2D Texture;										  \n" // Uniforms[1]
"																				  \n"
"				in vec2 Frag_UV;												  \n"
"				in vec4 Frag_Color;												  \n"
"																				  \n"
"				out vec4 Out_Color;												  \n"
"				void main()														  \n"
"				{																  \n"
"					Out_Color = Frag_Color * texture( Texture, Frag_UV.st);		  \n"
"				}																  \n";

		Memory->Shaders[PapayaShader_ImGui].Handle = glCreateProgram();
		int32 g_VertHandle = glCreateShader(GL_VERTEX_SHADER);
		int32 g_FragHandle = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(g_VertHandle, 1, &vertex_shader, 0);
		glShaderSource(g_FragHandle, 1, &fragment_shader, 0);
		glCompileShader(g_VertHandle);
		glCompileShader(g_FragHandle);
		glAttachShader(Memory->Shaders[PapayaShader_ImGui].Handle, g_VertHandle);
		glAttachShader(Memory->Shaders[PapayaShader_ImGui].Handle, g_FragHandle);
		glLinkProgram (Memory->Shaders[PapayaShader_ImGui].Handle);

		Memory->Shaders[PapayaShader_ImGui].Attributes[0] = glGetAttribLocation (Memory->Shaders[PapayaShader_ImGui].Handle, "Position");
		Memory->Shaders[PapayaShader_ImGui].Attributes[1] = glGetAttribLocation (Memory->Shaders[PapayaShader_ImGui].Handle, "UV");
		Memory->Shaders[PapayaShader_ImGui].Attributes[2] = glGetAttribLocation (Memory->Shaders[PapayaShader_ImGui].Handle, "Color");

		Memory->Shaders[PapayaShader_ImGui].Uniforms[0]	 = glGetUniformLocation(Memory->Shaders[PapayaShader_ImGui].Handle, "ProjMtx");
		Memory->Shaders[PapayaShader_ImGui].Uniforms[1]	 = glGetUniformLocation(Memory->Shaders[PapayaShader_ImGui].Handle, "Texture");

		glGenBuffers(1, &Memory->VertexBuffers[PapayaVertexBuffer_ImGui].VboHandle);

		glGenVertexArrays(1, &Memory->VertexBuffers[PapayaVertexBuffer_ImGui].VaoHandle);
		glBindVertexArray(Memory->VertexBuffers[PapayaVertexBuffer_ImGui].VaoHandle);
		glBindBuffer(GL_ARRAY_BUFFER, Memory->VertexBuffers[PapayaVertexBuffer_ImGui].VboHandle);
		glEnableVertexAttribArray(Memory->Shaders[PapayaShader_ImGui].Attributes[0]);
		glEnableVertexAttribArray(Memory->Shaders[PapayaShader_ImGui].Attributes[1]);
		glEnableVertexAttribArray(Memory->Shaders[PapayaShader_ImGui].Attributes[2]);

	#define OFFSETOF(TYPE, ELEMENT) ((size_t)&(((TYPE *)0)->ELEMENT))
		glVertexAttribPointer(Memory->Shaders[PapayaShader_ImGui].Attributes[0], 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)OFFSETOF(ImDrawVert, pos));
		glVertexAttribPointer(Memory->Shaders[PapayaShader_ImGui].Attributes[1], 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)OFFSETOF(ImDrawVert, uv));
		glVertexAttribPointer(Memory->Shaders[PapayaShader_ImGui].Attributes[2], 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(ImDrawVert), (GLvoid*)OFFSETOF(ImDrawVert, col));
	#undef OFFSETOF
		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		// Create fonts texture
		ImGuiIO& io = ImGui::GetIO();

		unsigned char* pixels;
		int width, height;
		//ImFont* my_font0 = io.Fonts->AddFontFromFileTTF("d:\\DroidSans.ttf", 16.0f);
		io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

		glGenTextures(1, &Memory->InterfaceTextureIDs[PapayaInterfaceTexture_Font]);
		glBindTexture(GL_TEXTURE_2D, Memory->InterfaceTextureIDs[PapayaInterfaceTexture_Font]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

		// Store our identifier
		io.Fonts->TexID = (void *)(intptr_t)Memory->InterfaceTextureIDs[PapayaInterfaceTexture_Font];

		// Cleanup
		io.Fonts->ClearInputData();
		io.Fonts->ClearTexData();
	}
	#pragma endregion

	#pragma region Load interface textures and colors
	{
		// Texture IDs
		Memory->InterfaceTextureIDs[PapayaInterfaceTexture_TitleBarButtons] = LoadAndBindImage("../../img/win32_titlebar_buttons.png");
		Memory->InterfaceTextureIDs[PapayaInterfaceTexture_TitleBarIcon] = LoadAndBindImage("../../img/win32_titlebar_icon.png");
		Memory->InterfaceTextureIDs[PapayaInterfaceTexture_InterfaceIcons] = LoadAndBindImage("../../img/interface_icons.png");

		// Colors
#if 1
		Memory->InterfaceColors[PapayaInterfaceColor_Clear]			= Color(45,45,48); // Dark grey
#else
		Memory->InterfaceColors[PapayaInterfaceColor_Clear]			= Color(114,144,154); // Light blue
#endif
		Memory->InterfaceColors[PapayaInterfaceColor_Workspace]		= Color(30,30,30); // Light blue
		Memory->InterfaceColors[PapayaInterfaceColor_Transparent]	= Color(0,0,0,0);
		Memory->InterfaceColors[PapayaInterfaceColor_Button]		= Color(92,92,94);
		Memory->InterfaceColors[PapayaInterfaceColor_ButtonHover]	= Color(64,64,64);
		Memory->InterfaceColors[PapayaInterfaceColor_ButtonActive]	= Color(0,122,204);
	}
	#pragma endregion

	#pragma region Load document
	{
		Memory->Documents = (PapayaDocument*)malloc(sizeof(PapayaDocument));
		//LoadImageIntoDocument("C:\\Users\\Apoorva\\Pictures\\ImageTest\\fruits.png", Memory->Documents);
		LoadImageIntoDocument("C:\\Users\\Apoorva\\Pictures\\ImageTest\\test4k.jpg", Memory->Documents);

		Memory->Documents[0].CanvasZoom = 0.8f * Math::Min((float)Memory->Window.Width/(float)Memory->Documents[0].Width, (float)Memory->Window.Height/(float)Memory->Documents[0].Height);
		if (Memory->Documents[0].CanvasZoom > 1.0f) { Memory->Documents[0].CanvasZoom = 1.0f; }
		Memory->Documents[0].CanvasPosition = Vec2((Memory->Window.Width  - (float)Memory->Documents[0].Width  * Memory->Documents[0].CanvasZoom)/2.0f, 
												   (Memory->Window.Height - (float)Memory->Documents[0].Height * Memory->Documents[0].CanvasZoom)/2.0f); // TODO: Center with respect to canvas, not window
	}
	#pragma endregion

	#pragma region Set up vertex buffers
	{
		// TODO: Needs a lot of refactoring
		glGenBuffers(1, &Memory->VertexBuffers[PapayaVertexBuffer_RTTBrush].VboHandle);
		glBindBuffer(GL_ARRAY_BUFFER, Memory->VertexBuffers[PapayaVertexBuffer_RTTBrush].VboHandle);
		glGenVertexArrays(1, &Memory->VertexBuffers[PapayaVertexBuffer_RTTBrush].VaoHandle);
		glBindVertexArray(Memory->VertexBuffers[PapayaVertexBuffer_RTTBrush].VaoHandle);
		glEnableVertexAttribArray(Memory->Shaders[PapayaShader_Brush].Attributes[0]); // Position attribute
		glEnableVertexAttribArray(Memory->Shaders[PapayaShader_Brush].Attributes[1]); // UV attribute
		//glEnableVertexAttribArray(Memory->Shaders[PapayaShader_Brush].Attributes[2]); // Vertex color attribute // TODO: Remove unused vertex color attributes from shaders
	
	#define OFFSETOF(TYPE, ELEMENT) ((size_t)&(((TYPE *)0)->ELEMENT))
		glVertexAttribPointer(Memory->Shaders[PapayaShader_Brush].Attributes[0], 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)OFFSETOF(ImDrawVert, pos));		 // Position attribute
		glVertexAttribPointer(Memory->Shaders[PapayaShader_Brush].Attributes[1], 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)OFFSETOF(ImDrawVert, uv));			 // UV attribute
		//glVertexAttribPointer(Memory->Shaders[PapayaShader_Brush].Attributes[2], 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(ImDrawVert), (GLvoid*)OFFSETOF(ImDrawVert, col));  // Vertex color attribute
	#undef OFFSETOF
		Vec2 Position = Vec2(0,0);
		Vec2 Size = Vec2((float)Memory->Documents[0].Width, (float)Memory->Documents[0].Height);
		ImDrawVert Verts[6];
		Verts[0].pos = Vec2(Position.x, Position.y);					Verts[0].uv = Vec2(0.0f, 1.0f);		Verts[0].col = 0xffffffff;
		Verts[1].pos = Vec2(Size.x + Position.x, Position.y);			Verts[1].uv = Vec2(1.0f, 1.0f);		Verts[1].col = 0xffffffff;
		Verts[2].pos = Vec2(Size.x + Position.x, Size.y + Position.y);	Verts[2].uv = Vec2(1.0f, 0.0f);		Verts[2].col = 0xffffffff;
		Verts[3].pos = Vec2(Position.x, Position.y);					Verts[3].uv = Vec2(0.0f, 1.0f);		Verts[3].col = 0xffffffff;
		Verts[4].pos = Vec2(Size.x + Position.x, Size.y + Position.y);	Verts[4].uv = Vec2(1.0f, 0.0f);		Verts[4].col = 0xffffffff;
		Verts[5].pos = Vec2(Position.x, Size.y + Position.y);			Verts[5].uv = Vec2(0.0f, 0.0f);		Verts[5].col = 0xffffffff;
		glBufferData(GL_ARRAY_BUFFER, sizeof(Verts), Verts, GL_STATIC_DRAW);

		glGenBuffers(1, &Memory->VertexBuffers[PapayaVertexBuffer_RTTAdd].VboHandle);
		glBindBuffer(GL_ARRAY_BUFFER, Memory->VertexBuffers[PapayaVertexBuffer_RTTAdd].VboHandle);
		glGenVertexArrays(1, &Memory->VertexBuffers[PapayaVertexBuffer_RTTAdd].VaoHandle);
		glBindVertexArray(Memory->VertexBuffers[PapayaVertexBuffer_RTTAdd].VaoHandle);
		glEnableVertexAttribArray(Memory->Shaders[PapayaShader_ImGui].Attributes[0]); // Position attribute
		glEnableVertexAttribArray(Memory->Shaders[PapayaShader_ImGui].Attributes[1]); // UV attribute
		glEnableVertexAttribArray(Memory->Shaders[PapayaShader_ImGui].Attributes[2]); // Vertex color attribute
	#define OFFSETOF(TYPE, ELEMENT) ((size_t)&(((TYPE *)0)->ELEMENT))
		glVertexAttribPointer(Memory->Shaders[PapayaShader_ImGui].Attributes[0], 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)OFFSETOF(ImDrawVert, pos));		 // Position attribute
		glVertexAttribPointer(Memory->Shaders[PapayaShader_ImGui].Attributes[1], 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)OFFSETOF(ImDrawVert, uv));			 // UV attribute
		glVertexAttribPointer(Memory->Shaders[PapayaShader_ImGui].Attributes[2], 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(ImDrawVert), (GLvoid*)OFFSETOF(ImDrawVert, col));  // Vertex color attribute
	#undef OFFSETOF
		glBufferData(GL_ARRAY_BUFFER, sizeof(Verts), Verts, GL_STATIC_DRAW);
		//glBindVertexArray(0);
		//glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
	#pragma endregion

	Memory->Tools.BrushDiameter = 256;
	Memory->Tools.BrushOpacity = 100.0f;
	Memory->Tools.BrushHardness = 0.0f;
	Memory->DrawCanvas = true;
	Memory->DrawOverlay = false;
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
		Memory->Mouse.Pos  = ImGui::GetMousePos();
		Vec2 MousePixelPos = Vec2(Math::Floor((Memory->Mouse.Pos.x - Memory->Documents[0].CanvasPosition.x) / Memory->Documents[0].CanvasZoom),
								  Math::Floor((Memory->Mouse.Pos.y - Memory->Documents[0].CanvasPosition.y) / Memory->Documents[0].CanvasZoom));
		Memory->Mouse.UV   = Vec2(MousePixelPos.x / (float) Memory->Documents[0].Width,
								  MousePixelPos.y / (float) Memory->Documents[0].Height);

		for (int32 i = 0; i < 3; i++)
		{
			Memory->Mouse.IsDown[i]   = ImGui::IsMouseDown(i);
			Memory->Mouse.Pressed[i]  = (Memory->Mouse.IsDown[i] && !Memory->Mouse.WasDown[i]);
			Memory->Mouse.Released[i] = (!Memory->Mouse.IsDown[i] && Memory->Mouse.WasDown[i]);
		}
	}
	#pragma endregion

    glViewport(0, 0, (int)ImGui::GetIO().DisplaySize.x, (int)ImGui::GetIO().DisplaySize.y);
	glClearBufferfv(GL_COLOR, 0, (GLfloat*)&Memory->InterfaceColors[PapayaInterfaceColor_Clear]);

	#pragma region Tool Param Bar
	{
		ImGui::SetNextWindowSize(ImVec2((float)Memory->Window.Width - 37, 30));
		ImGui::SetNextWindowPos(ImVec2(34, 30));
			
		ImGuiWindowFlags WindowFlags = 0;
		WindowFlags |= ImGuiWindowFlags_NoTitleBar;
		WindowFlags |= ImGuiWindowFlags_NoResize;
		WindowFlags |= ImGuiWindowFlags_NoMove;
		WindowFlags |= ImGuiWindowFlags_NoScrollbar;
		WindowFlags |= ImGuiWindowFlags_NoCollapse;
		WindowFlags |= ImGuiWindowFlags_NoScrollWithMouse; // TODO: Once the overall look has been established, make commonly used templates

		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0,0));
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(30,0));

		ImGui::PushStyleColor(ImGuiCol_Button, Memory->InterfaceColors[PapayaInterfaceColor_Button]);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Memory->InterfaceColors[PapayaInterfaceColor_ButtonHover]);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, Memory->InterfaceColors[PapayaInterfaceColor_ButtonActive]);
		ImGui::PushStyleColor(ImGuiCol_WindowBg, Memory->InterfaceColors[PapayaInterfaceColor_Transparent]);
		ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, Memory->InterfaceColors[PapayaInterfaceColor_ButtonActive]);

		bool Show = true;
		ImGui::Begin("Tool param bar", &Show, WindowFlags);
		ImGui::PushItemWidth(85);

		static float Hardness = 100.0f;
		static float Flow = 100.0f;
		ImGui::InputInt("Diameter", &Memory->Tools.BrushDiameter);
		Memory->Tools.BrushDiameter = Math::Clamp(Memory->Tools.BrushDiameter, 1, Memory->Tools.MaxBrushDiameter);
		
		ImGui::PopItemWidth();
		ImGui::PushItemWidth(80);
		ImGui::SameLine();
		ImGui::SliderFloat("Hardness", &Memory->Tools.BrushHardness, 0.0f, 100.0f, "%.0f");
		ImGui::SameLine();
		ImGui::SliderFloat("Opacity", &Memory->Tools.BrushOpacity, 0.0f, 100.0f, "%.0f");
		ImGui::SameLine();
		ImGui::SliderFloat("Flow", &Flow, 0.0f, 100.0f, "%.0f");

		ImGui::PopItemWidth();
		ImGui::End();

		ImGui::PopStyleVar(3);
		ImGui::PopStyleColor(5);
	}
	#pragma endregion

	#pragma region Brush tool
	{
		local_persist Color BrushCol = Color(0.0f,1.0f,0.0f);

		if (ImGui::IsKeyPressed(VK_UP, false))
		{
			Memory->Tools.BrushDiameter++;
			Memory->DrawCanvas = !Memory->DrawCanvas;
		}
		if (ImGui::IsKeyPressed(VK_DOWN, false))
		{
			Memory->Tools.BrushDiameter--;
			Memory->DrawOverlay = !Memory->DrawOverlay;
		}
		if (ImGui::IsKeyPressed(VK_NUMPAD1, false))
		{
			Memory->Documents[0].CanvasZoom = 1.0;
		}

		#pragma region Right mouse dragging
		{
			if (Memory->Mouse.Pressed[1])
			{
				Memory->Tools.RightClickDragStartPos = Memory->Mouse.Pos;
				Memory->Tools.RightClickDragStartDiameter = Memory->Tools.BrushDiameter;
				Memory->Tools.RightClickDragStartHardness = Memory->Tools.BrushHardness;
				Memory->Tools.RightClickDragStartOpacity = Memory->Tools.BrushOpacity;
				Memory->Tools.RightClickShiftPressed = ImGui::IsKeyDown(VK_SHIFT);
				Platform::StartMouseCapture();
				Platform::SetCursorVisibility(false);
			}
			else if (Memory->Mouse.IsDown[1])
			{
				if (Memory->Tools.RightClickShiftPressed)
				{
					float Opacity = Memory->Tools.RightClickDragStartOpacity + (ImGui::GetMouseDragDelta(1).x * 0.25f);
					Memory->Tools.BrushOpacity = Math::Clamp(Opacity, 0.0f, 100.0f);
				}
				else
				{
					float Diameter = Memory->Tools.RightClickDragStartDiameter + (ImGui::GetMouseDragDelta(1).x / Memory->Documents[0].CanvasZoom * 2.0f);
					Memory->Tools.BrushDiameter = Math::Clamp((int32)Diameter, 1, Memory->Tools.MaxBrushDiameter);

					float Hardness = Memory->Tools.RightClickDragStartHardness + (ImGui::GetMouseDragDelta(1).y * 0.25f);
					Memory->Tools.BrushHardness = Math::Clamp(Hardness, 0.0f, 100.0f);
				}
			}
			else if (Memory->Mouse.Released[1])
			{
				Platform::ReleaseMouseCapture();
				Platform::SetMousePosition(Memory->Tools.RightClickDragStartPos);
				Platform::SetCursorVisibility(true);
			}
		}
		#pragma endregion

		if (Memory->Mouse.IsDown[0])
		{
			Memory->DrawOverlay = true;
			BrushCol = (Memory->Mouse.IsDown[0]) ? Color(0.0f,1.0f,0.0f) : Color(1.0f,0.0f,0.0f);

			glBindFramebuffer(GL_FRAMEBUFFER, Memory->FrameBufferObject);

			if (Memory->Mouse.Pressed[0])
			{
				GLuint clearColor[4] = {0, 0, 0, 0};
				glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, Memory->FboSampleTexture, 0);
				glClearBufferuiv(GL_COLOR, 0, clearColor);
				glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, Memory->FboRenderTexture, 0);
			}
			glViewport(0, 0, Memory->Documents[0].Width, Memory->Documents[0].Height);

			glDisable(GL_BLEND);
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
		
			Vec2 CorrectedPos		= Memory->Mouse.UV     + (Memory->Tools.BrushDiameter % 2 == 0 ? Vec2() : Vec2(0.5f/width, 0.5f/height));
			Vec2 CorrectedLastPos	= Memory->Mouse.LastUV + (Memory->Tools.BrushDiameter % 2 == 0 ? Vec2() : Vec2(0.5f/width, 0.5f/height));

			/*Vec2 CorrectedPos		= Vec2(0.6f, 0.5f)     + (BrushSize % 2 == 0 ? Vec2() : Vec2(0.5f/width, 0.5f/height));
			Vec2 CorrectedLastPos	= Vec2(0.6f, 0.5f) + (BrushSize % 2 == 0 ? Vec2() : Vec2(0.5f/width, 0.5f/height));*/

			glUniformMatrix4fv(Memory->Shaders[PapayaShader_Brush].Uniforms[0], 1, GL_FALSE, &ortho_projection[0][0]);
			//glUniform1i(Memory->Shaders[PapayaShader_Brush].Uniforms[1], Memory->Documents[0].TextureID); // Texture uniform
			glUniform2f(Memory->Shaders[PapayaShader_Brush].Uniforms[2], CorrectedPos.x, CorrectedPos.y); // Pos uniform
			glUniform2f(Memory->Shaders[PapayaShader_Brush].Uniforms[3], CorrectedLastPos.x, CorrectedLastPos.y); // Lastpos uniform
			glUniform1f(Memory->Shaders[PapayaShader_Brush].Uniforms[4], (float)Memory->Tools.BrushDiameter / ((float)Memory->Documents[0].Width * 2.0f)); // TODO: Support non-square documents
			glUniform4f(Memory->Shaders[PapayaShader_Brush].Uniforms[5], BrushCol.r, BrushCol.g, BrushCol.b, BrushCol.a);
			glUniform1f(Memory->Shaders[PapayaShader_Brush].Uniforms[6], Memory->Tools.BrushOpacity / 100.0f);

			glBindBuffer(GL_ARRAY_BUFFER, Memory->VertexBuffers[PapayaVertexBuffer_RTTBrush].VboHandle);
			glBindVertexArray(Memory->VertexBuffers[PapayaVertexBuffer_RTTBrush].VaoHandle);
		
			glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)Memory->FboSampleTexture);
			glDrawArrays(GL_TRIANGLES, 0, 6);

			uint32 Temp = Memory->FboRenderTexture;
			Memory->FboRenderTexture = Memory->FboSampleTexture;
			glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, Memory->FboRenderTexture, 0);
			Memory->FboSampleTexture = Temp;

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glViewport(0, 0, (int)ImGui::GetIO().DisplaySize.x, (int)ImGui::GetIO().DisplaySize.y);
		}
		else if (Memory->Mouse.Released[0])
		{
			glDisable(GL_SCISSOR_TEST);
			glBindFramebuffer(GL_FRAMEBUFFER, Memory->FrameBufferObject);
			glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, Memory->FboRenderTexture, 0);

			glViewport(0, 0, Memory->Documents[0].Width, Memory->Documents[0].Height);

			// Setup orthographic projection matrix
			const float width = (float)Memory->Documents[0].Width;
			const float height = (float)Memory->Documents[0].Height;
			const float ortho_projection[4][4] =						// TODO: Separate out all ortho projection matrices into a single variable in PapayaMemory
			{
				{ 2.0f/width,	0.0f,			0.0f,		0.0f },
				{ 0.0f,			2.0f/-height,	0.0f,		0.0f },
				{ 0.0f,			0.0f,			-1.0f,		0.0f },
				{ -1.0f,		1.0f,			0.0f,		1.0f },
			};
			glUseProgram(Memory->Shaders[PapayaShader_ImGui].Handle);
		

			glUniformMatrix4fv(Memory->Shaders[PapayaShader_ImGui].Uniforms[0], 1, GL_FALSE, &ortho_projection[0][0]);
			//glUniform1i(Memory->Shaders[PapayaShader_ImGui].Uniforms[1], 0); // Texture uniform

			glBindBuffer(GL_ARRAY_BUFFER, Memory->VertexBuffers[PapayaVertexBuffer_RTTAdd].VboHandle);
			glBindVertexArray(Memory->VertexBuffers[PapayaVertexBuffer_RTTAdd].VaoHandle);

			glEnable(GL_BLEND);
			glBlendEquationSeparate(GL_FUNC_ADD, GL_MAX); // TODO: Handle the case where the original texture has an alpha below 1.0
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)Memory->Documents[0].TextureID);
			glDrawArrays(GL_TRIANGLES, 0, 6);
			glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)Memory->FboSampleTexture);
			glDrawArrays(GL_TRIANGLES, 0, 6);

			uint32 Temp = Memory->FboRenderTexture;
			Memory->FboRenderTexture = Memory->Documents[0].TextureID;
			glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, Memory->FboRenderTexture, 0);
			Memory->Documents[0].TextureID = Temp;
			
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glViewport(0, 0, (int)ImGui::GetIO().DisplaySize.x, (int)ImGui::GetIO().DisplaySize.y);

			glDisable(GL_BLEND);

//==================================================================================================
// NOTE: On AMD drivers, the following is causing texture corruption (black spots) and crashes.
// Why, though? I added a glFinish() call, plus generous sleep before the call to GetTexImage.
// Didn't make a difference. Needs more investigation.
//
#if 0
			//glFinish();
			//Sleep(5000);
			glBindTexture(GL_TEXTURE_2D, Memory->Documents[0].TextureID);
			glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, Memory->Documents[0].Texture); // TODO: Do we even need a local texture copy?
#endif
//==================================================================================================
			Memory->DrawOverlay = false;
		}

#if 0
		// =========================================================================================
		// Brush falloff visualization

		const int32 ArraySize = 256;
		local_persist float Opacities[ArraySize] = { 0 };

		const float MaxScale = 90.0f;
		float t = Memory->Tools.BrushHardness / 100.0f;
		float Scale = 1.0f / (1.0f - t);
		float Phase = (1.0f - Scale) * (float)Math::Pi;
		float Period = (float)Math::Pi * Scale / (float)ArraySize;

		for (int32 i = 0; i < ArraySize; i++)
		{
			Opacities[i] = (cosf(((float)i * Period) + Phase) + 1.0f) * 0.5f;
			if ((float)i < (float)ArraySize - ((float)ArraySize / Scale)) { Opacities[i] = 1.0f; }
		}

		ImGui::Begin("Brush falloff");
		ImGui::PlotLines("", Opacities, ArraySize, 0, 0, FLT_MIN, FLT_MAX, Vec2(256,256));
		ImGui::Text("%.2f - %.2f", Opacities[0], Opacities[ArraySize - 1]);
		ImGui::End();
		// =========================================================================================
#endif
}
	#pragma endregion

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

	#pragma region Draw canvas
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
		glBindBuffer(GL_ARRAY_BUFFER, 0); // TODO: Remove?
		glBindVertexArray(Memory->VertexBuffers[PapayaVertexBuffer_ImGui].VaoHandle);

		glScissor(34 + (int)Memory->Window.MaximizeOffset, 
				  3 + (int)Memory->Window.MaximizeOffset, 
				  (int)Memory->Window.Width - 37 - (2 * (int)Memory->Window.MaximizeOffset), 
				  (int)Memory->Window.Height - 58 - (2 * (int)Memory->Window.MaximizeOffset));

		glClearBufferfv(GL_COLOR, 0, (float*)&Memory->InterfaceColors[PapayaInterfaceColor_Workspace]);

		if (Memory->DrawCanvas)
		{
			glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)Memory->Documents[0].TextureID);
			glDrawArrays(GL_TRIANGLES, 0, 6);
		}
		if (Memory->DrawOverlay)
		{
			glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)Memory->FboSampleTexture);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glDrawArrays(GL_TRIANGLES, 0, 6);
		}

		// Restore modified state
		glBindVertexArray(0);
		glUseProgram(last_program);
		glDisable(GL_SCISSOR_TEST);
		glDisable(GL_BLEND);
		glBindTexture(GL_TEXTURE_2D, last_texture);
	}
	#pragma endregion

	#pragma region Draw brush cursor
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
		glUseProgram(Memory->Shaders[PapayaShader_BrushCursor].Handle);
		glUniformMatrix4fv(Memory->Shaders[PapayaShader_BrushCursor].Uniforms[0], 1, GL_FALSE, &ortho_projection[0][0]); // Projection matrix uniform
		glUniform4f(Memory->Shaders[PapayaShader_BrushCursor].Uniforms[1], 1.0f, 0.0f, 0.0f, Memory->Tools.BrushOpacity / 100.0f); // Brush color 
		glUniform1f(Memory->Shaders[PapayaShader_BrushCursor].Uniforms[2], Memory->Tools.BrushHardness / 100.0f);					// Hardness

		// Grow our buffer according to what we need
		glBindBuffer(GL_ARRAY_BUFFER, Memory->VertexBuffers[PapayaVertexBuffer_BrushCursor].VboHandle);

		ImDrawVert* buffer_data = (ImDrawVert*)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
		float ScaledDiameter = Memory->Tools.BrushDiameter * Memory->Documents[0].CanvasZoom;
		Vec2 Size = Vec2(ScaledDiameter,ScaledDiameter);
		Vec2 Position = (Memory->Mouse.IsDown[1] || Memory->Mouse.WasDown[1] ? Memory->Tools.RightClickDragStartPos : Memory->Mouse.Pos) - (Size * 0.5f);
		buffer_data[0].pos = Vec2(Position.x, Position.y);		
		buffer_data[1].pos = Vec2(Size.x + Position.x, Position.y);	
		buffer_data[2].pos = Vec2(Size.x + Position.x, Size.y + Position.y);
		buffer_data[3].pos = Vec2(Position.x, Position.y);
		buffer_data[4].pos = Vec2(Size.x + Position.x, Size.y + Position.y);
		buffer_data[5].pos = Vec2(Position.x, Size.y + Position.y);
		glUnmapBuffer(GL_ARRAY_BUFFER);

		glBindVertexArray(Memory->VertexBuffers[PapayaVertexBuffer_BrushCursor].VaoHandle);

		/*glScissor(34 + (int)Memory->Window.MaximizeOffset, 
				  3 + (int)Memory->Window.MaximizeOffset, 
				  (int)Memory->Window.Width - 37 - (2 * (int)Memory->Window.MaximizeOffset), 
				  (int)Memory->Window.Height - 58 - (2 * (int)Memory->Window.MaximizeOffset));*/


		glDrawArrays(GL_TRIANGLES, 0, 6);


		// Restore modified state
		glBindVertexArray(0);
		glUseProgram(last_program);
		glDisable(GL_SCISSOR_TEST);
		glDisable(GL_BLEND);
		glBindTexture(GL_TEXTURE_2D, last_texture);
	}
	#pragma endregion

	#pragma region Title Bar Icon
	{
		ImGui::SetNextWindowSize(ImVec2((float)Memory->Window.IconWidth,(float)Memory->Window.TitleBarHeight));
		ImGui::SetNextWindowPos(ImVec2(1.0f + Memory->Window.MaximizeOffset, 1.0f + Memory->Window.MaximizeOffset));

		ImGuiWindowFlags WindowFlags = 0;
		WindowFlags |= ImGuiWindowFlags_NoTitleBar;
		WindowFlags |= ImGuiWindowFlags_NoResize;
		WindowFlags |= ImGuiWindowFlags_NoMove;
		WindowFlags |= ImGuiWindowFlags_NoScrollbar;
		WindowFlags |= ImGuiWindowFlags_NoCollapse;
		WindowFlags |= ImGuiWindowFlags_NoScrollWithMouse;

		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(2,2));
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0,0));
		ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(0,0));
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0,0));

		ImGui::PushStyleColor(ImGuiCol_WindowBg, Memory->InterfaceColors[PapayaInterfaceColor_Transparent]);

		bool bTrue = true;
		ImGui::Begin("Title Bar Icon", &bTrue, WindowFlags);
		ImGui::Image((void*)Memory->InterfaceTextureIDs[PapayaInterfaceTexture_TitleBarIcon], ImVec2(28,28));
		ImGui::End();

		ImGui::PopStyleColor(1);
		ImGui::PopStyleVar(5);
	}
	#pragma endregion

	#pragma region Title Bar Menu
	{
		ImGui::SetNextWindowSize(ImVec2(Memory->Window.Width - Memory->Window.IconWidth - Memory->Window.TitleBarButtonsWidth - 3.0f - (2.0f * Memory->Window.MaximizeOffset),
			Memory->Window.TitleBarHeight - 10.0f));
		ImGui::SetNextWindowPos(ImVec2(2.0f + Memory->Window.IconWidth + Memory->Window.MaximizeOffset,
			1.0f + Memory->Window.MaximizeOffset + 5.0f));

		ImGuiWindowFlags WindowFlags = 0;
		WindowFlags |= ImGuiWindowFlags_NoTitleBar;
		WindowFlags |= ImGuiWindowFlags_NoResize;
		WindowFlags |= ImGuiWindowFlags_NoMove;
		WindowFlags |= ImGuiWindowFlags_NoScrollbar;
		WindowFlags |= ImGuiWindowFlags_NoCollapse;
		WindowFlags |= ImGuiWindowFlags_NoScrollWithMouse;
		WindowFlags |= ImGuiWindowFlags_MenuBar;

		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0,4));
		ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(5,5));
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8,8));

		ImGui::PushStyleColor(ImGuiCol_WindowBg, Memory->InterfaceColors[PapayaInterfaceColor_Transparent]);
		ImGui::PushStyleColor(ImGuiCol_MenuBarBg, Memory->InterfaceColors[PapayaInterfaceColor_Transparent]);
		ImGui::PushStyleColor(ImGuiCol_HeaderHovered, Memory->InterfaceColors[PapayaInterfaceColor_ButtonHover]);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8,4));
		ImGui::PushStyleColor(ImGuiCol_Header, Memory->InterfaceColors[PapayaInterfaceColor_Transparent]);

		bool bTrue = true;
		ImGui::Begin("Title Bar Menu", &bTrue, WindowFlags);
		bool foo;
		if (ImGui::BeginMenuBar())
		{
			ImGui::PushStyleColor(ImGuiCol_WindowBg, Memory->InterfaceColors[PapayaInterfaceColor_Clear]);
			if (ImGui::BeginMenu("FILE"))
			{
				#pragma region File Menu
				{
					if (ImGui::MenuItem("New")) {}
					if (ImGui::MenuItem("Open", "Ctrl+O")) {}
					if (ImGui::BeginMenu("Open Recent"))
					{
						ImGui::MenuItem("fish_hat.c");
						ImGui::MenuItem("fish_hat.inl");
						ImGui::MenuItem("fish_hat.h");
						if (ImGui::BeginMenu("More.."))
						{
							ImGui::MenuItem("Hello");
							ImGui::MenuItem("Sailor");
							if (ImGui::BeginMenu("Recurse.."))
							{
								ShowExampleMenuFile();
								ImGui::EndMenu();
							}
							ImGui::EndMenu();
						}
						ImGui::EndMenu();
					}
					if (ImGui::MenuItem("Save", "Ctrl+S")) {}
					if (ImGui::MenuItem("Save As..")) {}
					ImGui::Separator();
					if (ImGui::BeginMenu("Options"))
					{
						static bool enabled = true;
						ImGui::MenuItem("Enabled", "", &enabled);
						ImGui::BeginChild("child", ImVec2(0, 60), true);
						for (int i = 0; i < 10; i++)
							ImGui::Text("Scrolling Text %d", i);
						ImGui::EndChild();
						static float f = 0.5f;
						ImGui::SliderFloat("Value", &f, 0.0f, 1.0f);
						ImGui::InputFloat("Input", &f, 0.1f);
						ImGui::EndMenu();
					}
					if (ImGui::BeginMenu("Colors"))
					{
						for (int i = 0; i < ImGuiCol_COUNT; i++)
							ImGui::MenuItem(ImGui::GetStyleColName((ImGuiCol)i));
						ImGui::EndMenu();
					}
					if (ImGui::BeginMenu("Disabled", false)) // Disabled
					{
						IM_ASSERT(0);
					}
					if (ImGui::MenuItem("Checked", NULL, true)) {}
					if (ImGui::MenuItem("Quit", "Alt+F4")) { Memory->IsRunning = false; }
				}
				#pragma endregion
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("EDIT"))
			{
				#pragma region Edit Menu
				{
					ImGui::MenuItem("Dummy", NULL, &foo);
				}
				#pragma endregion
				ImGui::EndMenu();
			}
			ImGui::EndMenuBar();
			ImGui::PopStyleColor();
		}
		ImGui::End();

		ImGui::PopStyleColor(4);
		ImGui::PopStyleVar(5);
	}
	#pragma endregion

	#pragma region Left toolbar
	{
		ImGui::SetNextWindowSize(ImVec2(36,650));
		ImGui::SetNextWindowPos(ImVec2(1.0f + Memory->Window.MaximizeOffset, 55.0f + Memory->Window.MaximizeOffset));
			
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
