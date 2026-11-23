// Desktop adapter for the four fdlibm kernels shipped with the GameCube sources.
// They access double words as lvalues. Compile these C files without strict aliasing.
#pragma once
#include <math.h>
#include <stdint.h>

// Use the host libc for fabs, floor and ldexp. The original SDK headers assume PPC.
#define _FDLIBM_H
#define MSL_MATH_H_
#ifdef PB_BIG_ENDIAN
#define __HI(x) (((int32_t *)&(x))[0])
#define __LO(x) (((int32_t *)&(x))[1])
#else
#define __HI(x) (((int32_t *)&(x))[1])
#define __LO(x) (((int32_t *)&(x))[0])
#endif

#define __kernel_sin PartyBoard_KernelSin
#define __kernel_cos PartyBoard_KernelCos
#define __kernel_rem_pio2 PartyBoard_KernelRemPi2
#define __ieee754_rem_pio2 PartyBoard_ReducePi2

double __kernel_sin(double x, double tail, int hasTail);
double __kernel_cos(double x, double tail);
int __kernel_rem_pio2(double *x, double *y, int exponent, int count, int precision, const int *twoOverPi);
int __ieee754_rem_pio2(double x, double *remainder);
