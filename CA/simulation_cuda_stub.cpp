#include "simulation.h"

void init_cuda(int /*count*/)
{
}

void cleanup_cuda()
{
}

void update_cuda(Particle* particles, int count, float dt,
                 int width, int height)
{
    update_scalar(particles, count, dt, width, height);
}
