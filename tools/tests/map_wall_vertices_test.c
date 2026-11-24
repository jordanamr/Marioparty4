#include <assert.h>
#include "src/game/mapspace.c"

int main(void)
{
    Vec triangle[] = {{10, 0, 20}, {14, 0, 20}, {10, 0, 24}};
    Vec center = {16, 0, 19};
    Vec hit = {-900, -900, -900};
    assert(Hitcheck_Triangle_with_Sphere(triangle, &center, 3, &hit));
    assert(hit.x == 14 && hit.y == 0 && hit.z == 20);
    center = (Vec){9, 0, 26};
    assert(Hitcheck_Triangle_with_Sphere(triangle, &center, 3, &hit));
    assert(hit.x == 10 && hit.y == 0 && hit.z == 24);
    center = (Vec){12, 0, 22};
    assert(Hitcheck_Triangle_with_Sphere(triangle, &center, 1, &hit));
    assert(hit.x == 12 && hit.z == 22);
    Vec flat[] = {{0, 0, 0}, {4, 0, 0}, {2, 0, 0}};
    center = (Vec){3, 1, 0};
    assert(Hitcheck_Triangle_with_Sphere(flat, &center, 1, &hit));
    assert(hit.x == 3 && hit.y == 0 && hit.z == 0);
    Vec quad[] = {{0, 0, 0}, {0, 0, 2}, {2, 0, 0}, {2, 0, 2}};
    s16 face[] = {(s16)0x8004, 0, 1, 2, 3};
    Vec dir = {0, 1, 0};
    center = (Vec){1.5f, 0, 0.5f};
    assert(DefIfnnerMapCircle(&center, face, quad, &dir));
    center = (Vec){0.5f, 0, 1.5f};
    assert(DefIfnnerMapCircle(&center, face, quad, &dir));
    center.x = 2.5f;
    assert(!DefIfnnerMapCircle(&center, face, quad, &dir));
    for (int i = 0; i < 40; ++i) {
        StoreFloorTriangle(i, i + 1, i + 2);
        assert(ColisionIdx[ColisionCount - 1][0] == i);
    }
    assert(ColisionCount == 10);
    nMap = 0;
    hit = (Vec){3, 2, 1};
    assert(MapPos(0, 12, 0, 10, &hit) == 12);
    assert(hit.x == 0 && hit.y == 1 && hit.z == 0);
    return 0;
}
