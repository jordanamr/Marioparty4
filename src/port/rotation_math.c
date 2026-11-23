#include "port/rotation_kernels.h"

// Matrix rotations feed the next simulation tick (e.g. the fish in Manta Rings).
// Evaluate both components using the same reduction and coefficients on every desktop.
void PartyBoard_RotationSinCos(float radians, float *sine, float *cosine)
{
    double angle[2];
    int quadrant = __ieee754_rem_pio2(radians, angle) & 3;
    double s = __kernel_sin(angle[0], angle[1], angle[1] != 0.0);
    double c = __kernel_cos(angle[0], angle[1]);
    const double cycle[4] = {s, c, -s, -c};
    *sine = (float)cycle[quadrant];
    *cosine = (float)cycle[(quadrant + 1) & 3];
}
