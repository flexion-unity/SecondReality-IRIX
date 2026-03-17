#include "Graphics.h"

#ifdef __sgi
static int g_texW = 1024;
static int g_texH = 512;
// SGI IRIX: use desktop OpenGL 1.3 fixed-function pipeline (no shaders)
#include <GL/gl.h>
// GL_BGRA / GL_CLAMP_TO_EDGE are in the OpenGL 1.2 spec; define them if the
// SGI header omits them.
#ifndef GL_BGRA
#  define GL_BGRA 0x80E1
#endif
#ifndef GL_CLAMP_TO_EDGE
#  define GL_CLAMP_TO_EDGE 0x812F
#endif
#else // !__sgi
#define GL_GLEXT_PROTOTYPES 1
#include <GLES3/gl3.h>
#include <SDL_opengles2.h>
#endif // __sgi

#include <SDL.h>

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include "Music/audioPlayer.h"

struct Float4
{
    float x{}, y{}, z{}, w{};
};

struct Float3
{
    float x{}, y{}, z{};
};

struct Float2
{
    float u{}, v{};
};

struct Vertex
{
    Float3 Pos{};
    Float2 Tex{};
};

struct CBChangesEveryFrame
{
    Float4 ratio{};
    Float4 JSSS{};
};

float g_width = 640.0f;
float g_height = 400.0f;
float g_sourceWidth = 1.0f;
float g_sourceHeight = 1.0f;
float g_jsss = 0.80f;

int g_demoScreenWidth = g_width; // SCREEN_WIDTH;
int g_demoScreenHeight = g_height; // SCREEN_HEIGHT;

unsigned int g_screen32[VIRTUAL_SCREEN_WIDTH * VIRTUAL_SCREEN_HEIGHT]{};

#ifdef _DEBUG
const float ClearColor[4] = { 48.0f / 255.0f, 48.0f / 255.0f, 48.0f / 255.0f, 1.0f };
#else
const float ClearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
#endif // _DEBUG

bool g_wantsToQuit{};

namespace
{

    SDL_Window * g_windows = nullptr;
    SDL_GLContext g_context = nullptr;

    GLuint g_texture = 0;

#ifndef __sgi
    // GLES2/3 path: shader program, VBO, sampler objects
    GLuint g_prog = 0;
    GLint g_Loc_uResolution = -1;
    GLint g_Loc_uTex0 = -1;
    GLint g_Loc_uTex1 = -1;
    GLuint g_VBO = 0;
    GLint g_Loc_aPos = -1;
    GLint g_Loc_aUV = -1;
    GLuint g_linearSampling = 0;
    GLuint g_nearestSampling = 0;
#endif // !__sgi
}

static void die(const char * msg)
{
    std::fprintf(stderr, "FATAL: %s\n", msg);
    std::exit(1);
}

#ifndef __sgi // shader compilation not needed for IRIX fixed-function path

static GLuint compileShader(GLenum type, const char * src)
{
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    GLint ok = GL_FALSE;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok)
    {
        GLint len = 0;
        glGetShaderiv(s, GL_INFO_LOG_LENGTH, &len);
        std::string log(len, '\0');
        glGetShaderInfoLog(s, len, nullptr, log.data());
        std::fprintf(stderr, "Shader compile error:\n%s\n", log.c_str());
        die("compileShader failed");
    }
    return s;
}

static GLuint linkProgram(GLuint vs, GLuint fs)
{
    GLuint p = glCreateProgram();
    glAttachShader(p, vs);
    glAttachShader(p, fs);

    glBindAttribLocation(p, 0, "aPos");
    glBindAttribLocation(p, 1, "aUV");

    glLinkProgram(p);
    GLint ok = GL_FALSE;
    glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok)
    {
        GLint len = 0;
        glGetProgramiv(p, GL_INFO_LOG_LENGTH, &len);
        std::string log(len, '\0');
        glGetProgramInfoLog(p, len, nullptr, log.data());
        std::fprintf(stderr, "Program link error:\n%s\n", log.c_str());
        die("linkProgram failed");
    }

    return p;
}

#endif // !__sgi

GLuint makeTextureRGBA8(int w, int h)
{
    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

#ifdef __sgi
    // On IRIX there are no sampler objects (GL 3.3+), so set filtering
    // directly on the texture object.  The palette stores bytes as [B,G,R,0],
    // so use GL_BGRA (available since OpenGL 1.2) for the pixel format.
   // IRIX GL 1.2 requires power-of-two texture dimensions
    int texW = 1, texH = 1;
    while (texW < w) texW <<= 1;
    while (texH < h) texH <<= 1;

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    std::vector<unsigned char> zeros(texW * texH * 4, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texW, texH, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, zeros.data());

#else
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
#endif

    return tex;
}

void UpdateTextureRGBA8(GLuint tex, int w, int h, const void * pixels)
{
    glBindTexture(GL_TEXTURE_2D, tex);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

#ifdef __sgi
    // Upload only the valid region — texture is power-of-two sized
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h,
                    GL_RGBA, GL_UNSIGNED_BYTE, pixels);
#else
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h,
                    GL_RGBA, GL_UNSIGNED_BYTE, pixels);
#endif
}

static void initGL()
{

#ifdef __sgi

    // Initialize all palette entries to opaque black.
    // Zero-initialized palette has alpha=0, making every pixel fully
    // transparent and compositing against the white X11 window background.
   for (int i = 0; i < PaletteColorCount; i++)
    {
        unsigned char * p = reinterpret_cast<unsigned char *>(&Shim::palette[i]);
        p[0] = 0; p[1] = 0; p[2] = 0; p[3] = 0xFF;
    }
    
    // IRIX: OpenGL 1.3 fixed-function setup. no shaders, no VBOs, no samplers
    g_texture = makeTextureRGBA8(VIRTUAL_SCREEN_WIDTH, VIRTUAL_SCREEN_HEIGHT);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_TEXTURE_2D);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    glClearColor(ClearColor[0], ClearColor[1], ClearColor[2], ClearColor[3]);

#else // !__sgi — GLES2/3 shader path

    const char * vertexShader = "precision mediump float;\n"
                                "attribute vec2 aPos;\n"
                                "attribute vec2 aUV;\n"
                                "uniform   vec3 uResolution;\n"
                                "varying   vec2 vUV;\n"
                                "void main(){\n"
                                "  vec2 pos = (aPos / uResolution.xy) * 2.0 - 1.0;\n"
                                "  pos.y = -pos.y;\n"
                                "  gl_Position = vec4(pos, 0.0, 1.0);\n"
                                "  vUV = aUV;\n"
                                "}\n";

    const char * fragmentShader = "precision mediump float;\n"
                                  "varying vec2 vUV;\n"
                                  "uniform vec3 uResolution;\n"
                                  "uniform sampler2D uTex0;\n"
                                  "uniform sampler2D uTex1;\n"
                                  "void main(){\n"
                                  "  gl_FragColor = vec4(mix(texture2D(uTex0, vUV).bgr, texture2D(uTex1, vUV).bgr, uResolution.z),1);\n"
                                  "}\n";

    GLuint vs = compileShader(GL_VERTEX_SHADER, vertexShader);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fragmentShader);
    g_prog = linkProgram(vs, fs);
    glDeleteShader(vs);
    glDeleteShader(fs);

    g_Loc_uResolution = glGetUniformLocation(g_prog, "uResolution");
    g_Loc_uTex0 = glGetUniformLocation(g_prog, "uTex0");
    g_Loc_uTex1 = glGetUniformLocation(g_prog, "uTex1");
    g_Loc_aPos = 0;
    g_Loc_aUV = 1;

    glGenBuffers(1, &g_VBO);

    g_texture = makeTextureRGBA8(VIRTUAL_SCREEN_WIDTH, VIRTUAL_SCREEN_HEIGHT);

    glGenSamplers(1, &g_linearSampling);
    glGenSamplers(1, &g_nearestSampling);

    glSamplerParameteri(g_linearSampling, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glSamplerParameteri(g_linearSampling, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glSamplerParameteri(g_nearestSampling, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glSamplerParameteri(g_nearestSampling, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glSamplerParameteri(g_linearSampling, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glSamplerParameteri(g_linearSampling, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glSamplerParameteri(g_nearestSampling, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glSamplerParameteri(g_nearestSampling, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glUseProgram(g_prog);
    glUniform1i(g_Loc_uTex0, 0);
    glUniform1i(g_Loc_uTex1, 1);
    glUseProgram(0);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    glClearColor(ClearColor[0], ClearColor[1], ClearColor[2], ClearColor[3]);

#endif // __sgi
}

static void Render()
{
#ifdef __sgi
    // UV must address only the content portion of the power-of-two texture
    float uMax = (float)VIRTUAL_SCREEN_WIDTH  / g_texW;  // 640/1024
    float vMax = (float)VIRTUAL_SCREEN_HEIGHT / g_texH;  // 400/512
    g_sourceWidth  = uMax * (static_cast<float>(g_demoScreenWidth)  / VIRTUAL_SCREEN_WIDTH);
    g_sourceHeight = vMax * (static_cast<float>(g_demoScreenHeight) / VIRTUAL_SCREEN_HEIGHT);
#endif

    glClearColor(ClearColor[0], ClearColor[1], ClearColor[2], ClearColor[3]);

    glViewport(0, 0, static_cast<GLsizei>(g_width), static_cast<GLsizei>(g_height));

#ifdef __sgi
    // Use float ratio so non-integer window sizes (e.g. 640x350) scale correctly
    const float ratioX = g_width  / static_cast<float>(SCREEN_WIDTH);
    const float ratioY = g_height / static_cast<float>(SCREEN_HEIGHT);
    const float ratio  = fmax(1.0f, fmin(ratioX, ratioY));
#else
    const int ratioX = static_cast<int>(g_width / SCREEN_WIDTH);
    const int ratioY = static_cast<int>(g_height / SCREEN_HEIGHT);
    const float ratio = fmax(1.0f, static_cast<float>(fmin(ratioX, ratioY)));
#endif

    const float w = ratio * SCREEN_WIDTH;
    const float h = ratio * SCREEN_HEIGHT;

    const float x = (g_width - w) * 0.5f;
    const float y = (g_height - h) * 0.5f;

#ifdef __sgi
    // Resize the window to exactly match the render quad — no black bars.
    // Runs at most once per mode change; stable because w==g_width on next frame.
    if (static_cast<int>(w) != static_cast<int>(g_width) ||
        static_cast<int>(h) != static_cast<int>(g_height))
    {
        g_width  = w;
        g_height = h;
        SDL_SetWindowSize(g_windows, static_cast<int>(w), static_cast<int>(h));
        SDL_SetWindowPosition(g_windows, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
        glViewport(0, 0, static_cast<GLsizei>(g_width), static_cast<GLsizei>(g_height));
    }
#endif

    glClear(GL_COLOR_BUFFER_BIT);

#ifdef __sgi
    // IRIX: fixed-function pipeline, no shaders, no VBOs.
    // Set up an orthographic projection matching SDL window coordinates
    // (origin top-left, y increases downward) and draw a textured quad.
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, g_width, g_height, 0.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, g_texture);

    glBegin(GL_QUADS);
        glTexCoord2f(0.0f,          0.0f);          glVertex2f(x,     y);
        glTexCoord2f(g_sourceWidth, 0.0f);          glVertex2f(x + w, y);
        glTexCoord2f(g_sourceWidth, g_sourceHeight); glVertex2f(x + w, y + h);
        glTexCoord2f(0.0f,          g_sourceHeight); glVertex2f(x,     y + h);
    glEnd();

#else // !__sgi — GLES2/3 shader path

    const GLfloat verts[] = {
        x,     y,     0.0f,          0.0f,           // top-left
        x,     y + h, 0.0f,          g_sourceHeight, // bottom-left
        x + w, y,     g_sourceWidth, 0.0f,           // top-right
        x + w, y + h, g_sourceWidth, g_sourceHeight  // bottom-right
    };

    glBindBuffer(GL_ARRAY_BUFFER, g_VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_DYNAMIC_DRAW);

    glUseProgram(g_prog);

    glUniform3f(g_Loc_uResolution, g_width, g_height, g_jsss);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, g_texture);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, g_texture);

    glBindSampler(0, g_linearSampling);
    glBindSampler(1, g_nearestSampling);

    glBindBuffer(GL_ARRAY_BUFFER, g_VBO);
    glEnableVertexAttribArray(g_Loc_aPos);
    glEnableVertexAttribArray(g_Loc_aUV);
    glVertexAttribPointer(g_Loc_aPos, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (const void *)(0));
    glVertexAttribPointer(g_Loc_aUV, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (const void *)(2 * sizeof(GLfloat)));

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glDisableVertexAttribArray(g_Loc_aPos);
    glDisableVertexAttribArray(g_Loc_aUV);
    glUseProgram(0);

#endif // __sgi

    SDL_GL_SwapWindow(g_windows);
}


bool is_macos()
{
    return false;
}
bool is_firefox()
{
    return false;
}
void InitSizesAndLlisteners() {}
void InstallClickAnywhereFullscreen() {}

bool Graphics::Init(WindowType _fullscreen)
{
    SDL_DisableScreenSaver();

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0)
    {
        std::fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

#ifdef __sgi
    // Request a desktop OpenGL 1.3 context (no profile mask = legacy/compatibility)
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
#else
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    InitSizesAndLlisteners();
    InstallClickAnywhereFullscreen();

/*    
#ifdef __sgi
    g_height = 350.0f;  // start at 640x350 — SR's canonical hi-res on IRIX
#endif
*/

    g_windows = SDL_CreateWindow("Second Reality - IRIX", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, g_width, g_height, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

    if (!g_windows)
    {
        die("SDL_CreateWindow failed");
    }

    if (g_context = SDL_GL_CreateContext(g_windows); !g_context)
    {
        die("SDL_GL_CreateContext failed");
    }

    SDL_GL_MakeCurrent(g_windows, g_context);
    SDL_GL_SetSwapInterval(0);

#ifndef __sgi
   // SDL_MaximizeWindow(g_windows);

    if (_fullscreen == WindowType::Fullscreen)
    {
        SDL_SetWindowFullscreen(g_windows, SDL_WINDOW_FULLSCREEN_DESKTOP);
        SDL_ShowCursor(SDL_DISABLE);
    }
#else
    if (_fullscreen == WindowType::Fullscreen)
    {
        SDL_ShowCursor(SDL_DISABLE);
        SDL_DisplayMode dm;
        if (SDL_GetCurrentDisplayMode(0, &dm) == 0)
        {
            SDL_SetWindowSize(g_windows, dm.w, dm.h);
            SDL_SetWindowPosition(g_windows, 0, 0);
            g_width  = static_cast<float>(dm.w);
            g_height = static_cast<float>(dm.h);
        }
    }
    // Re-bind after all window geometry changes are done
    SDL_GL_MakeCurrent(g_windows, g_context);
#endif
    initGL();

    return true;
}

void Graphics::Update()
{
    SDL_Event ev;
    while (SDL_PollEvent(&ev))
    {
        if ((ev.type == SDL_QUIT) || (ev.type == SDL_KEYUP && ev.key.keysym.sym == SDLK_ESCAPE))
        {
            g_wantsToQuit = true;
        }

        if (ev.type == SDL_WINDOWEVENT)
        {
            if (ev.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
            {
                g_width = ev.window.data1;
                g_height = ev.window.data2;
            }
        }
    }


    PrepareTextureForGPU();
    UpdateTextureRGBA8(g_texture, VIRTUAL_SCREEN_WIDTH, VIRTUAL_SCREEN_HEIGHT, g_screen32);

    Render();

    AudioPlayer::Update(true);
}

void Graphics::Close()
{

#ifndef __sgi
    // Sampler objects and VBOs are GLES3/GL3.3+ features; not available on IRIX
    if (g_linearSampling) glDeleteSamplers(1, &g_linearSampling);
    if (g_nearestSampling) glDeleteSamplers(1, &g_nearestSampling);
    if (g_VBO) glDeleteBuffers(1, &g_VBO);
    if (g_prog) glDeleteProgram(g_prog);
#endif // !__sgi

    if (g_texture) glDeleteTextures(1, &g_texture);

    SDL_GL_DeleteContext(g_context);
    SDL_DestroyWindow(g_windows);
    SDL_Quit();


    SDL_EnableScreenSaver();
}


bool Graphics::WantsToQuit()
{
    return g_wantsToQuit;
}

void Graphics::PrepareTextureForGPU()
{
    if (g_demoScreenWidth == SCREEN_WIDTH && g_demoScreenHeight == SCREEN_HEIGHT)
    {
        unsigned char * src = Shim::vram + Shim::startpixel;
        unsigned int * dst = g_screen32;
        for (int y = 0; y < g_demoScreenHeight; y++)
        {
            for (int x = 0; x < g_demoScreenWidth; x++)
            {
                unsigned int c = Shim::palette[*src++];
                *dst++ = c;
            }

            dst += VIRTUAL_SCREEN_WIDTH - g_demoScreenWidth;
        }
    }
    else if (g_demoScreenWidth == SCREEN_WIDTH && g_demoScreenHeight == DOUBlE_SCREEN_HEIGHT)
    {
        unsigned char * src = Shim::vram + Shim::startpixel;
        unsigned int * dst = g_screen32;
        for (int y = 0; y < g_demoScreenHeight; y++)
        {
            for (int x = 0; x < g_demoScreenWidth; x++)
            {
                unsigned int c = Shim::palette[*src++];
                *dst++ = c;
            }

            dst += VIRTUAL_SCREEN_WIDTH - g_demoScreenWidth;
        }
    }
    else if (g_demoScreenWidth == 640 && g_demoScreenHeight == 350)
    {
        unsigned char * src = Shim::vram + Shim::startpixel;
        unsigned int * dst = g_screen32 + VIRTUAL_SCREEN_WIDTH * (VIRTUAL_SCREEN_HEIGHT - 350) / 2;
        for (int y = 0; y < g_demoScreenHeight; y++)
        {
            for (int x = 0; x < g_demoScreenWidth; x++)
            {
                *dst++ = Shim::palette[*src++];
            }
        }
    }

    g_sourceWidth = static_cast<float>(g_demoScreenWidth) / VIRTUAL_SCREEN_WIDTH;
    g_sourceHeight = static_cast<float>(g_demoScreenHeight) / VIRTUAL_SCREEN_HEIGHT;
}

void Graphics::ChangeMode(int x, int y, float jsss)
{
    g_demoScreenWidth = x;
    g_demoScreenHeight = y;

    g_jsss = jsss;

    Common::reset();

    CLEAR(g_screen32);

    Shim::startpixel = 0;
}
