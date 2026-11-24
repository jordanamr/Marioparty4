#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "src/REL/m428Dll/map.c"

void *HuMemDirectMallocNum(HeapID heap, size_t size, uintptr_t num)
{
    return calloc(1, size);
}

static void CheckFace(s16 type)
{
    Vec vertices[] = { {0, 0, 2}, {10, 0, 2}, {0, 10, 2}, {10, 10, 2} };
    HSFFACE face = { 0 };
    HSFBUFFER positions = { 0 }, faces = { 0 };
    HSFOBJECT object = { 0 };
    M433DllUnkStruct2 mesh = { 0 };
    face.type = type;
    for (int i = 0; i < 4; i++) {
        face.indices[i][0] = i;
    }
    positions.count = 4;
    positions.data = vertices;
    faces.count = 1;
    faces.data = &face;
    object.mesh.vertex = &positions;
    object.mesh.face = &faces;
    lbl_1_bss_128 = &mesh;
    fn_1_5684(NULL, &object);

    Vec firstHalf = {1, 1, -1}, secondHalf = {8, 8, -1}, outside = {11, 11, -1};
    assert(fn_1_5370(&mesh, &firstHalf) && firstHalf.z == 2);
    assert(fn_1_5370(&mesh, &secondHalf) == (type == HSF_FACE_QUAD));
    assert(secondHalf.z == (type == HSF_FACE_QUAD ? 2 : -1));
    assert(!fn_1_5370(&mesh, &outside));
    free(mesh.unk_0C);
}

int main(void)
{
    CheckFace(HSF_FACE_TRI);
    CheckFace(HSF_FACE_QUAD);
    puts("Collision faces: triangle, both halves of the quad, and exterior points.");
    return 0;
}
