#include <dolphin/mtx.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    // Raw binary32 values, including a Manta Rings angle for which the
    // Windows and Linux single-precision math libraries round differently.
    static const struct { uint32_t angle, sine, cosine; } cases[] = {
        {0x00000000, 0x00000000, 0x3f800000},
        {0x80000000, 0x80000000, 0x3f800000},
        {0x00000001, 0x00000001, 0x3f800000},
        {0xbeee23e4, 0xbee5a573, 0x3f64ce09},
        {0x3fc90fdb, 0x3f800000, 0xb33bbd2e},
        {0xc0490fdb, 0x33bbbd2e, 0xbf800000},
        {0x4f000000, 0xbf78a7c9, 0x3e738617},
        {0x7f7fffff, 0xbf0599b3, 0x3f5a5f96}
    };
    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        float radians;
        Mtx rotation;
        memcpy(&radians, &cases[i].angle, sizeof(radians));
        C_MTXRotRad(rotation, 'y', radians);
        if (memcmp(&rotation[0][2], &cases[i].sine, sizeof(float)) ||
            memcmp(&rotation[0][0], &cases[i].cosine, sizeof(float))) {
            fprintf(stderr, "Rotation mismatch for angle %08x\n", (unsigned)cases[i].angle);
            return 1;
        }
    }
    // Sky Dive tilts its player model about a vector with MTXRotAxisDeg.
    // Check the matrix entry that contributes to the resulting Z position.
    const Vec axis = {1.0f, 0.0f, 0.0f};
    const uint32_t tilt = 0x41e96e41, expectedSin = 0x3ef99e79, expectedCos = 0x3f5f83a1;
    float degrees;
    Mtx rotation;
    memcpy(&degrees, &tilt, sizeof(degrees));
    MTXRotAxisDeg(rotation, &axis, degrees);
    if (memcmp(&rotation[2][1], &expectedSin, sizeof(float)) ||
        memcmp(&rotation[1][1], &expectedCos, sizeof(float))) {
        fputs("Sky Dive axis rotation mismatch\n", stderr);
        return 1;
    }
    puts("Desktop matrix rotation tests passed");
    return 0;
}
