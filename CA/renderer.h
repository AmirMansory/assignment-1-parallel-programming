/**
 * renderer.h
 * ----------------------------------------------------------------------------
 * SDL2-based renderer interface.
 * Same API as before — only the implementation changed from raylib to SDL2.
 */
#pragma once

#include "simulation.h"

void init_renderer(int width, int height);
void cleanup_renderer();

void render_frame(const Particle* particles, int count,
                  int width, int height,
                  ComputeMode mode, double update_ms, int fps);
