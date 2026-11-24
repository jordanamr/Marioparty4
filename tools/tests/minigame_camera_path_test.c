#include <assert.h>
#include <fenv.h>
#include "src/REL/m403Dll/main.c"

int main(void)
{
    float knots[3] = {0, 1, 2}, values[3] = {3, 6, 9}, coefficients[3];
    fn_1_4FF4(knots, values, coefficients, 3);
    assert(fn_1_52AC(0.5f, knots, values, coefficients, 3) == 4.5f);
    assert(fn_1_52AC(2, knots, values, coefficients, 3) == 9);
    knots[1] = knots[0];
    feclearexcept(FE_ALL_EXCEPT);
    assert(fn_1_52AC(0.5f, knots, values, coefficients, 3) == 3);
    assert(!fetestexcept(FE_DIVBYZERO | FE_INVALID));

    Vec points[3] = {{1, 2, 3}, {4, 5, 6}, {7, 8, 9}};
    Vec sample;
    float path[3][3];
    fn_1_5400(knots, points, path, 3);
    fn_1_57E0(2, &sample, knots, points, path, 3);
    assert(sample.x == 7 && sample.y == 8 && sample.z == 9);
    return 0;
}
