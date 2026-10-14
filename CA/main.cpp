/**
 * main.cpp
 * ----------------------------------------------------------------------------
 * SDL2 main loop for the bullet-impact N-body simulation.
 *
 * Left-click anywhere to fire a "bullet" — spawns an explosion of shrapnel
 * particles at that point.  Watch them cluster via N-body gravity, arc under
 * real gravity, and bounce off the ground.
 *
 * Press 1-4 to switch compute backends and see the speedup in the HUD.
 */
#include "simulation.h"
#include "renderer.h"

#include <SDL.h>

#include <cstdio>
#include <cstdlib>
#include <vector>

// ---------------------------------------------------------------------------
// Configuration
// ---------------------------------------------------------------------------
static constexpr int   WINDOW_W      = 1280;
static constexpr int   WINDOW_H      = 720;
static constexpr int   PARTICLE_MIN  = 50;
static constexpr int   PARTICLE_MAX  = 5000;
static constexpr int   DEFAULT_COUNT = 500;
static constexpr float FIRE_POWER    = 600.0f;   // max initial shrapnel speed

// ---------------------------------------------------------------------------
// Entry point
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
// ASCII art startup banner
// ---------------------------------------------------------------------------
static void print_banner()
{
    printf("\n");
    printf("  \033[1;33m*    *   * * *   *   *   * * *   *   *   * * *   *   *\033[0m\n");
    printf("  \033[1;33m*    *   *   *   **  *   * * *   **  *   *       **  *\033[0m\n");
    printf("  \033[1;31m* ** *   * * *   * * *   * *     * * *   *  **   * * *\033[0m\n");
    printf("  \033[1;31m**  **   *   *   *  **   * * *   *  **   *   *   *  **\033[0m\n");
    printf("  \033[1;35m*    *   * * *   *   *   * * *   *   *   * * *   *   *\033[0m\n");
    printf("\n");
    printf("  \033[1;36m====================================================\033[0m\n");
    printf("  \033[1;36m|       N-Body Shrapnel Simulation v2.0           |\033[0m\n");
    printf("  \033[1;36m|    Watch particles dance under gravity & N-body! |\033[0m\n");
    printf("  \033[1;36m====================================================\033[0m\n");
    printf("\n");
    printf("  \033[0;32m  TIP: Left-click to create an explosion!  \033[1;33m(*^_^*)\033[0m\n");
    printf("\n");
}

int main(int /*argc*/, char* /*argv*/[])
{
    srand(42);

    print_banner();

    init_renderer(WINDOW_W, WINDOW_H);

    // --- Initial explosion at centre -----------------------------------
    int  particle_count = DEFAULT_COUNT;
    std::vector<Particle> particles(particle_count);
    init_particles(particles.data(), particle_count,
                   WINDOW_W / 2.0f, WINDOW_H / 2.0f, FIRE_POWER);

    // --- State ---------------------------------------------------------
    ComputeMode mode      = ComputeMode::SCALAR;
    double      update_ms = 0.0;
    bool        running   = true;

    Uint64 perfFreq  = SDL_GetPerformanceFrequency();
    Uint32 lastTime  = SDL_GetTicks();
    int    fps       = 0;
    int    frameCnt  = 0;
    Uint32 fpsTimer  = lastTime;

    // --- Main loop -----------------------------------------------------
    while (running) {
        // ---- Input -------------------------------------------------
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            switch (ev.type) {
            case SDL_QUIT:
                running = false;
                break;

            case SDL_MOUSEBUTTONDOWN:
                if (ev.button.button == SDL_BUTTON_LEFT) {
                    float mx = static_cast<float>(ev.button.x);
                    float my = static_cast<float>(ev.button.y);
                    init_particles(particles.data(), particle_count,
                                   mx, my, FIRE_POWER);
                    static const char* fire_emoji[] = {
                        "\033[1;31m[ BOOM! ]\033[0m",
                        "\033[1;33m[ KABOOM!!! ]\033[0m",
                        "\033[1;35m[ BLAST OFF! ]\033[0m",
                        "\033[1;31m[ FIRE IN THE HOLE! ]\033[0m",
                        "\033[1;33m[ EXPLOSION!!! ]\033[0m",
                    };
                    int idx = rand() % 5;
                    printf("%s  Impact at (%.0f, %.0f) — %d shrapnel particles unleashed!  \033[1;32m(>_<)b\033[0m\n",
                           fire_emoji[idx], mx, my, particle_count);
                }
                break;

            case SDL_KEYDOWN:
                switch (ev.key.keysym.sym) {
                case SDLK_ESCAPE:
                    running = false;
                    break;
                case SDLK_1: mode = ComputeMode::SCALAR; break;
                case SDLK_2: mode = ComputeMode::SIMD;   break;
                case SDLK_3: mode = ComputeMode::OPENMP; break;
                case SDLK_4: mode = ComputeMode::CUDA;   break;
                case SDLK_r:
                    init_particles(particles.data(), particle_count,
                                   WINDOW_W / 2.0f, WINDOW_H / 2.0f,
                                   FIRE_POWER);
                    break;
                case SDLK_KP_PLUS:
                case SDLK_EQUALS:
                    if (particle_count < PARTICLE_MAX) {
                        particle_count = (particle_count * 3) / 2;
                        if (particle_count > PARTICLE_MAX)
                            particle_count = PARTICLE_MAX;
                        particles.resize(particle_count);
                        init_particles(particles.data(), particle_count,
                                       WINDOW_W / 2.0f, WINDOW_H / 2.0f,
                                       FIRE_POWER);
                        printf("  \033[1;32m[ + ]\033[0m Particles boosted to %d!  \033[0;33m(^_^)v\033[0m\n", particle_count);
                    }
                    break;
                case SDLK_KP_MINUS:
                case SDLK_MINUS:
                    if (particle_count > PARTICLE_MIN) {
                        particle_count = (particle_count * 2) / 3;
                        if (particle_count < PARTICLE_MIN)
                            particle_count = PARTICLE_MIN;
                        particles.resize(particle_count);
                        init_particles(particles.data(), particle_count,
                                       WINDOW_W / 2.0f, WINDOW_H / 2.0f,
                                       FIRE_POWER);
                        printf("  \033[1;34m[ - ]\033[0m Particles trimmed to %d  \033[0;33m(o_o)\033[0m\n", particle_count);
                    }
                    break;
                }
                break;
            }
        }

        // ---- Timing ------------------------------------------------
        Uint32 now = SDL_GetTicks();
        float  dt  = (now - lastTime) / 1000.0f;
        lastTime   = now;
        if (dt > 0.1f) dt = 0.016f;

        // ---- Compute step (timed) ----------------------------------
        Uint64 t0 = SDL_GetPerformanceCounter();
        update_particles(particles.data(), particle_count, dt,
                         WINDOW_W, WINDOW_H, mode);
        Uint64 t1 = SDL_GetPerformanceCounter();
        update_ms = static_cast<double>(t1 - t0) * 1000.0 / perfFreq;

        // ---- FPS counter -------------------------------------------
        ++frameCnt;
        if (now - fpsTimer >= 1000) {
            fps      = frameCnt;
            frameCnt = 0;
            fpsTimer = now;
        }

        // ---- Render ------------------------------------------------
        render_frame(particles.data(), particle_count,
                     WINDOW_W, WINDOW_H,
                     mode, update_ms, fps);
    }

    // --- Cleanup --------------------------------------------------------
    cleanup_cuda();
    cleanup_renderer();
    return 0;
}
