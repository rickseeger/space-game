#include "mesh.h"

namespace vd {

void Mesh::destroy() {
    if (ebo) glDeleteBuffers(1, &ebo);
    if (vbo) glDeleteBuffers(1, &vbo);
    if (vao) glDeleteVertexArrays(1, &vao);
    vao = vbo = ebo = 0;
    indexCount = 0;
}

Mesh uploadMesh(const std::vector<Vertex>& verts, const std::vector<unsigned>& idx) {
    Mesh m;
    glGenVertexArrays(1, &m.vao);
    glGenBuffers(1, &m.vbo);
    glGenBuffers(1, &m.ebo);
    glBindVertexArray(m.vao);
    glBindBuffer(GL_ARRAY_BUFFER, m.vbo);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(Vertex), verts.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, idx.size() * sizeof(unsigned), idx.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, pos));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));
    glBindVertexArray(0);
    m.indexCount = (int)idx.size();
    return m;
}

static void addTri(std::vector<Vertex>& v, std::vector<unsigned>& idx,
                   glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 col) {
    glm::vec3 n = safeNormalize(glm::cross(b - a, c - a));
    unsigned base = (unsigned)v.size();
    v.push_back({a, n, col});
    v.push_back({b, n, col});
    v.push_back({c, n, col});
    idx.push_back(base); idx.push_back(base+1); idx.push_back(base+2);
}

Mesh makeFighterMesh(const glm::vec3& baseColor, bool playerStyle) {
    std::vector<Vertex> verts;
    std::vector<unsigned> idx;
    glm::vec3 body = baseColor;
    glm::vec3 dark = baseColor * 0.35f;
    glm::vec3 edge = playerStyle ? glm::vec3(0.5f, 1.f, 1.f) : glm::vec3(1.f, 0.6f, 0.2f);
    glm::vec3 engine = playerStyle ? glm::vec3(0.3f, 0.8f, 1.f) : glm::vec3(1.f, 0.35f, 0.1f);

    // Elongated triangular fighter with wings (nose -Z)
    glm::vec3 nose(0, 0, -3.2f);
    glm::vec3 midTop(0, 0.7f, 0.2f);
    glm::vec3 midBot(0, -0.45f, 0.4f);
    glm::vec3 leftW(-2.6f, -0.1f, 1.4f);
    glm::vec3 rightW(2.6f, -0.1f, 1.4f);
    glm::vec3 leftR(-0.9f, 0.1f, 2.2f);
    glm::vec3 rightR(0.9f, 0.1f, 2.2f);
    glm::vec3 tail(0, 0.15f, 2.4f);
    glm::vec3 fin(0, 1.3f, 1.6f);

    // Fuselage facets
    addTri(verts, idx, nose, midTop, rightW, body);
    addTri(verts, idx, nose, leftW, midTop, body);
    addTri(verts, idx, nose, rightW, midBot, dark);
    addTri(verts, idx, nose, midBot, leftW, dark);
    addTri(verts, idx, midTop, leftW, leftR, edge);
    addTri(verts, idx, midTop, rightR, rightW, edge);
    addTri(verts, idx, midTop, leftR, rightR, body);
    addTri(verts, idx, midBot, rightW, rightR, dark);
    addTri(verts, idx, midBot, leftR, leftW, dark);
    addTri(verts, idx, midBot, rightR, leftR, dark);
    // Wings undersides / tips
    addTri(verts, idx, leftW, leftR, glm::vec3(-2.9f, -0.3f, 1.8f), edge * 0.7f);
    addTri(verts, idx, rightW, glm::vec3(2.9f, -0.3f, 1.8f), rightR, edge * 0.7f);
    // Tail fin
    addTri(verts, idx, midTop, fin, tail, edge);
    addTri(verts, idx, midTop, tail, fin, edge * 0.8f);
    // Engine block (rear)
    glm::vec3 e0(-0.55f, -0.25f, 2.35f);
    glm::vec3 e1(0.55f, -0.25f, 2.35f);
    glm::vec3 e2(0.55f, 0.45f, 2.35f);
    glm::vec3 e3(-0.55f, 0.45f, 2.35f);
    addTri(verts, idx, e0, e1, e2, engine);
    addTri(verts, idx, e0, e2, e3, engine);

    if (playerStyle) {
        // Canopy
        glm::vec3 c0(0, 0.55f, -0.8f);
        addTri(verts, idx, midTop, c0, glm::vec3(0.4f, 0.5f, 0.1f), glm::vec3(0.4f, 0.9f, 1.f));
        addTri(verts, idx, midTop, glm::vec3(-0.4f, 0.5f, 0.1f), c0, glm::vec3(0.4f, 0.9f, 1.f));
    }

    return uploadMesh(verts, idx);
}

Mesh makeLaserMesh() {
    std::vector<Vertex> verts;
    std::vector<unsigned> idx;
    glm::vec3 col(1.f);
    // Elongated bolt along -Z
    float halfLen = 2.2f;
    float r = 0.18f;
    glm::vec3 tip(0, 0, -halfLen);
    glm::vec3 aft(0, 0, halfLen * 0.4f);
    const int N = 6;
    for (int i = 0; i < N; i++) {
        float a0 = (float)i / N * 2.f * PI;
        float a1 = (float)(i + 1) / N * 2.f * PI;
        glm::vec3 p0(std::cos(a0) * r, std::sin(a0) * r, 0);
        glm::vec3 p1(std::cos(a1) * r, std::sin(a1) * r, 0);
        addTri(verts, idx, tip, p1, p0, col);
        addTri(verts, idx, aft, p0, p1, col * 0.5f);
    }
    return uploadMesh(verts, idx);
}

Mesh makeUnitSphere(int slices, int stacks) {
    std::vector<Vertex> verts;
    std::vector<unsigned> idx;
    for (int y = 0; y <= stacks; y++) {
        float v = (float)y / stacks;
        float phi = v * PI;
        for (int x = 0; x <= slices; x++) {
            float u = (float)x / slices;
            float theta = u * 2.f * PI;
            glm::vec3 n(std::sin(phi) * std::cos(theta), std::cos(phi), std::sin(phi) * std::sin(theta));
            verts.push_back({n, n, glm::vec3(1.f)});
        }
    }
    for (int y = 0; y < stacks; y++) {
        for (int x = 0; x < slices; x++) {
            unsigned i0 = y * (slices + 1) + x;
            unsigned i1 = i0 + slices + 1;
            idx.push_back(i0); idx.push_back(i1); idx.push_back(i0 + 1);
            idx.push_back(i0 + 1); idx.push_back(i1); idx.push_back(i1 + 1);
        }
    }
    return uploadMesh(verts, idx);
}

Mesh makeQuad() {
    std::vector<Vertex> verts = {
        {{-1,-1,0},{0,0,1},{1,1,1}}, {{1,-1,0},{0,0,1},{1,1,1}},
        {{1,1,0},{0,0,1},{1,1,1}}, {{-1,1,0},{0,0,1},{1,1,1}},
    };
    std::vector<unsigned> idx = {0,1,2, 0,2,3};
    return uploadMesh(verts, idx);
}

} // namespace vd
