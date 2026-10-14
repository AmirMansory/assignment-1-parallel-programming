/**
 * renderer.cpp
 * ----------------------------------------------------------------------------
 * SDL2-based renderer for the N-body bullet-impact simulation.
 *
 *   - Pixel buffer + streaming texture (single draw call per frame)
 *   - Per-particle colour from simulation (hot → cool fade)
 *   - Ground line visual
 *   - HUD with mode / timing / FPS / controls
 */
#include "renderer.h"

#include <SDL.h>
#include <SDL_ttf.h>

#include <cstdio>
#include <cstdlib>
#include <vector>
#include <string>

// ---------------------------------------------------------------------------
// Internal state
// ---------------------------------------------------------------------------

static SDL_Window*   g_window      = nullptr;
static SDL_Renderer* g_renderer    = nullptr;
static SDL_Texture*  g_particleTex = nullptr;
static Uint8*        g_pixelBuf    = nullptr;
static int           g_bufW        = 0;
static int           g_bufH        = 0;

static TTF_Font*     g_font        = nullptr;
static TTF_Font*     g_fontSmall   = nullptr;

// Starfield
struct Star { float x, y; Uint8 bright; };
static std::vector<Star> g_stars;

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void init_renderer(int width, int height)
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError()); exit(1);
    }
    if (TTF_Init() != 0) {
        fprintf(stderr, "TTF_Init: %s\n", TTF_GetError()); exit(1);
    }

    g_window = SDL_CreateWindow(
        "Bullet Impact — N-Body Shrapnel Simulation",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        width, height, SDL_WINDOW_SHOWN);
    if (!g_window) {
        fprintf(stderr, "SDL_CreateWindow: %s\n", SDL_GetError()); exit(1);
    }

    g_renderer = SDL_CreateRenderer(g_window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!g_renderer) {
        fprintf(stderr, "SDL_CreateRenderer: %s\n", SDL_GetError()); exit(1);
    }

    g_particleTex = SDL_CreateTexture(g_renderer,
        SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING,
        width, height);
    if (!g_particleTex) {
        fprintf(stderr, "SDL_CreateTexture: %s\n", SDL_GetError()); exit(1);
    }

    g_bufW = width;
    g_bufH = height;
    g_pixelBuf = new Uint8[static_cast<size_t>(width) * height * 4];

    // Generate starfield (static, seeded for consistency)
    g_stars.resize(450);
    for (auto& s : g_stars) {
        s.x = static_cast<float>(rand() % width);
        s.y = static_cast<float>(rand() % height);
        s.bright = static_cast<Uint8>(30 + rand() % 100);
    }

    // Load font in a portable, configurable way:
    // 1) If the environment variable SIM_FONT is set, try that path.
    // 2) Look for ./fonts/ in the executable directory (useful for shipping a font with the project).
    // 3) Try common Windows font paths.
    // 4) Try common Linux font paths.
    // This makes it easy for Windows users to run the project by bundling a font
    // into a `fonts/` folder next to the executable, or by setting SIM_FONT.

    // Try environment override first
    const char* env_font = getenv("SIM_FONT");
    if (env_font && *env_font) {
        g_font = TTF_OpenFont(env_font, 22);
        if (g_font) g_fontSmall = TTF_OpenFont(env_font, 15);
    }

    // If not specified by env, build a candidate list relative to the executable
    if (!g_font) {
        char* basePath = SDL_GetBasePath(); // caller must free with SDL_free
        std::vector<std::string> candidates;
        if (basePath) {
            std::string base(basePath);
            SDL_free(basePath);
            // Trim trailing slash/backslash if present
            if (!base.empty() && (base.back() == '/' || base.back() == '\\')) base.pop_back();
            std::string fontsDir = base + "/fonts/";
            candidates.push_back(fontsDir + "Consolas.ttf");
            candidates.push_back(fontsDir + "Consola.ttf");
            candidates.push_back(fontsDir + "consola.ttf");
            candidates.push_back(fontsDir + "DejaVuSansMono.ttf");
            candidates.push_back(fontsDir + "DejaVuSans.ttf");
            candidates.push_back(fontsDir + "LiberationMono-Regular.ttf");
        }

        // Common Windows locations
        candidates.push_back("C:/Windows/Fonts/consola.ttf");
        candidates.push_back("C:/Windows/Fonts/lucon.ttf");
        candidates.push_back("C:/Windows/Fonts/Consola.ttf");

        // Common Linux locations
        candidates.push_back("/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf");
        candidates.push_back("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf");
        candidates.push_back("/usr/share/fonts/truetype/liberation/LiberationMono-Regular.ttf");
        candidates.push_back("/usr/share/fonts/truetype/freefont/FreeSans.ttf");

        for (const auto& fp : candidates) {
            g_font = TTF_OpenFont(fp.c_str(), 22);
            if (g_font) {
                g_fontSmall = TTF_OpenFont(fp.c_str(), 15);
                break;
            }
        }
    }

    if (!g_font) {
        fprintf(stderr, "TTF_OpenFont failed (HUD text disabled).\n"
                        "Set SIM_FONT to a valid .ttf path or place a font in ./fonts/.\n");
    }
}

void cleanup_renderer()
{
    delete[] g_pixelBuf;  g_pixelBuf = nullptr;
    if (g_font)      { TTF_CloseFont(g_font);      g_font      = nullptr; }
    if (g_fontSmall) { TTF_CloseFont(g_fontSmall); g_fontSmall = nullptr; }
    if (g_particleTex) { SDL_DestroyTexture(g_particleTex); g_particleTex = nullptr; }
    if (g_renderer)    { SDL_DestroyRenderer(g_renderer);    g_renderer    = nullptr; }
    if (g_window)      { SDL_DestroyWindow(g_window);        g_window      = nullptr; }
    TTF_Quit();
    SDL_Quit();
}

// ---------------------------------------------------------------------------
// Text helper
// ---------------------------------------------------------------------------

static int draw_text(int x, int y, const char* str, TTF_Font* font,
                     SDL_Color color)
{
    if (!font || !str) return 0;
    SDL_Surface* surf = TTF_RenderText_Blended(font, str, color);
    if (!surf) return 0;
    SDL_Texture* tex = SDL_CreateTextureFromSurface(g_renderer, surf);
    if (!tex) { SDL_FreeSurface(surf); return 0; }

    SDL_Rect dst = { x, y, surf->w, surf->h };
    SDL_RenderCopy(g_renderer, tex, nullptr, &dst);

    SDL_DestroyTexture(tex);
    SDL_FreeSurface(surf);
    return surf->h;
}

// ---------------------------------------------------------------------------
// HUD
// ---------------------------------------------------------------------------

static void draw_hud(ComputeMode mode, double update_ms, int fps,
                     int count, int width, const char* hint)
{
    // Gradient HUD bar (dark indigo → transparent)
    const int barH = 100;
    SDL_SetRenderDrawBlendMode(g_renderer, SDL_BLENDMODE_BLEND);
    for (int y = 0; y < barH; ++y) {
        float t   = 1.0f - static_cast<float>(y) / barH;
        Uint8 a   = static_cast<Uint8>(200 * t * t);
        Uint8 r   = static_cast<Uint8>(15  * t);
        Uint8 g   = static_cast<Uint8>(10  * t);
        Uint8 b   = static_cast<Uint8>(40  * t);
        SDL_SetRenderDrawColor(g_renderer, r, g, b, a);
        SDL_Rect row = { 0, y, width, 1 };
        SDL_RenderFillRect(g_renderer, &row);
    }
    // Accent line at bottom of bar
    SDL_SetRenderDrawColor(g_renderer, 120, 80, 255, 120);
    SDL_Rect accent = { 0, barH, width, 2 };
    SDL_RenderFillRect(g_renderer, &accent);
    SDL_SetRenderDrawBlendMode(g_renderer, SDL_BLENDMODE_NONE);

    SDL_Color modeCol;
    switch (mode) {
        case ComputeMode::SCALAR: modeCol = SDL_Color{255,255,100,255}; break;
        case ComputeMode::SIMD:   modeCol = SDL_Color{100,255,100,255}; break;
        case ComputeMode::OPENMP: modeCol = SDL_Color{100,200,255,255}; break;
        case ComputeMode::CUDA:   modeCol = SDL_Color{255,100,255,255}; break;
        default:                  modeCol = SDL_Color{255,255,255,255}; break;
    }

    char buf[256];

    // Title line
    snprintf(buf, sizeof(buf), "%s", mode_name(mode));
    int yOff = 10;
    draw_text(14, yOff, buf, g_font, modeCol);

    // Stats
    snprintf(buf, sizeof(buf), "%.2f ms  |  %d FPS  |  %d particles",
             update_ms, fps, count);
    yOff += 28;
    draw_text(14, yOff, buf, g_font, SDL_Color{220,220,255,255});

    // Hint line
    yOff += 26;
    draw_text(14, yOff, hint, g_fontSmall, SDL_Color{180,170,220,255});
}

// ---------------------------------------------------------------------------
// Frame render
// ---------------------------------------------------------------------------

void render_frame(const Particle* particles, int count,
                  int width, int height,
                  ComputeMode mode, double update_ms, int fps)
{
    // ---- 1. Gradient background (deep space blue → black) -----------
    {
        for (int y = 0; y < g_bufH; ++y) {
            float t = static_cast<float>(y) / g_bufH;
            Uint8 bgR = static_cast<Uint8>(4  + 6 * (1.0f - t));
            Uint8 bgG = static_cast<Uint8>(4  + 8 * (1.0f - t));
            Uint8 bgB = static_cast<Uint8>(8  + 18 * (1.0f - t));
            Uint8* row = g_pixelBuf + y * g_bufW * 4;
            for (int x = 0; x < g_bufW; ++x) {
                row[0] = bgR; row[1] = bgG; row[2] = bgB; row[3] = 255;
                row += 4;
            }
        }
    }

    // ---- 2. Starfield ------------------------------------------------
    for (const auto& s : g_stars) {
        int sx = static_cast<int>(s.x);
        int sy = static_cast<int>(s.y);
        if (sx >= 0 && sx < g_bufW && sy >= 0 && sy < g_bufH) {
            int idx = (sy * g_bufW + sx) * 4;
            int nr = g_pixelBuf[idx+0] + s.bright; if (nr > 255) nr = 255;
            int ng = g_pixelBuf[idx+1] + s.bright; if (ng > 255) ng = 255;
            int nb = g_pixelBuf[idx+2] + s.bright; if (nb > 255) nb = 255;
            g_pixelBuf[idx+0] = (Uint8)nr;
            g_pixelBuf[idx+1] = (Uint8)ng;
            g_pixelBuf[idx+2] = (Uint8)nb;
        }
    }

    // ---- 3. Ground (gradient layer) ----------------------------------
    {
        const int groundBase = g_bufH - 8;
        for (int y = groundBase; y < g_bufH; ++y) {
            float fade = static_cast<float>(y - groundBase) / 8.0f;
            float glow = (y >= g_bufH - 4) ? 1.0f : (1.0f - fade) * 0.5f;
            Uint8 gr = static_cast<Uint8>(80  * glow + (1.0f - glow) * 4);
            Uint8 gg = static_cast<Uint8>(55  * glow + (1.0f - glow) * 4);
            Uint8 gb = static_cast<Uint8>(30  * glow + (1.0f - glow) * 8);
            Uint8* row = g_pixelBuf + y * g_bufW * 4;
            for (int x = 0; x < g_bufW; ++x) {
                row[0] = gr; row[1] = gg; row[2] = gb;
                row += 4;
            }
        }
    }

    // ---- 4. Rasterise particles as glowing 5×5 spots ----------------
    // Glow kernel (Manhattan distance falloff)
    static const float glowKernel[3] = { 1.0f, 0.55f, 0.18f };
    for (int i = 0; i < count; ++i) {
        int px = static_cast<int>(particles[i].x);
        int py = static_cast<int>(particles[i].y);
        if (px < -2 || px >= g_bufW + 2 || py < -2 || py >= g_bufH + 2)
            continue;

        float cr_f = particles[i].r;
        float cg_f = particles[i].g;
        float cb_f = particles[i].b;

        for (int dy = -2; dy <= 2; ++dy) {
            int sy = py + dy;
            if (sy < 0 || sy >= g_bufH) continue;
            float ky = glowKernel[abs(dy)];
            for (int dx = -2; dx <= 2; ++dx) {
                int sx = px + dx;
                if (sx < 0 || sx >= g_bufW) continue;
                float kx = glowKernel[abs(dx)];
                float k  = ky * kx;
                int idx  = (sy * g_bufW + sx) * 4;
                int nr = g_pixelBuf[idx+0] + static_cast<int>(cr_f * 255.0f * k);
                int ng = g_pixelBuf[idx+1] + static_cast<int>(cg_f * 255.0f * k);
                int nb = g_pixelBuf[idx+2] + static_cast<int>(cb_f * 255.0f * k);
                if (nr > 255) nr = 255;
                if (ng > 255) ng = 255;
                if (nb > 255) nb = 255;
                g_pixelBuf[idx+0] = (Uint8)nr;
                g_pixelBuf[idx+1] = (Uint8)ng;
                g_pixelBuf[idx+2] = (Uint8)nb;
            }
        }
    }

    // ---- 5. Upload & draw --------------------------------------------
    SDL_UpdateTexture(g_particleTex, nullptr, g_pixelBuf, g_bufW * 4);

    SDL_RenderClear(g_renderer);
    SDL_RenderCopy(g_renderer, g_particleTex, nullptr, nullptr);

    // Controls hint
    const char* hint = "[1]Scalar [2]SIMD [3]OpenMP [4]CUDA  "
                       "[+/-]Particles  [R]Reset  [LMB]Fire  [ESC]Quit";
    draw_hud(mode, update_ms, fps, count, width, hint);

    SDL_RenderPresent(g_renderer);
}
