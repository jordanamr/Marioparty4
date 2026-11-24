// Desktop adapter for the fdlibm kernels shipped with the GameCube sources.
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

#define __ieee754_asin PartyBoard_KernelAsin
#define __ieee754_acos PartyBoard_KernelAcos
#define __ieee754_atan2 PartyBoard_KernelAtan2
#define atan PartyBoard_KernelAtan
#define fabs__Fd fabs
double __ieee754_asin(double x);
double __ieee754_acos(double x);
double __ieee754_atan2(double y, double x);
double atan(double x);

#define __kernel_sin PartyBoard_KernelSin
#define __kernel_cos PartyBoard_KernelCos
#define __kernel_rem_pio2 PartyBoard_KernelRemPi2
#define __ieee754_rem_pio2 PartyBoard_ReducePi2

double __kernel_sin(double x, double tail, int hasTail);
double __kernel_cos(double x, double tail);
int __kernel_rem_pio2(double *x, double *y, int exponent, int count, int precision, const int *twoOverPi);
int __ieee754_rem_pio2(double x, double *remainder);
