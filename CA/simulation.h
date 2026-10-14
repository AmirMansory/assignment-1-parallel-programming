/**
 * simulation.h
 * ----------------------------------------------------------------------------
 * N-body bullet-impact shrapnel simulation.
 *
 * Each particle explodes from the impact point and is affected by:
 *   - Gravity (downward)
 *   - Air drag
 *   - N-body mutual gravitational attraction (the heavy O(n²) computation)
 *   - Ground / wall bounce with energy loss
 *
 * The 1/sqrt() in the N-body inner loop makes scalar genuinely slow,
 * so OpenMP / SIMD / CUDA speedups are large and obvious.
 */
#pragma once

#include <cstdint>

// ---------------------------------------------------------------------------
// Data model
// ---------------------------------------------------------------------------

struct Particle {
    float x, y;       // position (pixels)
    float vx, vy;     // velocity (pixels/second)
    float mass;       // affects N-body pull (1.0 = default)
    float lifetime;   // seconds remaining (fades to black)
    float r, g, b;    // colour (hot → cool as energy drops)
};

// ---------------------------------------------------------------------------
// Physics constants
// ---------------------------------------------------------------------------

static constexpr float G_CONST   = 800.0f;    // N-body gravitational constant
static constexpr float SOFTENING = 16.0f;     // prevents singularity at r=0
static constexpr float GRAVITY   = 400.0f;    // pixels/s² downward
static constexpr float DRAG      = 0.15f;     // air resistance coefficient
static constexpr float BOUNCE    = 0.35f;     // energy retained on bounce
static constexpr float MAX_LIFE  = 12.0f;     // particle lifetime (seconds)

// ---------------------------------------------------------------------------
// Compute backend selection
// ---------------------------------------------------------------------------

enum class ComputeMode : uint8_t {
    SCALAR = 0,
    SIMD   = 1,
    OPENMP = 2,
    CUDA   = 3,
};

const char* mode_name(ComputeMode mode);

// ---------------------------------------------------------------------------
// Particle initialisation
// ---------------------------------------------------------------------------

/// Spawns an explosion of `count` particles at (cx, cy) with the given power.
/// Particles get random outward velocities, random masses, and full lifetime.
void init_particles(Particle* particles, int count,
                    float cx, float cy, float power);

// ---------------------------------------------------------------------------
// Per-backend update functions
// ---------------------------------------------------------------------------

void update_scalar(Particle* p, int count, float dt, int width, int height);
void update_simd  (Particle* p, int count, float dt, int width, int height);
void update_openmp(Particle* p, int count, float dt, int width, int height);
void update_cuda  (Particle* p, int count, float dt, int width, int height);

// ---------------------------------------------------------------------------
// CUDA resource management
// ---------------------------------------------------------------------------

void init_cuda(int count);
void cleanup_cuda();

// ---------------------------------------------------------------------------
// Unified dispatch
// ---------------------------------------------------------------------------

void update_particles(Particle* particles, int count, float dt,
                      int width, int height, ComputeMode mode);
