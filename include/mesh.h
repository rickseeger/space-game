#pragma once
#include "common.h"

namespace vd {

struct Mesh {
    GLuint vao = 0, vbo = 0, ebo = 0;
    int indexCount = 0;
    void destroy();
};

struct Vertex {
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec3 color;
};

Mesh makeFighterMesh(const glm::vec3& baseColor, bool playerStyle);
Mesh makeLaserMesh();
Mesh makeUnitSphere(int slices = 12, int stacks = 8);
Mesh makeQuad();
Mesh uploadMesh(const std::vector<Vertex>& verts, const std::vector<unsigned>& idx);

} // namespace vd
