#include <assert.h>
#include "game/object.h"
static omObjData *models[1];
#define omGetGroupMemberListEx(process, index) models
#include "src/REL/m406Dll/player.c"

int main(void)
{
    omObjData object = {0};
    s16 modelIds[6] = {0};
    HSFOBJECT root = {0};
    HSFDATA file = {0};
    HSFBUFFER faces = {0};
    HSFBUFFER vertices = {0};
    UnkM406PlayerStruct2 floor = {0};
    Vec point = {21, 10, -60000};
    Vec normal = {3, 4, 5};

    /* Beyond the last course segment there is neither a wall nor a floor. */
    assert(!fn_1_13C10(&point, &normal));
    assert(normal.x == 3 && normal.y == 4 && normal.z == 5);
    assert(fn_1_143F4(&point, &floor) == -100000 && !floor.unk_00);

    models[0] = &object;
    object.model = modelIds;
    Hu3DData[0].hsf = &file;
    file.root = &root;
    point.z = -59999;
    assert(!fn_1_13C10(&point, &normal));
    assert(fn_1_143F4(&point, &floor) == -100000 && !floor.unk_00);

    HSFFACE wall = {0};
    Vec corners[] = {{0, 0, -50}, {0, 100, -50}, {0, 0, 50}};
    wall.type = 2;
    wall.nbt.x = 1;
    wall.indices[1][0] = 1;
    wall.indices[2][0] = 2;
    faces.data = &wall;
    faces.count = 1;
    vertices.data = corners;
    root.type = 2;
    root.mesh.face = &faces;
    root.mesh.vertex = &vertices;
    point = (Vec){21, 10, -10};
    assert(fn_1_13C10(&point, &normal));
    assert(point.x > 100 && point.x < 100.01f && point.z == -10);
    assert(normal.x == 1 && normal.y == 0 && normal.z == 0);
    return 0;
}
