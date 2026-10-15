/**
 * simulation_cpu.cpp
 * ----------------------------------------------------------------------------
 * CPU implementations of the N-body bullet-impact simulation.
 *
 *   update_scalar()  — baseline double loop with 1/sqrt
 *   update_openmp()  — parallel outer loop, race-free (each i writes only p[i])
 *   update_simd()    — STUB (students implement AVX2)
 *
 * Physics per frame, per particle:
 *   1. N-body gravity (O(n²)) — the heavy part
 *   2. Downward gravity + air drag
 *   3. Position integration + wall/ground bounce
 *   4. Lifetime decay + colour cooling
 */
#include "simulation.h"

#include <cmath>
#include <cstdlib>

#ifdef _OPENMP
#  include <omp.h>
#endif

// ===========================================================================
// Helpers
// ===========================================================================

const char* mode_name(ComputeMode mode)
{
    switch (mode) {
        case ComputeMode::SCALAR: return "SCALAR";
        case ComputeMode::SIMD:   return "SIMD (AVX2)";
        case ComputeMode::OPENMP: return "OpenMP";
#ifdef SIMULATION_CPU_ONLY
        case ComputeMode::CUDA:   return "CUDA (CPU fallback)";
#else
        case ComputeMode::CUDA:   return "CUDA";
#endif
    }
    return "UNKNOWN";
}

static float rand_range(float lo, float hi)
{
    return lo + static_cast<float>(rand()) / RAND_MAX * (hi - lo);
}

void init_particles(Particle* p, int count,
                    float cx, float cy, float power)
{
    for (int i = 0; i < count; ++i) {
        float angle = rand_range(0.0f, 6.2831853f);
        float speed = rand_range(30.0f, power);

        p[i].x        = cx;
        p[i].y        = cy;
        p[i].vx       = speed * cosf(angle);
        p[i].vy       = speed * sinf(angle);
        p[i].mass     = rand_range(0.5f, 2.5f);
        p[i].lifetime = rand_range(4.0f, MAX_LIFE);
        p[i].r = p[i].g = p[i].b = 1.0f;
    }
}

// ===========================================================================
// Per-frame N-body update helpers
// ===========================================================================

/// Process one particle's N-body forces (inner loop over all j).
/// Called by every backend so physics is identical.

static inline void nbody_forces(Particle* p, int count, float dt, int i)
{
    float ax = 0.0f;
    float ay = 0.0f;
    float xi = p[i].x;
    float yi = p[i].y;

    for (int j = 0; j < count; ++j) {
        if (i == j) continue; 
        float dx = p[j].x - xi;
        float dy = p[j].y - yi;
        float dist_sq = (dx * dx) + (dy * dy) + SOFTENING;
        float inv_dist = 1.0f / sqrtf(dist_sq);
        float inv_dist3 = inv_dist * inv_dist * inv_dist;
        float F = G_CONST * p[j].mass * inv_dist3;
        ax += dx * F;
        ay += dy * F;
    }

    p[i].vx += ax * dt;
    p[i].vy += ay * dt;
}

/// Apply gravity, drag, position, bounds, lifetime, colour to one particle.
static inline void apply_physics(Particle* p, float dt,
                                 int width, int height, int i)
{
    // Apply downward gravity

    // Apply air drag (dampens velocity) 

    //update position based on velocity

    // Bounce off walls / ground with energy loss


    p[i].lifetime -= dt;
    if (p[i].lifetime < 0.0f) p[i].lifetime = 0.0f;

    // Colour: rainbow mapped by speed, fades with lifetime
    // Fast particles = blue/violet, medium = green/yellow, slow = red/orange
    float spd2 = p[i].vx * p[i].vx + p[i].vy * p[i].vy;
    float t    = spd2 / 200000.0f;
    if (t > 1.0f) t = 1.0f;
    float life = p[i].lifetime / MAX_LIFE;

    float r, g, b;
    if (t < 0.25f) {
        
    } else if (t < 0.5f) {
    
    } else if (t < 0.75f) {

    } else {
        
    }

    // Add per-particle hue variation based on index (rainbow explosion!)
    float hueShift = static_cast<float>(i) * 0.382f;  // golden ratio offset
    float cs = cosf(hueShift);
    float sn = sinf(hueShift);
    // Rotate hue slightly based on particle index
    float rr = r * cs - g * sn;
    float gg = r * sn + g * cs;
    if (rr < 0.0f) rr = 0.0f;
    if (gg < 0.0f) gg = 0.0f;

    p[i].r = rr * life;
    p[i].g = gg * life;
    p[i].b = b  * life;
}

// ===========================================================================
// 1. SCALAR  (reference baseline)
// ===========================================================================

void update_scalar(Particle* p, int count, float dt, int width, int height)
{
    // N-body  (O(n²) — the heavy part)
    for (int i = 0; i < count; ++i)
        nbody_forces(p, count, dt, i);

    // Per-particle physics
    for (int i = 0; i < count; ++i)
        apply_physics(p, dt, width, height, i);
}

// ===========================================================================
// 2. SIMD  (STUB — students implement AVX2 vectorisation of inner loop)
// ===========================================================================

void update_simd(Particle* p, int count, float dt, int width, int height)
{
    // TODO: vectorise nbody_forces() with _mm256_rsqrt_ps() etc.
    update_scalar(p, count, dt, width, height);
}

// ===========================================================================
// 3. OPENMP  (parallel outer loop)
// ===========================================================================

void update_openmp(Particle* p, int count, float dt, int width, int height)
{
    // N-body: parallel outer loop. Each thread writes only to p[i],
    // so no race conditions. Inner loop reads all p[j] (read-only).
    update_scalar(p, count, dt, width, height);
}

// ===========================================================================
// Dispatch
// ===========================================================================

void update_particles(Particle* particles, int count, float dt,
                      int width, int height, ComputeMode mode)
{
    switch (mode) {
        case ComputeMode::SCALAR: update_scalar(particles, count, dt, width, height); break;
        case ComputeMode::SIMD:   update_simd  (particles, count, dt, width, height); break;
        case ComputeMode::OPENMP: update_openmp(particles, count, dt, width, height); break;
        case ComputeMode::CUDA:   update_cuda  (particles, count, dt, width, height); break;
    }
}
