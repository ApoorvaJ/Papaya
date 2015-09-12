#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

namespace Papaya
{

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

internal bool OpenDocument(char* Path, PapayaMemory* Memory)
{
    #pragma region Load image
    {
        Memory->Document.Texture = stbi_load(Path, &Memory->Document.Width, &Memory->Document.Height, &Memory->Document.ComponentsPerPixel, 4);

        if (!Memory->Document.Texture) { return false; }

        // Create texture
        glGenTextures(1, &Memory->Document.TextureID);
        glBindTexture(GL_TEXTURE_2D, Memory->Document.TextureID);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, Memory->Document.Width, Memory->Document.Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, Memory->Document.Texture);

        Memory->Document.InverseAspect = (float)Memory->Document.Height / (float)Memory->Document.Width;
        Memory->Document.CanvasZoom = 0.8f * Math::Min((float)Memory->Window.Width/(float)Memory->Document.Width, (float)Memory->Window.Height/(float)Memory->Document.Height);
        if (Memory->Document.CanvasZoom > 1.0f) { Memory->Document.CanvasZoom = 1.0f; }
        Memory->Document.CanvasPosition = Vec2((Memory->Window.Width  - (float)Memory->Document.Width  * Memory->Document.CanvasZoom)/2.0f,
                                               (Memory->Window.Height - (float)Memory->Document.Height * Memory->Document.CanvasZoom)/2.0f); // TODO: Center with respect to canvas, not window
    }
    #pragma endregion

    #pragma region Set up the frame buffer
    {
        // Create a framebuffer object and bind it
        glGenFramebuffers(1, &Memory->FrameBufferObject);
        glBindFramebuffer(GL_FRAMEBUFFER, Memory->FrameBufferObject);

        Memory->FboRenderTexture = AllocateEmptyTexture(Memory->Document.Width, Memory->Document.Height);
        Memory->FboSampleTexture = AllocateEmptyTexture(Memory->Document.Width, Memory->Document.Height);

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
        Vec2 Size = Vec2((float)Memory->Document.Width, (float)Memory->Document.Height);
        ImDrawVert Verts[6];
        Verts[0].pos = Vec2(Position.x, Position.y);                    Verts[0].uv = Vec2(0.0f, 1.0f);    Verts[0].col = 0xffffffff;
        Verts[1].pos = Vec2(Size.x + Position.x, Position.y);           Verts[1].uv = Vec2(1.0f, 1.0f);    Verts[1].col = 0xffffffff;
        Verts[2].pos = Vec2(Size.x + Position.x, Size.y + Position.y);  Verts[2].uv = Vec2(1.0f, 0.0f);    Verts[2].col = 0xffffffff;
        Verts[3].pos = Vec2(Position.x, Position.y);                    Verts[3].uv = Vec2(0.0f, 1.0f);    Verts[3].col = 0xffffffff;
        Verts[4].pos = Vec2(Size.x + Position.x, Size.y + Position.y);  Verts[4].uv = Vec2(1.0f, 0.0f);    Verts[4].col = 0xffffffff;
        Verts[5].pos = Vec2(Position.x, Size.y + Position.y);           Verts[5].uv = Vec2(0.0f, 0.0f);    Verts[5].col = 0xffffffff;
        glBufferData(GL_ARRAY_BUFFER, sizeof(Verts), Verts, GL_STATIC_DRAW);

        glGenBuffers(1, &Memory->VertexBuffers[PapayaVertexBuffer_RTTAdd].VboHandle);
        glBindBuffer(GL_ARRAY_BUFFER, Memory->VertexBuffers[PapayaVertexBuffer_RTTAdd].VboHandle);
        glGenVertexArrays(1, &Memory->VertexBuffers[PapayaVertexBuffer_RTTAdd].VaoHandle);
        glBindVertexArray(Memory->VertexBuffers[PapayaVertexBuffer_RTTAdd].VaoHandle);

        glEnableVertexAttribArray(Memory->Shaders[PapayaShader_ImGui].Attributes[0]); // Position attribute
        glEnableVertexAttribArray(Memory->Shaders[PapayaShader_ImGui].Attributes[1]); // UV attribute
        glEnableVertexAttribArray(Memory->Shaders[PapayaShader_ImGui].Attributes[2]); // Vertex color attribute
    #define OFFSETOF(TYPE, ELEMENT) ((size_t)&(((TYPE *)0)->ELEMENT))
        glVertexAttribPointer(Memory->Shaders[PapayaShader_ImGui].Attributes[0], 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)OFFSETOF(ImDrawVert, pos));        // Position attribute
        glVertexAttribPointer(Memory->Shaders[PapayaShader_ImGui].Attributes[1], 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)OFFSETOF(ImDrawVert, uv));         // UV attribute
        glVertexAttribPointer(Memory->Shaders[PapayaShader_ImGui].Attributes[2], 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(ImDrawVert), (GLvoid*)OFFSETOF(ImDrawVert, col)); // Vertex color attribute
    #undef OFFSETOF
        glBufferData(GL_ARRAY_BUFFER, sizeof(Verts), Verts, GL_STATIC_DRAW);
        //glBindVertexArray(0);
        //glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
    #pragma endregion

    return true;
}

internal void CloseDocument(PapayaMemory* Memory)
{
    // Document
    if (Memory->Document.Texture)
    {
        free(Memory->Document.Texture);
        Memory->Document.Texture = 0;
    }

    if (Memory->Document.TextureID)
    {
        glDeleteTextures(1, &Memory->Document.TextureID);
        Memory->Document.TextureID = 0;
    }

    // Frame buffer
    if (Memory->FrameBufferObject)
    {
        glDeleteFramebuffers(1, &Memory->FrameBufferObject);
        Memory->FrameBufferObject = 0;
    }

    if (Memory->FboRenderTexture)
    {
        glDeleteTextures(1, &Memory->FboRenderTexture);
        Memory->FboRenderTexture = 0;
    }

    if (Memory->FboSampleTexture)
    {
        glDeleteTextures(1, &Memory->FboSampleTexture);
        Memory->FboSampleTexture = 0;
    }

    // Vertex Buffer: RTTBrush
    if (Memory->VertexBuffers[PapayaVertexBuffer_RTTBrush].VboHandle)
    {
        glDeleteBuffers(1, &Memory->VertexBuffers[PapayaVertexBuffer_RTTBrush].VboHandle);
        Memory->VertexBuffers[PapayaVertexBuffer_RTTBrush].VboHandle = 0;
    }

    if (Memory->VertexBuffers[PapayaVertexBuffer_RTTBrush].VaoHandle)
    {
        glDeleteVertexArrays(1, &Memory->VertexBuffers[PapayaVertexBuffer_RTTBrush].VaoHandle);
        Memory->VertexBuffers[PapayaVertexBuffer_RTTBrush].VaoHandle = 0;
    }

    // Vertex Buffer: RTTAdd
    if (Memory->VertexBuffers[PapayaVertexBuffer_RTTAdd].VboHandle)
    {
        glDeleteBuffers(1, &Memory->VertexBuffers[PapayaVertexBuffer_RTTAdd].VboHandle);
        Memory->VertexBuffers[PapayaVertexBuffer_RTTAdd].VboHandle = 0;
    }

    if (Memory->VertexBuffers[PapayaVertexBuffer_RTTAdd].VaoHandle)
    {
        glDeleteVertexArrays(1, &Memory->VertexBuffers[PapayaVertexBuffer_RTTAdd].VaoHandle);
        Memory->VertexBuffers[PapayaVertexBuffer_RTTAdd].VaoHandle = 0;
    }
}

void Initialize(PapayaMemory* Memory)
{
	#pragma region Init tool params
	{
		Memory->Tools.BrushDiameter = 50;
		Memory->Tools.BrushHardness = 90.0f;
		Memory->Tools.BrushOpacity = 100.0f;
		Memory->DrawCanvas = true;
		Memory->DrawOverlay = false;
		Memory->Tools.CurrentColor = Color(220, 163, 89);
		Memory->Tools.ColorPickerOpen = false;
		Memory->Tools.HueStripPosition = Vec2(305, 128);
		Memory->Tools.HueStripSize = Vec2(30, 256);
	}
	#pragma endregion

    #pragma region Brush shader
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
"																													\n"
"				#define M_PI 3.1415926535897932384626433832795														\n"
"																													\n"
"				uniform sampler2D	Texture;																		\n" // Uniforms[1]
"				uniform vec2		Pos;																			\n" // Uniforms[2]
"				uniform vec2		LastPos;																		\n" // Uniforms[3]
"				uniform float		Radius;																			\n" // Uniforms[4]
"				uniform vec4		BrushColor;																		\n" // Uniforms[5]
"				uniform float		Hardness;																		\n" // Uniforms[6]
"				uniform float		InvAspect;																		\n" // Uniforms[7]
"																													\n"
"				in vec2 Frag_UV;																					\n"
"				out vec4 Out_Color;																					\n"
"																													\n"
"				void line(vec2 p1, vec2 p2, vec2 uv, float radius, out float distLine, out float distp1)			\n"
"				{																									\n"
"					if (distance(p1,p2) <= 0.0)																		\n"
"					{																								\n"
"						distLine = distance(uv, p1);																\n"
"						distp1 = 0.0;																				\n"
"						return;																						\n"
"					}																								\n"
"																													\n"
"					float a = abs(distance(p1, uv));																\n"
"					float b = abs(distance(p2, uv));																\n"
"					float c = abs(distance(p1, p2));																\n"
"					float d = sqrt(c*c + radius*radius);															\n"
"																													\n"
"					vec2 a1 = normalize(uv - p1);																	\n"
"					vec2 b1 = normalize(uv - p2);																	\n"
"					vec2 c1 = normalize(p2 - p1);																	\n"
"					if (dot(a1,c1) < 0.0)																			\n"
"					{																								\n"
"						distLine = a;																				\n"
"						distp1 = 0.0;																				\n"
"						return;																						\n"
"					}																								\n"
"																													\n"
"					if (dot(b1,c1) > 0.0)																			\n"
"					{																								\n"
"						distLine = b;																				\n"
"						distp1 = 1.0;																				\n"
"						return;																						\n"
"					}																								\n"
"																													\n"
"					float p = (a + b + c) * 0.5;																	\n"
"					float h = 2.0 / c * sqrt( p * (p-a) * (p-b) * (p-c) );											\n"
"																													\n"
"					if (isnan(h))																					\n"
"					{																								\n"
"						distLine = 0.0;																				\n"
"						distp1 = a / c;																				\n"
"					}																								\n"
"					else																							\n"
"					{																								\n"
"						distLine = h;																				\n"
"						distp1 = sqrt(a*a - h*h) / c;																\n"
"					}																								\n"
"				}																									\n"
"																													\n"
"				void main()																							\n"
"				{																									\n"
"					vec4 t = texture(Texture, Frag_UV.st);															\n"
"																													\n"
"					float distLine, distp1;																			\n"
"					vec2 aspectUV = vec2(Frag_UV.x, Frag_UV.y * InvAspect);											\n"
"					line(LastPos, Pos, aspectUV, Radius, distLine, distp1);											\n"
"																													\n"
"					float Scale = 1.0 / (2.0 * Radius * (1.0 - Hardness));											\n"
"					float Period = M_PI * Scale;																	\n"
"					float Phase = (1.0 - Scale * 2.0 * Radius) * M_PI * 0.5;										\n"
"					float Alpha = cos((Period * distLine) + Phase);													\n"
"					if (distLine < Radius - (0.5/Scale)) Alpha = 1.0;												\n"
"					if (distLine > Radius)		  Alpha = 0.0;														\n"
"																													\n"
"					float FinalAlpha = max(t.a, Alpha * BrushColor.a);												\n"
"																													\n"
"					Out_Color = vec4(BrushColor.r, BrushColor.g, BrushColor.b, 										\n"
"								clamp(FinalAlpha,0.0,1.0));															\n" // TODO: Needs improvement. Self-intersection corners look weird.
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
        Memory->Shaders[PapayaShader_Brush].Uniforms[4]   = glGetUniformLocation(Memory->Shaders[PapayaShader_Brush].Handle, "Radius");
        Memory->Shaders[PapayaShader_Brush].Uniforms[5]   = glGetUniformLocation(Memory->Shaders[PapayaShader_Brush].Handle, "BrushColor");
        Memory->Shaders[PapayaShader_Brush].Uniforms[6]   = glGetUniformLocation(Memory->Shaders[PapayaShader_Brush].Handle, "Hardness");
        Memory->Shaders[PapayaShader_Brush].Uniforms[7]   = glGetUniformLocation(Memory->Shaders[PapayaShader_Brush].Handle, "InvAspect");
    }
    #pragma endregion

    #pragma region Brush cursor shader
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
"				uniform float		PixelDiameter;																	\n" // Uniforms[3]
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
"					float BorderThickness = 1.0 / PixelDiameter;													\n"
"					Out_Color = (Dist > 0.5 - BorderThickness && Dist < 0.5) ?										\n"
"					            vec4(0.0,0.0,0.0,1.0) :																\n"
"					            vec4(BrushColor.r, BrushColor.g, BrushColor.b, 	Alpha * BrushColor.a);				\n"
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
        Memory->Shaders[PapayaShader_BrushCursor].Uniforms[3]   = glGetUniformLocation(Memory->Shaders[PapayaShader_BrushCursor].Handle, "PixelDiameter");

        // Vertex buffer
        glGenBuffers(1, &Memory->VertexBuffers[PapayaVertexBuffer_BrushCursor].VboHandle);
        glBindBuffer(GL_ARRAY_BUFFER, Memory->VertexBuffers[PapayaVertexBuffer_BrushCursor].VboHandle);
        glGenVertexArrays(1, &Memory->VertexBuffers[PapayaVertexBuffer_BrushCursor].VaoHandle);
        glBindVertexArray(Memory->VertexBuffers[PapayaVertexBuffer_BrushCursor].VaoHandle);
        glEnableVertexAttribArray(Memory->Shaders[PapayaShader_BrushCursor].Attributes[0]); // Position attribute
        glEnableVertexAttribArray(Memory->Shaders[PapayaShader_BrushCursor].Attributes[1]); // Position attribute
#define OFFSETOF(TYPE, ELEMENT) ((size_t)&(((TYPE *)0)->ELEMENT))
        glVertexAttribPointer(Memory->Shaders[PapayaShader_BrushCursor].Attributes[0], 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)OFFSETOF(ImDrawVert, pos));   // Position attribute
        glVertexAttribPointer(Memory->Shaders[PapayaShader_BrushCursor].Attributes[1], 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)OFFSETOF(ImDrawVert, uv));    // UV attribute
        //glVertexAttribPointer(Memory->Shaders[PapayaShader_Brush].Attributes[2], 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(ImDrawVert), (GLvoid*)OFFSETOF(ImDrawVert, col)); // Vertex color attribute
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

    #pragma region Picker saturation-value shader
    {
    const GLchar *vertex_shader =
"   #version 330                                            \n"
"   uniform mat4 ProjMtx;                                   \n" // Uniforms[0]
"                                                           \n"
"   in  vec2 Position;                                      \n" // Attributes[0]
"   in  vec2 UV;                                            \n" // Attributes[1]
"   out vec2 Frag_UV;                                       \n"
"                                                           \n"
"   void main()                                             \n"
"   {                                                       \n"
"       Frag_UV = UV;                                       \n"
"       gl_Position = ProjMtx * vec4(Position.xy,0,1);      \n"
"   }                                                       \n";

    const GLchar* fragment_shader =
"   #version 330                                                    \n"
"                                                                   \n"
"   uniform float Hue;                                              \n" // Uniforms[1]
"   uniform vec2 Current;                                           \n" // Uniforms[2]
"                                                                   \n"
"   in  vec2 Frag_UV;                                               \n"
"   out vec4 Out_Color;                                             \n"
"                                                                   \n"
"   vec3 hsv2rgb(vec3 c)                                            \n" // Source: Fast branchless RGB to HSV conversion in GLSL
"   {                                                               \n" // http://lolengine.net/blog/2013/07/27/rgb-to-hsv-in-glsl
"       vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);              \n"
"       vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);           \n"
"       return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);   \n"
"   }                                                               \n"
"                                                                   \n"
"   void main()                                                     \n"
"   {                                                               \n"
"       vec3 RGB = hsv2rgb(vec3(Hue, Frag_UV.x, Frag_UV.y));        \n"
"       Out_Color = vec4(RGB.x, RGB.y, RGB.z, 1.0);                 \n"
"   }                                                               \n";

        Memory->Shaders[PapayaShader_PickerSVBox].Handle = glCreateProgram();
        uint32 g_VertHandle = glCreateShader(GL_VERTEX_SHADER);
        uint32 g_FragHandle = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(g_VertHandle, 1, &vertex_shader, 0);
        glShaderSource(g_FragHandle, 1, &fragment_shader, 0);
        glCompileShader(g_VertHandle);
        glCompileShader(g_FragHandle);
        glAttachShader(Memory->Shaders[PapayaShader_PickerSVBox].Handle, g_VertHandle);
        glAttachShader(Memory->Shaders[PapayaShader_PickerSVBox].Handle, g_FragHandle);
        Util::PrintGlShaderCompilationStatus(g_VertHandle);
        Util::PrintGlShaderCompilationStatus(g_FragHandle);
        glLinkProgram (Memory->Shaders[PapayaShader_PickerSVBox].Handle);

        Memory->Shaders[PapayaShader_PickerSVBox].Attributes[0] = glGetAttribLocation (Memory->Shaders[PapayaShader_PickerSVBox].Handle, "Position");
        Memory->Shaders[PapayaShader_PickerSVBox].Attributes[1] = glGetAttribLocation (Memory->Shaders[PapayaShader_PickerSVBox].Handle, "UV");

        Memory->Shaders[PapayaShader_PickerSVBox].Uniforms[0]   = glGetUniformLocation(Memory->Shaders[PapayaShader_PickerSVBox].Handle, "ProjMtx");
        Memory->Shaders[PapayaShader_PickerSVBox].Uniforms[1]   = glGetUniformLocation(Memory->Shaders[PapayaShader_PickerSVBox].Handle, "Hue");
        Memory->Shaders[PapayaShader_PickerSVBox].Uniforms[2]   = glGetUniformLocation(Memory->Shaders[PapayaShader_PickerSVBox].Handle, "Current");

        // Vertex buffer
        glGenBuffers(1, &Memory->VertexBuffers[PapayaVertexBuffer_PickerSVBox].VboHandle);
        glBindBuffer(GL_ARRAY_BUFFER, Memory->VertexBuffers[PapayaVertexBuffer_PickerSVBox].VboHandle);
        glGenVertexArrays(1, &Memory->VertexBuffers[PapayaVertexBuffer_PickerSVBox].VaoHandle);
        glBindVertexArray(Memory->VertexBuffers[PapayaVertexBuffer_PickerSVBox].VaoHandle);
        glEnableVertexAttribArray(Memory->Shaders[PapayaShader_PickerSVBox].Attributes[0]); // Position attribute
        glEnableVertexAttribArray(Memory->Shaders[PapayaShader_PickerSVBox].Attributes[1]); // UV attribute
#define OFFSETOF(TYPE, ELEMENT) ((size_t)&(((TYPE *)0)->ELEMENT))
        glVertexAttribPointer(Memory->Shaders[PapayaShader_PickerSVBox].Attributes[0], 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)OFFSETOF(ImDrawVert, pos));   // Position attribute
        glVertexAttribPointer(Memory->Shaders[PapayaShader_PickerSVBox].Attributes[1], 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)OFFSETOF(ImDrawVert, uv));    // UV attribute
#undef OFFSETOF

        Vec2 Position = Vec2(42,128);
        Vec2 Size = Vec2(256,256);
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

    #pragma region Picker hue shader
    {
    const GLchar *vertex_shader =
"   #version 330                                            \n"
"   uniform mat4 ProjMtx;                                   \n" // Uniforms[0]
"                                                           \n"
"   in  vec2 Position;                                      \n" // Attributes[0]
"   in  vec2 UV;                                            \n" // Attributes[1]
"   out vec2 Frag_UV;                                       \n"
"                                                           \n"
"   void main()                                             \n"
"   {                                                       \n"
"       Frag_UV = UV;                                       \n"
"       gl_Position = ProjMtx * vec4(Position.xy,0,1);      \n"
"   }                                                       \n";

    const GLchar* fragment_shader =
"   #version 330                                                    \n"
"                                                                   \n"
"   uniform float Current;                                          \n" // Uniforms[1]
"                                                                   \n"
"   in  vec2 Frag_UV;                                               \n"
"   out vec4 Out_Color;                                             \n"
"                                                                   \n"
"   vec3 hsv2rgb(vec3 c)                                            \n" // Source: Fast branchless RGB to HSV conversion in GLSL
"   {                                                               \n" // http://lolengine.net/blog/2013/07/27/rgb-to-hsv-in-glsl
"       vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);              \n"
"       vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);           \n"
"       return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);   \n"
"   }                                                               \n"
"                                                                   \n"
"   void main()                                                     \n"
"   {                                                               \n"
"       vec4 Hue = vec4(hsv2rgb(vec3(Frag_UV.y, 1.0, 1.0)).xyz,1.0);\n"
"       if (abs(0.5 - Frag_UV.x) > 0.3333)                          \n"
"       {                                                           \n"
"           Out_Color = vec4(0.36,0.36,0.37,                        \n"
"                       float(abs(Frag_UV.y - Current) < 0.0039));  \n"
"       }                                                           \n"
"       else                                                        \n"
"           Out_Color = Hue;                                        \n"
"   }                                                               \n";

        Memory->Shaders[PapayaShader_PickerHStrip].Handle = glCreateProgram();
        uint32 g_VertHandle = glCreateShader(GL_VERTEX_SHADER);
        uint32 g_FragHandle = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(g_VertHandle, 1, &vertex_shader, 0);
        glShaderSource(g_FragHandle, 1, &fragment_shader, 0);
        glCompileShader(g_VertHandle);
        glCompileShader(g_FragHandle);
        glAttachShader(Memory->Shaders[PapayaShader_PickerHStrip].Handle, g_VertHandle);
        glAttachShader(Memory->Shaders[PapayaShader_PickerHStrip].Handle, g_FragHandle);
        Util::PrintGlShaderCompilationStatus(g_VertHandle);
        Util::PrintGlShaderCompilationStatus(g_FragHandle);
        glLinkProgram (Memory->Shaders[PapayaShader_PickerHStrip].Handle);

        Memory->Shaders[PapayaShader_PickerHStrip].Attributes[0] = glGetAttribLocation (Memory->Shaders[PapayaShader_PickerHStrip].Handle, "Position");
        Memory->Shaders[PapayaShader_PickerHStrip].Attributes[1] = glGetAttribLocation (Memory->Shaders[PapayaShader_PickerHStrip].Handle, "UV");

        Memory->Shaders[PapayaShader_PickerHStrip].Uniforms[0]   = glGetUniformLocation(Memory->Shaders[PapayaShader_PickerHStrip].Handle, "ProjMtx");
        Memory->Shaders[PapayaShader_PickerHStrip].Uniforms[1]   = glGetUniformLocation(Memory->Shaders[PapayaShader_PickerHStrip].Handle, "Current");

        // Vertex buffer
        glGenBuffers(1, &Memory->VertexBuffers[PapayaVertexBuffer_PickerHStrip].VboHandle);
        glBindBuffer(GL_ARRAY_BUFFER, Memory->VertexBuffers[PapayaVertexBuffer_PickerHStrip].VboHandle);
        glGenVertexArrays(1, &Memory->VertexBuffers[PapayaVertexBuffer_PickerHStrip].VaoHandle);
        glBindVertexArray(Memory->VertexBuffers[PapayaVertexBuffer_PickerHStrip].VaoHandle);
        glEnableVertexAttribArray(Memory->Shaders[PapayaShader_PickerHStrip].Attributes[0]); // Position attribute
        glEnableVertexAttribArray(Memory->Shaders[PapayaShader_PickerHStrip].Attributes[1]); // UV attribute
#define OFFSETOF(TYPE, ELEMENT) ((size_t)&(((TYPE *)0)->ELEMENT))
        glVertexAttribPointer(Memory->Shaders[PapayaShader_PickerHStrip].Attributes[0], 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)OFFSETOF(ImDrawVert, pos));   // Position attribute
        glVertexAttribPointer(Memory->Shaders[PapayaShader_PickerHStrip].Attributes[1], 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)OFFSETOF(ImDrawVert, uv));    // UV attribute
#undef OFFSETOF

        Vec2 Position = Memory->Tools.HueStripPosition;
        Vec2 Size = Memory->Tools.HueStripSize;
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
        glGenBuffers(1, &Memory->VertexBuffers[PapayaVertexBuffer_ImGui].ElementsHandle);

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
        //ImFont* my_font0 = io.Fonts->AddFontFromFileTTF("d:\\DroidSans.ttf", 15.0f);
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
        Memory->InterfaceColors[PapayaInterfaceColor_Clear]         = Color(45,45,48); // Dark grey
#else
        Memory->InterfaceColors[PapayaInterfaceColor_Clear]         = Color(114,144,154); // Light blue
#endif
        Memory->InterfaceColors[PapayaInterfaceColor_Workspace]     = Color(30,30,30); // Light blue
        Memory->InterfaceColors[PapayaInterfaceColor_Transparent]   = Color(0,0,0,0);
        Memory->InterfaceColors[PapayaInterfaceColor_Button]        = Color(92,92,94);
        Memory->InterfaceColors[PapayaInterfaceColor_ButtonHover]   = Color(64,64,64);
        Memory->InterfaceColors[PapayaInterfaceColor_ButtonActive]  = Color(0,122,204);
    }
    #pragma endregion

#ifdef PAPAYA_DEFAULT_IMAGE
    OpenDocument(PAPAYA_DEFAULT_IMAGE, Memory);
#endif    
}

void Shutdown(PapayaMemory* Memory)
{
    //TODO: Free stuff
}

void UpdateAndRender(PapayaMemory* Memory, PapayaDebugMemory* DebugMemory)
{
    #pragma region Current mouse info
    {
        Memory->Mouse.Pos  = ImGui::GetMousePos();
        Vec2 MousePixelPos = Vec2(Math::Floor((Memory->Mouse.Pos.x - Memory->Document.CanvasPosition.x) / Memory->Document.CanvasZoom),
                                  Math::Floor((Memory->Mouse.Pos.y - Memory->Document.CanvasPosition.y) / Memory->Document.CanvasZoom));
        Memory->Mouse.UV   = Vec2(MousePixelPos.x / (float) Memory->Document.Width,
                                  MousePixelPos.y / (float) Memory->Document.Height);

        for (int32 i = 0; i < 3; i++)
        {
            Memory->Mouse.IsDown[i]   = ImGui::IsMouseDown(i);
            Memory->Mouse.Pressed[i]  = (Memory->Mouse.IsDown[i] && !Memory->Mouse.WasDown[i]);
            Memory->Mouse.Released[i] = (!Memory->Mouse.IsDown[i] && Memory->Mouse.WasDown[i]);
        }
    }
    #pragma endregion

    #pragma region Clear screen buffer
    {
        glViewport(0, 0, (int)ImGui::GetIO().DisplaySize.x, (int)ImGui::GetIO().DisplaySize.y);
        glClearBufferfv(GL_COLOR, 0, (GLfloat*)&Memory->InterfaceColors[PapayaInterfaceColor_Clear]);

        glEnable(GL_SCISSOR_TEST);
        glScissor(34, 3,
                 (int)Memory->Window.Width  - 37, (int)Memory->Window.Height - 58); // TODO: Remove magic numbers

        glClearBufferfv(GL_COLOR, 0, (float*)&Memory->InterfaceColors[PapayaInterfaceColor_Workspace]);
        glDisable(GL_SCISSOR_TEST);
    }
    #pragma endregion

    #pragma region Title Bar Menu
    {
        ImGui::SetNextWindowSize(ImVec2(Memory->Window.Width - Memory->Window.MenuHorizontalOffset - Memory->Window.TitleBarButtonsWidth - 3.0f,
                                        Memory->Window.TitleBarHeight - 10.0f));
        ImGui::SetNextWindowPos(ImVec2(2.0f + Memory->Window.MenuHorizontalOffset, 6.0f));

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
        if (ImGui::BeginMenuBar())
        {
            ImGui::PushStyleColor(ImGuiCol_WindowBg, Memory->InterfaceColors[PapayaInterfaceColor_Clear]);
            if (ImGui::BeginMenu("FILE"))
            {
                #pragma region File Menu
                {
                    if (ImGui::MenuItem("Open"))
                    {
                        char* Path = Platform::OpenFileDialog();
                        if (Path)
                        {
                            CloseDocument(Memory);
                            OpenDocument(Path, Memory);
                            free(Path);
                        }
                    }

                    if (ImGui::MenuItem("Close"))
                    {
                        CloseDocument(Memory);
                    }

                    if (ImGui::MenuItem("Save", "Ctrl+S"))
                    {
                        char* Path = Platform::SaveFileDialog();
                        if (Path) // TODO: Do this on a separate thread. Massively blocks UI for large images.
                        {
                            glFinish();
                            glBindTexture(GL_TEXTURE_2D, Memory->Document.TextureID);
                            glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, Memory->Document.Texture);
                            glFinish();

                            int32 Result =  stbi_write_png(Path, Memory->Document.Width, Memory->Document.Height, 4, Memory->Document.Texture, 4 * Memory->Document.Width);
                            if (!Result)
                            {
                                // TODO: Log: Save failed
                                Platform::Print("Save failed\n");
                            }

                            free(Path);
                        }
                    }
                    if (ImGui::MenuItem("Save As..")) {}
                    ImGui::Separator();
                    if (ImGui::MenuItem("Quit", "Alt+F4")) { Memory->IsRunning = false; }
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
        ImGui::SetNextWindowPos(ImVec2(1.0f, 55.0f));

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

        #define CALCUV(X, Y) ImVec2((float)X*20.0f/256.0f, (float)Y*20.0f/256.0f)
        {
            ImGui::PushID(0);
            if (ImGui::ImageButton((void*)(intptr_t)Memory->InterfaceTextureIDs[PapayaInterfaceTexture_InterfaceIcons], ImVec2(20, 20), CALCUV(0, 0), CALCUV(1, 1), 6, ImVec4(0, 0, 0, 0)))
            {

            }
            ImGui::PopID();

            ImGui::PushID(1);
            if (ImGui::ImageButton((void*)(intptr_t)Memory->InterfaceTextureIDs[PapayaInterfaceTexture_InterfaceIcons], ImVec2(33, 33), CALCUV(0, 0), CALCUV(0, 0), 0, Memory->Tools.CurrentColor))
            {
                Memory->Tools.ColorPickerOpen = !Memory->Tools.ColorPickerOpen;
                Memory->Tools.NewColor = Memory->Tools.CurrentColor;
            }
            ImGui::PopID();
        }
        #undef CALCUV

        ImGui::End();

        ImGui::PopStyleVar(5);
        ImGui::PopStyleColor(4);
    }
    #pragma endregion

    #pragma region Color Picker
    {
        if (Memory->Tools.ColorPickerOpen) // TODO: Work-in-progress
        {
            float PosY = 86;
            ImGui::SetNextWindowSize(ImVec2(315, 35));
            ImGui::SetNextWindowPos(ImVec2(34, PosY));

            ImGuiWindowFlags WindowFlags = 0;
            WindowFlags |= ImGuiWindowFlags_NoTitleBar;
            WindowFlags |= ImGuiWindowFlags_NoResize;
            WindowFlags |= ImGuiWindowFlags_NoMove;
            WindowFlags |= ImGuiWindowFlags_NoScrollbar;
            WindowFlags |= ImGuiWindowFlags_NoCollapse;
            WindowFlags |= ImGuiWindowFlags_NoScrollWithMouse;

            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

            ImGui::PushStyleColor(ImGuiCol_Button, Memory->InterfaceColors[PapayaInterfaceColor_Button]);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Memory->InterfaceColors[PapayaInterfaceColor_ButtonHover]);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, Memory->InterfaceColors[PapayaInterfaceColor_ButtonActive]);
            ImGui::PushStyleColor(ImGuiCol_WindowBg, Memory->InterfaceColors[PapayaInterfaceColor_Clear]); // TODO: Investigate: Should be opaque, but is partially transparent
            ImGui::Begin("Color preview", 0, WindowFlags);
            {
                ImGui::ImageButton((void*)(intptr_t)Memory->InterfaceTextureIDs[PapayaInterfaceTexture_InterfaceIcons], ImVec2(141, 34), ImVec2(0, 0), ImVec2(0, 0), 0, Memory->Tools.CurrentColor);
                ImGui::SameLine();
                ImGui::ImageButton((void*)(intptr_t)Memory->InterfaceTextureIDs[PapayaInterfaceTexture_InterfaceIcons], ImVec2(173, 34), ImVec2(0, 0), ImVec2(0, 0), 0, Memory->Tools.NewColor);
            }
            ImGui::End();
            ImGui::PopStyleVar(3);

            ImGui::SetNextWindowSize(ImVec2(315, 330));
            ImGui::SetNextWindowPos(ImVec2(34, PosY + 35));
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
            ImGui::Begin("Color picker", 0, WindowFlags);
            {
                ImGui::BeginChild("HSV Gradients", Vec2(315, 258));
                ImGui::EndChild();
                int32 col[3];
                col[0] = (int)(Memory->Tools.NewColor.r * 255.0f);
                col[1] = (int)(Memory->Tools.NewColor.g * 255.0f);
                col[2] = (int)(Memory->Tools.NewColor.b * 255.0f);
                if (ImGui::InputInt3("RGB", col))
                {
                    Memory->Tools.NewColor = Color(col[0], col[1], col[2]); // TODO: Clamping
                }

                if (ImGui::Button("OK"))
                {
                    Memory->Tools.CurrentColor = Memory->Tools.NewColor;
                    Memory->Tools.ColorPickerOpen = false;
                }
                ImGui::SameLine();
                if (ImGui::Button("Cancel"))
                {
                    Memory->Tools.ColorPickerOpen = false;
                }
            }
            ImGui::End();
            ImGui::PopStyleVar(1);
            ImGui::PopStyleColor(4);
        }
    }
    #pragma endregion

    if (!Memory->Document.TextureID) { return; }

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

        ImGui::InputInt("Diameter", &Memory->Tools.BrushDiameter);
        Memory->Tools.BrushDiameter = Math::Clamp(Memory->Tools.BrushDiameter, 1, Memory->Tools.MaxBrushDiameter);

        ImGui::PopItemWidth();
        ImGui::PushItemWidth(80);
        ImGui::SameLine();
        ImGui::SliderFloat("Hardness", &Memory->Tools.BrushHardness, 0.0f, 100.0f, "%.0f");
        ImGui::SameLine();
        ImGui::SliderFloat("Opacity", &Memory->Tools.BrushOpacity, 0.0f, 100.0f, "%.0f");

        ImGui::PopItemWidth();
        ImGui::End();

        ImGui::PopStyleVar(3);
        ImGui::PopStyleColor(5);
    }
    #pragma endregion

    #pragma region Brush tool
    {
/*
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
            Memory->Document.CanvasZoom = 1.0;
        }
*/
        #pragma region Right mouse dragging
        {
            if (Memory->Mouse.Pressed[1])
            {
                Memory->Tools.RightClickDragStartPos = Memory->Mouse.Pos;
                Memory->Tools.RightClickDragStartDiameter = Memory->Tools.BrushDiameter;
                Memory->Tools.RightClickDragStartHardness = Memory->Tools.BrushHardness;
                Memory->Tools.RightClickDragStartOpacity = Memory->Tools.BrushOpacity;
                Memory->Tools.RightClickShiftPressed = ImGui::GetIO().KeyShift;
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
                    float Diameter = Memory->Tools.RightClickDragStartDiameter + (ImGui::GetMouseDragDelta(1).x / Memory->Document.CanvasZoom * 2.0f);
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

            glBindFramebuffer(GL_FRAMEBUFFER, Memory->FrameBufferObject);

            if (Memory->Mouse.Pressed[0])
            {
                GLuint clearColor[4] = {0, 0, 0, 0};
                glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, Memory->FboSampleTexture, 0);
                glClearBufferuiv(GL_COLOR, 0, clearColor);
                glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, Memory->FboRenderTexture, 0);
            }
            glViewport(0, 0, Memory->Document.Width, Memory->Document.Height);

            glDisable(GL_BLEND);
            glDisable(GL_SCISSOR_TEST);

            // Setup orthographic projection matrix
            const float width = (float)Memory->Document.Width;
            const float height = (float)Memory->Document.Height;
            const float ortho_projection[4][4] =
            {
                { 2.0f/width,   0.0f,           0.0f,       0.0f },
                { 0.0f,        -2.0f/height,    0.0f,       0.0f },
                { 0.0f,         0.0f,          -1.0f,       0.0f },
                { -1.0f,        1.0f,           0.0f,       1.0f },
            };
            glUseProgram(Memory->Shaders[PapayaShader_Brush].Handle);

            Vec2 CorrectedPos     = Memory->Mouse.UV     + (Memory->Tools.BrushDiameter % 2 == 0 ? Vec2() : Vec2(0.5f/width, 0.5f/height));
            Vec2 CorrectedLastPos = Memory->Mouse.LastUV + (Memory->Tools.BrushDiameter % 2 == 0 ? Vec2() : Vec2(0.5f/width, 0.5f/height));

#if 0
            // Brush testing routine
            local_persist int32 i = 0;

            if (i%2)
            {
                local_persist int32 j = 0;
                CorrectedPos		= Vec2( j*0.2f,     j*0.2f) + (Memory->Tools.BrushDiameter % 2 == 0 ? Vec2() : Vec2(0.5f/width, 0.5f/height));
                CorrectedLastPos	= Vec2((j+1)*0.2f, (j+1)*0.2f) + (Memory->Tools.BrushDiameter % 2 == 0 ? Vec2() : Vec2(0.5f/width, 0.5f/height));
                j++;
            }
            else
            {
                local_persist int32 k = 0;
                CorrectedPos		= Vec2( k*0.2f,     1.0f-k*0.2f) + (Memory->Tools.BrushDiameter % 2 == 0 ? Vec2() : Vec2(0.5f/width, 0.5f/height));
                CorrectedLastPos	= Vec2((k+1)*0.2f, 1.0f-(k+1)*0.2f) + (Memory->Tools.BrushDiameter % 2 == 0 ? Vec2() : Vec2(0.5f/width, 0.5f/height));
                k++;
            }
            i++;
#endif

            glUniformMatrix4fv(Memory->Shaders[PapayaShader_Brush].Uniforms[0], 1, GL_FALSE, &ortho_projection[0][0]);
            //glUniform1i(Memory->Shaders[PapayaShader_Brush].Uniforms[1], Memory->Document.TextureID); // Texture uniform
            glUniform2f(Memory->Shaders[PapayaShader_Brush].Uniforms[2], CorrectedPos.x, CorrectedPos.y * Memory->Document.InverseAspect); // Pos uniform
            glUniform2f(Memory->Shaders[PapayaShader_Brush].Uniforms[3], CorrectedLastPos.x, CorrectedLastPos.y * Memory->Document.InverseAspect); // Lastpos uniform
            glUniform1f(Memory->Shaders[PapayaShader_Brush].Uniforms[4], (float)Memory->Tools.BrushDiameter / ((float)Memory->Document.Width * 2.0f));
            float Opacity = Memory->Tools.BrushOpacity / 100.0f; //(Math::Distance(CorrectedLastPos, CorrectedPos) > 0.0 ? Memory->Tools.BrushOpacity / 100.0f : 0.0f);
            glUniform4f(Memory->Shaders[PapayaShader_Brush].Uniforms[5], Memory->Tools.CurrentColor.r, 
                                                                         Memory->Tools.CurrentColor.g, 
                                                                         Memory->Tools.CurrentColor.b, 
                                                                         Opacity);
            glUniform1f(Memory->Shaders[PapayaShader_Brush].Uniforms[6], Memory->Tools.BrushHardness / 100.0f);
            glUniform1f(Memory->Shaders[PapayaShader_Brush].Uniforms[7], Memory->Document.InverseAspect); // Inverse Aspect uniform

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

            glViewport(0, 0, Memory->Document.Width, Memory->Document.Height);

            // Setup orthographic projection matrix
            const float width = (float)Memory->Document.Width;
            const float height = (float)Memory->Document.Height;
            const float ortho_projection[4][4] =                    // TODO: Separate out all ortho projection matrices into a single variable in PapayaMemory
            {                                                       //
                { 2.0f/width,   0.0f,           0.0f,       0.0f }, //
                { 0.0f,         2.0f/-height,   0.0f,       0.0f }, //
                { 0.0f,         0.0f,          -1.0f,       0.0f }, //
                { -1.0f,        1.0f,           0.0f,       1.0f }, //
            };
            glUseProgram(Memory->Shaders[PapayaShader_ImGui].Handle);


            glUniformMatrix4fv(Memory->Shaders[PapayaShader_ImGui].Uniforms[0], 1, GL_FALSE, &ortho_projection[0][0]);
            //glUniform1i(Memory->Shaders[PapayaShader_ImGui].Uniforms[1], 0); // Texture uniform

            glBindBuffer(GL_ARRAY_BUFFER, Memory->VertexBuffers[PapayaVertexBuffer_RTTAdd].VboHandle);
            glBindVertexArray(Memory->VertexBuffers[PapayaVertexBuffer_RTTAdd].VaoHandle);

            glEnable(GL_BLEND);
            glBlendEquationSeparate(GL_FUNC_ADD, GL_MAX); // TODO: Handle the case where the original texture has an alpha below 1.0
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)Memory->Document.TextureID);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)Memory->FboSampleTexture);
            glDrawArrays(GL_TRIANGLES, 0, 6);

            uint32 Temp = Memory->FboRenderTexture;
            Memory->FboRenderTexture = Memory->Document.TextureID;
            glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, Memory->FboRenderTexture, 0);
            Memory->Document.TextureID = Temp;

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
            glBindTexture(GL_TEXTURE_2D, Memory->Document.TextureID);
            glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, Memory->Document.Texture); // TODO: Do we even need a local texture copy?
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
        Memory->Document.CanvasPosition += ImGui::GetMouseDragDelta(2);
        ImGui::ResetMouseDragDelta(2);

        // Zooming
        if (!ImGui::IsMouseDown(2) && ImGui::GetIO().MouseWheel)
        {
            float MinZoom = 0.01f, MaxZoom = 32.0f;
            float ZoomSpeed = 0.2f * Memory->Document.CanvasZoom;
            float ScaleDelta = Math::Min(MaxZoom - Memory->Document.CanvasZoom, ImGui::GetIO().MouseWheel * ZoomSpeed);
            Vec2 OldCanvasZoom = Vec2((float)Memory->Document.Width, (float)Memory->Document.Height) * Memory->Document.CanvasZoom;

            Memory->Document.CanvasZoom += ScaleDelta;
            if (Memory->Document.CanvasZoom < MinZoom) { Memory->Document.CanvasZoom = MinZoom; } // TODO: Dynamically clamp min such that fully zoomed out image is 2x2 pixels?
            Vec2 NewCanvasSize = Vec2((float)Memory->Document.Width, (float)Memory->Document.Height) * Memory->Document.CanvasZoom;

            if ((NewCanvasSize.x > Memory->Window.Width || NewCanvasSize.y > Memory->Window.Height))
            {
                Vec2 PreScaleMousePos = (Memory->Mouse.Pos - Memory->Document.CanvasPosition) / OldCanvasZoom;
                Memory->Document.CanvasPosition -= Vec2(PreScaleMousePos.x * ScaleDelta * (float)Memory->Document.Width, PreScaleMousePos.y * ScaleDelta * (float)Memory->Document.Height);
            }
            else // TODO: Maybe disable centering on zoom out. Needs more usability testing.
            {
                Vec2 WindowSize = Vec2((float)Memory->Window.Width, (float)Memory->Window.Height);
                Memory->Document.CanvasPosition = (WindowSize - NewCanvasSize) * 0.5f;
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
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (Memory->Document.CanvasZoom >= 2.0f) ? GL_NEAREST : GL_NEAREST);// : GL_LINEAR);

        // Setup orthographic projection matrix
        const float width = ImGui::GetIO().DisplaySize.x;
        const float height = ImGui::GetIO().DisplaySize.y;
        const float ortho_projection[4][4] =
        {
            { 2.0f/width,   0.0f,           0.0f,       0.0f },
            { 0.0f,         2.0f/-height,   0.0f,       0.0f },
            { 0.0f,         0.0f,          -1.0f,       0.0f },
            { -1.0f,        1.0f,           0.0f,       1.0f },
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
        Vec2 Position = Memory->Document.CanvasPosition;
        Vec2 Size = Vec2(Memory->Document.Width * Memory->Document.CanvasZoom, Memory->Document.Height * Memory->Document.CanvasZoom);
        ImDrawVert Verts[6];
        Verts[0].pos = Vec2(Position.x, Position.y);                    Verts[0].uv = Vec2(0.0f, 0.0f); Verts[0].col = 0xffffffff;
        Verts[1].pos = Vec2(Size.x + Position.x, Position.y);           Verts[1].uv = Vec2(1.0f, 0.0f); Verts[1].col = 0xffffffff;
        Verts[2].pos = Vec2(Size.x + Position.x, Size.y + Position.y);  Verts[2].uv = Vec2(1.0f, 1.0f); Verts[2].col = 0xffffffff;
        Verts[3].pos = Vec2(Position.x, Position.y);                    Verts[3].uv = Vec2(0.0f, 0.0f); Verts[3].col = 0xffffffff;
        Verts[4].pos = Vec2(Size.x + Position.x, Size.y + Position.y);  Verts[4].uv = Vec2(1.0f, 1.0f); Verts[4].col = 0xffffffff;
        Verts[5].pos = Vec2(Position.x, Size.y + Position.y);           Verts[5].uv = Vec2(0.0f, 1.0f); Verts[5].col = 0xffffffff;

        unsigned char* buffer_data = (unsigned char*)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
        memcpy(buffer_data, Verts, 6 * sizeof(ImDrawVert)); //TODO: Profile this.
        buffer_data += 6 * sizeof(ImDrawVert);
        glUnmapBuffer(GL_ARRAY_BUFFER);
        glBindBuffer(GL_ARRAY_BUFFER, 0); // TODO: Remove?
        glBindVertexArray(Memory->VertexBuffers[PapayaVertexBuffer_ImGui].VaoHandle);

        if (Memory->DrawCanvas)
        {
            glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)Memory->Document.TextureID);
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
            { 2.0f/width,   0.0f,           0.0f,       0.0f },
            { 0.0f,         2.0f/-height,   0.0f,       0.0f },
            { 0.0f,         0.0f,          -1.0f,       0.0f },
            { -1.0f,        1.0f,           0.0f,       1.0f },
        };
        glUseProgram(Memory->Shaders[PapayaShader_BrushCursor].Handle);
        glUniformMatrix4fv(Memory->Shaders[PapayaShader_BrushCursor].Uniforms[0], 1, GL_FALSE, &ortho_projection[0][0]);                // Projection matrix uniform
        glUniform4f(Memory->Shaders[PapayaShader_BrushCursor].Uniforms[1], 1.0f, 0.0f, 0.0f, Memory->Mouse.IsDown[1] ? 
                                                                                             Memory->Tools.BrushOpacity/100.0f : 0.0f);	// Brush color
        glUniform1f(Memory->Shaders[PapayaShader_BrushCursor].Uniforms[2], Memory->Tools.BrushHardness / 100.0f);						// Hardness
        glUniform1f(Memory->Shaders[PapayaShader_BrushCursor].Uniforms[3], Memory->Tools.BrushDiameter * Memory->Document.CanvasZoom);	// PixelDiameter

        // Grow our buffer according to what we need
        glBindBuffer(GL_ARRAY_BUFFER, Memory->VertexBuffers[PapayaVertexBuffer_BrushCursor].VboHandle);

        ImDrawVert* buffer_data = (ImDrawVert*)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
        float ScaledDiameter = Memory->Tools.BrushDiameter * Memory->Document.CanvasZoom;
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

    //EndOfFunction:

}

void RenderImGui(ImDrawData* DrawData, void* mem)
{
    PapayaMemory* Memory = (PapayaMemory*)mem;

    // Backup GL state
    GLint last_program, last_texture, last_array_buffer, last_element_array_buffer, last_vertex_array;
    glGetIntegerv(GL_CURRENT_PROGRAM, &last_program);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &last_array_buffer);
    glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &last_element_array_buffer);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &last_vertex_array);

    // Setup render state: alpha-blending enabled, no face culling, no depth testing, scissor enabled
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_SCISSOR_TEST);
    glActiveTexture(GL_TEXTURE0);

    // Handle cases of screen coordinates != from framebuffer coordinates (e.g. retina displays)
    ImGuiIO& io = ImGui::GetIO();
    float fb_height = io.DisplaySize.y * io.DisplayFramebufferScale.y;
    DrawData->ScaleClipRects(io.DisplayFramebufferScale);

    // Setup orthographic projection matrix
    const float ortho_projection[4][4] =
    {
        { 2.0f/io.DisplaySize.x, 0.0f,                   0.0f, 0.0f },
        { 0.0f,                  2.0f/-io.DisplaySize.y, 0.0f, 0.0f },
        { 0.0f,                  0.0f,                  -1.0f, 0.0f },
        {-1.0f,                  1.0f,                   0.0f, 1.0f },
    };
    glUseProgram(Memory->Shaders[PapayaShader_ImGui].Handle);
    glUniform1i(Memory->Shaders[PapayaShader_ImGui].Uniforms[1], 0);
    glUniformMatrix4fv(Memory->Shaders[PapayaShader_ImGui].Uniforms[0], 1, GL_FALSE, &ortho_projection[0][0]);
    glBindVertexArray(Memory->VertexBuffers[PapayaVertexBuffer_ImGui].VaoHandle);

    for (int n = 0; n < DrawData->CmdListsCount; n++)
    {
        const ImDrawList* cmd_list = DrawData->CmdLists[n];
        const ImDrawIdx* idx_buffer_offset = 0;

        glBindBuffer(GL_ARRAY_BUFFER, Memory->VertexBuffers[PapayaVertexBuffer_ImGui].VboHandle);
        glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)cmd_list->VtxBuffer.size() * sizeof(ImDrawVert), (GLvoid*)&cmd_list->VtxBuffer.front(), GL_STREAM_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, Memory->VertexBuffers[PapayaVertexBuffer_ImGui].ElementsHandle);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)cmd_list->IdxBuffer.size() * sizeof(ImDrawIdx), (GLvoid*)&cmd_list->IdxBuffer.front(), GL_STREAM_DRAW);

        for (const ImDrawCmd* pcmd = cmd_list->CmdBuffer.begin(); pcmd != cmd_list->CmdBuffer.end(); pcmd++)
        {
            if (pcmd->UserCallback)
            {
                pcmd->UserCallback(cmd_list, pcmd);
            }
            else
            {
                glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)pcmd->TextureId);
                glScissor((int)pcmd->ClipRect.x, (int)(fb_height - pcmd->ClipRect.w), (int)(pcmd->ClipRect.z - pcmd->ClipRect.x), (int)(pcmd->ClipRect.w - pcmd->ClipRect.y));
                glDrawElements(GL_TRIANGLES, (GLsizei)pcmd->ElemCount, GL_UNSIGNED_SHORT, idx_buffer_offset);
            }
            idx_buffer_offset += pcmd->ElemCount;
        }
    }

    // Restore modified GL state
    glUseProgram(last_program);
    glBindTexture(GL_TEXTURE_2D, last_texture);
    glBindBuffer(GL_ARRAY_BUFFER, last_array_buffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, last_element_array_buffer);
    glBindVertexArray(last_vertex_array);
    glDisable(GL_SCISSOR_TEST);
}

void RenderAfterGui(PapayaMemory* Memory)
{
    if (Memory->Tools.ColorPickerOpen)
    {
        #pragma region Update hue picker
        {
            
            if (Memory->Mouse.IsDown[0] &&
                Memory->Mouse.Pos.x > Memory->Tools.HueStripPosition.x &&
                Memory->Mouse.Pos.x < Memory->Tools.HueStripPosition.x + Memory->Tools.HueStripSize.x &&
                Memory->Mouse.Pos.y > Memory->Tools.HueStripPosition.y &&
                Memory->Mouse.Pos.y < Memory->Tools.HueStripPosition.y + Memory->Tools.HueStripSize.y)
            {
                Memory->Tools.NewColorHue = 1.0f - (Memory->Mouse.Pos.y - Memory->Tools.HueStripPosition.y)/256.0f;
            }
            
        }
        #pragma endregion

        #pragma region Draw hue picker
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

            // Setup orthographic projection matrix
            const float width = ImGui::GetIO().DisplaySize.x;
            const float height = ImGui::GetIO().DisplaySize.y;
            const float ortho_projection[4][4] =
            {
                { 2.0f/width,   0.0f,           0.0f,       0.0f },
                { 0.0f,         2.0f/-height,   0.0f,       0.0f },
                { 0.0f,         0.0f,          -1.0f,       0.0f },
                { -1.0f,        1.0f,           0.0f,       1.0f },
            };

            glUseProgram(Memory->Shaders[PapayaShader_PickerHStrip].Handle);
            glUniformMatrix4fv(Memory->Shaders[PapayaShader_PickerHStrip].Uniforms[0], 1, GL_FALSE, &ortho_projection[0][0]); // Projection matrix uniform
            glUniform1f(Memory->Shaders[PapayaShader_PickerHStrip].Uniforms[1], Memory->Tools.NewColorHue);                                        // Current

            glBindBuffer(GL_ARRAY_BUFFER, Memory->VertexBuffers[PapayaVertexBuffer_PickerHStrip].VboHandle);
            glBindVertexArray(Memory->VertexBuffers[PapayaVertexBuffer_PickerHStrip].VaoHandle);

            glDrawArrays(GL_TRIANGLES, 0, 6);

            // Restore modified state
            glBindVertexArray(0);
            glUseProgram(last_program);
            glDisable(GL_BLEND);
            glBindTexture(GL_TEXTURE_2D, last_texture);
        }
        #pragma endregion

        #pragma region Draw saturation-value picker
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

            // Setup orthographic projection matrix
            const float width = ImGui::GetIO().DisplaySize.x;
            const float height = ImGui::GetIO().DisplaySize.y;
            const float ortho_projection[4][4] =
            {
                { 2.0f/width,   0.0f,           0.0f,       0.0f },
                { 0.0f,         2.0f/-height,   0.0f,       0.0f },
                { 0.0f,         0.0f,          -1.0f,       0.0f },
                { -1.0f,        1.0f,           0.0f,       1.0f },
            };

            glUseProgram(Memory->Shaders[PapayaShader_PickerSVBox].Handle);
            glUniformMatrix4fv(Memory->Shaders[PapayaShader_PickerSVBox].Uniforms[0], 1, GL_FALSE, &ortho_projection[0][0]); // Projection matrix uniform
            glUniform1f(Memory->Shaders[PapayaShader_PickerSVBox].Uniforms[1], Memory->Tools.NewColorHue);                   // Hue
            glUniform2f(Memory->Shaders[PapayaShader_PickerSVBox].Uniforms[2], 0.0f, 0.0f);                                  // Current

            glBindBuffer(GL_ARRAY_BUFFER, Memory->VertexBuffers[PapayaVertexBuffer_PickerSVBox].VboHandle);
            glBindVertexArray(Memory->VertexBuffers[PapayaVertexBuffer_PickerSVBox].VaoHandle);

            glDrawArrays(GL_TRIANGLES, 0, 6);

            // Restore modified state
            glBindVertexArray(0);
            glUseProgram(last_program);
            glDisable(GL_BLEND);
            glBindTexture(GL_TEXTURE_2D, last_texture);
        }
        #pragma endregion
    }

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

}
