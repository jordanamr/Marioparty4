#include <dolphin/mtx.h>
#include "port/rotation_math.h"
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
    // Direction changes observed in m416 at netplay frame 6946.
    static const struct { uint32_t input, result; } steering[] = {
        {0x3f7b7a55, 0x3fb0f7a8}, {0x3e98d2b3, 0x3e9b3039}, {0x3e8746f6, 0x3e88e718}
    };
    for (size_t i = 0; i < sizeof(steering) / sizeof(steering[0]); ++i) {
        float ratio;
        memcpy(&ratio, &steering[i].input, sizeof(ratio));
        float angle = PartyBoard_AsinF(ratio);
        if (memcmp(&angle, &steering[i].result, sizeof(angle))) {
            fputs("Obstacle avoidance angle mismatch\n", stderr);
            return 1;
        }
    }

    // Makin' Waves constructs an axis quaternion, then interpolates orientations.
    Quaternion from = {0, 0, 0, 1}, to;
    const uint32_t radWord = 0xbf6e23e4, qx = 0xbee5a573, qw = 0x3f64ce09;
    const uint32_t sx = 0xbe2f5aee, sw = 0x3f7c37ee;
    float rad;
    memcpy(&rad, &radWord, sizeof(rad));
    C_QUATRotAxisRad(&to, &axis, rad);
    C_QUATSlerp(&from, &to, &from, 0.37f);
    if (memcmp(&to.x, &qx, sizeof(float)) || memcmp(&to.w, &qw, sizeof(float)) ||
        memcmp(&from.x, &sx, sizeof(float)) || memcmp(&from.w, &sw, sizeof(float))) {
        fputs("Quaternion rotation or interpolation mismatch\n", stderr);
        return 1;
    }
    const uint32_t expectedAtan = 0x402a0e2b, expectedAcos = 0x3fb93814;
    float direction = PartyBoard_Atan2F(0.1234567f, -0.2345678f);
    float incline = PartyBoard_AcosF(0.1234567f);
    if (memcmp(&direction, &expectedAtan, sizeof(float)) ||
        memcmp(&incline, &expectedAcos, sizeof(float))) {
        fputs("Matrix orientation angle mismatch\n", stderr);
        return 1;
    }
    puts("Desktop rotation tests passed");
    return 0;
}
