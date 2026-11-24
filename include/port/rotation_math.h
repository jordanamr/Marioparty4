#ifndef PARTYBOARD_ROTATION_MATH_H
#define PARTYBOARD_ROTATION_MATH_H

#ifdef PARTYBOARD_ROTATION_MATH
#ifdef __cplusplus
extern "C" {
#endif
void PartyBoard_RotationSinCos(float radians, float *sine, float *cosine);
float PartyBoard_AsinF(float x);
float PartyBoard_AcosF(float x);
float PartyBoard_Atan2F(float y, float x);
#ifdef __cplusplus
}
#endif
#else
#include <math.h>
static inline float PartyBoard_AsinF(float x) { return asinf(x); }
static inline float PartyBoard_AcosF(float x) { return acosf(x); }
static inline float PartyBoard_Atan2F(float y, float x) { return atan2f(y, x); }
#endif

#endif
