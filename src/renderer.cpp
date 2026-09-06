#include "renderer.h"
#include <fstream>
#include <sstream>
#include <iostream>

namespace vd {

void StarField::init(int count, RNG& rng) {
    stars.clear(); brightness.clear();
    stars.reserve(count); brightness.reserve(count);
    for (int i = 0; i < count; i++) {
        stars.push_back(rng.inSphere(500.f) * (rng.uniform(0.3f, 1.f) + 0.2f));
        // redistribute more uniformly in cube
        stars.back() = glm::vec3(rng.uniform(-700.f, 700.f), rng.uniform(-700.f, 700.f), rng.uniform(-700.f, 700.f));
        brightness.push_back(rng.uniform(0.45f, 1.f));
    }
}

std::string Renderer::readFile(const std::string& path) {
    std::ifstream f(path);
    if (!f) {
        // try relative to executable cwd / shaders
        std::ifstream f2(std::string("shaders/") + path);
        if (!f2) {
            std::cerr << "Failed to read shader: " << path << "\n";
            return {};
        }
        std::stringstream ss; ss << f2.rdbuf(); return ss.str();
    }
    std::stringstream ss; ss << f.rdbuf(); return ss.str();
}

GLuint Renderer::compile(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    GLint ok = 0; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024]; glGetShaderInfoLog(s, 1024, nullptr, log);
        std::cerr << "Shader compile error: " << log << "\n";
    }
    return s;
}

GLuint Renderer::link(GLuint vs, GLuint fs) {
    GLuint p = glCreateProgram();
    glAttachShader(p, vs); glAttachShader(p, fs);
    glLinkProgram(p);
    GLint ok = 0; glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[1024]; glGetProgramInfoLog(p, 1024, nullptr, log);
        std::cerr << "Link error: " << log << "\n";
    }
    glDeleteShader(vs); glDeleteShader(fs);
    return p;
}

GLuint Renderer::loadProgram(const std::string& vsPath, const std::string& fsPath) {
    auto find = [](const std::string& name) -> std::string {
        const char* tries[] = {"", "./", "shaders/", "./shaders/", "../shaders/"};
        for (auto t : tries) {
            std::ifstream f(std::string(t) + name);
            if (f) { std::stringstream ss; ss << f.rdbuf(); return ss.str(); }
        }
        std::cerr << "Missing shader file: " << name << "\n";
        return {};
    };
    std::string vs = find(vsPath);
    std::string fs = find(fsPath);
    if (vs.empty() || fs.empty()) return 0;
    return link(compile(GL_VERTEX_SHADER, vs.c_str()), compile(GL_FRAGMENT_SHADER, fs.c_str()));
}

bool Renderer::init(int w, int h) {
    width = w; height = h;
    shaderSolid = loadProgram("solid.vert", "solid.frag");
    shaderStar = loadProgram("star.vert", "star.frag");
    shaderParticle = loadProgram("particle.vert", "particle.frag");
    shaderUnlit = loadProgram("unlit.vert", "unlit.frag");
    if (!shaderSolid || !shaderStar || !shaderParticle || !shaderUnlit) return false;

    playerMesh = makeFighterMesh(glm::vec3(0.25f, 0.85f, 1.f), true);
    enemyMesh = makeFighterMesh(glm::vec3(1.f, 0.45f, 0.15f), false);
    laserMesh = makeLaserMesh();
    sphereMesh = makeUnitSphere(14, 10);
    quadMesh = makeQuad();
    // shard = small irregular tetra
    {
        std::vector<Vertex> v;
        std::vector<unsigned> i;
        auto add = [&](glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 col) {
            glm::vec3 n = safeNormalize(glm::cross(b-a, c-a));
            unsigned b0 = (unsigned)v.size();
            v.push_back({a,n,col}); v.push_back({b,n,col}); v.push_back({c,n,col});
            i.push_back(b0); i.push_back(b0+1); i.push_back(b0+2);
        };
        glm::vec3 a(0.6f,0,0), b(-0.3f,0.5f,0), c(-0.3f,-0.4f,0.4f), d(-0.2f,-0.2f,-0.5f);
        add(a,b,c,{1,1,1}); add(a,c,d,{1,1,1}); add(a,d,b,{1,1,1}); add(b,d,c,{1,1,1});
        shardMesh = uploadMesh(v, i);
    }
    glEnable(GL_PROGRAM_POINT_SIZE);
    return true;
}

void Renderer::resize(int w, int h) { width = w; height = h; }

void Renderer::shutdown() {
    playerMesh.destroy(); enemyMesh.destroy(); laserMesh.destroy();
    sphereMesh.destroy(); quadMesh.destroy(); shardMesh.destroy();
    if (shaderSolid) glDeleteProgram(shaderSolid);
    if (shaderStar) glDeleteProgram(shaderStar);
    if (shaderParticle) glDeleteProgram(shaderParticle);
    if (shaderUnlit) glDeleteProgram(shaderUnlit);
}

void Renderer::beginFrame(const ChaseCamera& cam, float aspect) {
    view = cam.view();
    proj = cam.proj(aspect);
    camPos = cam.position;
    glViewport(0, 0, width, height);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.01f, 0.012f, 0.03f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    flash.intensity *= 0.85f;
}

void Renderer::drawStars(const StarField& sf, const ChaseCamera& cam, const glm::vec3& playerVel) {
    if (sf.stars.empty()) return;
    static GLuint vao = 0, vbo = 0;
    static size_t cap = 0;
    if (!vao) { glGenVertexArrays(1, &vao); glGenBuffers(1, &vbo); }
    struct StarV { glm::vec3 p; float b; };
    std::vector<StarV> data(sf.stars.size());
    for (size_t i = 0; i < sf.stars.size(); i++) {
        data[i] = {sf.stars[i], sf.brightness[i]};
    }
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    if (data.size() > cap) {
        glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(StarV), data.data(), GL_DYNAMIC_DRAW);
        cap = data.size();
    } else {
        glBufferSubData(GL_ARRAY_BUFFER, 0, data.size() * sizeof(StarV), data.data());
    }
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(StarV), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, sizeof(StarV), (void*)sizeof(glm::vec3));

    glUseProgram(shaderStar);
    glUniformMatrix4fv(glGetUniformLocation(shaderStar, "uView"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(shaderStar, "uProj"), 1, GL_FALSE, glm::value_ptr(proj));
    glUniform3fv(glGetUniformLocation(shaderStar, "uCamPos"), 1, glm::value_ptr(cam.position));
    glUniform1f(glGetUniformLocation(shaderStar, "uSpeed"), glm::length(playerVel));
    glUniform1f(glGetUniformLocation(shaderStar, "uAspect"), (float)width / std::max(1, height));
    glDepthMask(GL_FALSE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glDrawArrays(GL_POINTS, 0, (GLsizei)data.size());
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_TRUE);
    glBindVertexArray(0);
}

void Renderer::drawShip(const Ship& ship, const Mesh& mesh, bool highlight) {
    if (!ship.alive) return;
    glm::mat4 model = glm::translate(glm::mat4(1), ship.position)
                    * glm::mat4_cast(ship.orientation);
    glm::mat4 mvp = proj * view * model;
    glm::mat3 nmat = glm::mat3(glm::transpose(glm::inverse(model)));
    glUseProgram(shaderSolid);
    glUniformMatrix4fv(glGetUniformLocation(shaderSolid, "uMVP"), 1, GL_FALSE, glm::value_ptr(mvp));
    glUniformMatrix4fv(glGetUniformLocation(shaderSolid, "uModel"), 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix3fv(glGetUniformLocation(shaderSolid, "uNormalMat"), 1, GL_FALSE, glm::value_ptr(nmat));
    glUniform3fv(glGetUniformLocation(shaderSolid, "uCamPos"), 1, glm::value_ptr(camPos));
    glUniform3fv(glGetUniformLocation(shaderSolid, "uTint"), 1, glm::value_ptr(ship.color));
    glUniform1f(glGetUniformLocation(shaderSolid, "uGlow"), ship.engineGlow * 0.6f + (highlight ? 0.25f : 0.f));
    glUniform1f(glGetUniformLocation(shaderSolid, "uFlash"), flash.intensity * 0.15f);
    glBindVertexArray(mesh.vao);
    glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT, nullptr);

    // Engine glow sphere at rear
    if (ship.engineGlow > 0.05f) {
        glm::vec3 engPos = ship.position - ship.forward() * 2.5f;
        float sc = 0.4f + ship.engineGlow * 0.7f;
        glm::mat4 em = glm::translate(glm::mat4(1), engPos) * glm::scale(glm::mat4(1), glm::vec3(sc));
        glm::mat4 emvp = proj * view * em;
        glUseProgram(shaderUnlit);
        glUniformMatrix4fv(glGetUniformLocation(shaderUnlit, "uMVP"), 1, GL_FALSE, glm::value_ptr(emvp));
        glm::vec3 ec = ship.isPlayer ? glm::vec3(0.4f, 0.9f, 1.f) : glm::vec3(1.f, 0.45f, 0.1f);
        glUniform3fv(glGetUniformLocation(shaderUnlit, "uColor"), 1, glm::value_ptr(ec));
        glUniform1f(glGetUniformLocation(shaderUnlit, "uAlpha"), 0.55f * ship.engineGlow);
        glDepthMask(GL_FALSE);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glBindVertexArray(sphereMesh.vao);
        glDrawElements(GL_TRIANGLES, sphereMesh.indexCount, GL_UNSIGNED_INT, nullptr);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_TRUE);
    }
}

void Renderer::drawLaser(const Laser& l, const Mesh& mesh) {
    if (!l.alive) return;
    glm::vec3 dir = safeNormalize(l.velocity);
    glm::vec3 up = std::abs(glm::dot(dir, glm::vec3(0,1,0))) > 0.95f ? glm::vec3(1,0,0) : glm::vec3(0,1,0);
    glm::vec3 r = safeNormalize(glm::cross(dir, up));
    up = glm::cross(r, dir);
    glm::mat3 rot(r, up, -dir);
    float lifeT = l.life / l.maxLife;
    glm::mat4 model = glm::translate(glm::mat4(1), l.position)
                    * glm::mat4(rot)
                    * glm::scale(glm::mat4(1), glm::vec3(1.f, 1.f, 1.3f));
    glm::mat4 mvp = proj * view * model;
    glUseProgram(shaderUnlit);
    glUniformMatrix4fv(glGetUniformLocation(shaderUnlit, "uMVP"), 1, GL_FALSE, glm::value_ptr(mvp));
    glm::vec3 col = l.color * (0.7f + 0.3f * lifeT);
    glUniform3fv(glGetUniformLocation(shaderUnlit, "uColor"), 1, glm::value_ptr(col));
    glUniform1f(glGetUniformLocation(shaderUnlit, "uAlpha"), 0.95f);
    glDepthMask(GL_FALSE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glBindVertexArray(mesh.vao);
    glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT, nullptr);
    // glow shell
    glm::mat4 glow = glm::translate(glm::mat4(1), l.position) * glm::mat4(rot)
                   * glm::scale(glm::mat4(1), glm::vec3(2.2f, 2.2f, 1.5f));
    glm::mat4 gmvp = proj * view * glow;
    glUniformMatrix4fv(glGetUniformLocation(shaderUnlit, "uMVP"), 1, GL_FALSE, glm::value_ptr(gmvp));
    glUniform1f(glGetUniformLocation(shaderUnlit, "uAlpha"), 0.25f);
    glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT, nullptr);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_TRUE);
}

void Renderer::drawParticles(const ParticleSystem& ps) {
    if (ps.particles.empty()) return;
    static GLuint vao = 0, vbo = 0; static size_t cap = 0;
    if (!vao) { glGenVertexArrays(1, &vao); glGenBuffers(1, &vbo); }
    struct PV { glm::vec3 p, c; float s; };
    std::vector<PV> data;
    data.reserve(ps.particles.size());
    for (auto& p : ps.particles) {
        float t = clampf(p.life / p.maxLife, 0.f, 1.f);
        data.push_back({p.pos, p.color * t, p.size * (0.5f + 0.5f * t)});
    }
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    size_t bytes = data.size() * sizeof(PV);
    if (data.size() > cap) { glBufferData(GL_ARRAY_BUFFER, bytes, data.data(), GL_DYNAMIC_DRAW); cap = data.size(); }
    else glBufferSubData(GL_ARRAY_BUFFER, 0, bytes, data.data());
    glEnableVertexAttribArray(0); glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(PV), (void*)0);
    glEnableVertexAttribArray(1); glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(PV), (void*)sizeof(glm::vec3));
    glEnableVertexAttribArray(2); glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(PV), (void*)(sizeof(glm::vec3)*2));
    glUseProgram(shaderParticle);
    glUniformMatrix4fv(glGetUniformLocation(shaderParticle, "uView"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(shaderParticle, "uProj"), 1, GL_FALSE, glm::value_ptr(proj));
    glDepthMask(GL_FALSE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glDrawArrays(GL_POINTS, 0, (GLsizei)data.size());
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_TRUE);
}

void Renderer::drawDebris(const DebrisField& df, const Mesh& shard) {
    glUseProgram(shaderSolid);
    for (auto& d : df.pieces) {
        if (!d.alive) continue;
        glm::mat4 model = glm::translate(glm::mat4(1), d.pos)
                        * glm::mat4_cast(d.ori)
                        * glm::scale(glm::mat4(1), glm::vec3(d.size));
        glm::mat4 mvp = proj * view * model;
        glm::mat3 nmat = glm::mat3(glm::transpose(glm::inverse(model)));
        glUniformMatrix4fv(glGetUniformLocation(shaderSolid, "uMVP"), 1, GL_FALSE, glm::value_ptr(mvp));
        glUniformMatrix4fv(glGetUniformLocation(shaderSolid, "uModel"), 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix3fv(glGetUniformLocation(shaderSolid, "uNormalMat"), 1, GL_FALSE, glm::value_ptr(nmat));
        glUniform3fv(glGetUniformLocation(shaderSolid, "uCamPos"), 1, glm::value_ptr(camPos));
        glUniform3fv(glGetUniformLocation(shaderSolid, "uTint"), 1, glm::value_ptr(d.color));
        float glow = clampf(d.life * 0.3f, 0.f, 1.f);
        glUniform1f(glGetUniformLocation(shaderSolid, "uGlow"), glow);
        glUniform1f(glGetUniformLocation(shaderSolid, "uFlash"), 0.f);
        glBindVertexArray(shard.vao);
        glDrawElements(GL_TRIANGLES, shard.indexCount, GL_UNSIGNED_INT, nullptr);
    }
}

void Renderer::drawExplosionSpheres(const std::vector<std::pair<glm::vec3, float>>& spheres, const Mesh& sphere) {
    glUseProgram(shaderUnlit);
    glDepthMask(GL_FALSE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    for (auto& sp : spheres) {
        float age = sp.second;
        float radius = 1.f + age * 18.f;
        float alpha = std::max(0.f, 1.f - age / 0.55f);
        glm::mat4 model = glm::translate(glm::mat4(1), sp.first)
                        * glm::scale(glm::mat4(1), glm::vec3(radius));
        glm::mat4 mvp = proj * view * model;
        glUniformMatrix4fv(glGetUniformLocation(shaderUnlit, "uMVP"), 1, GL_FALSE, glm::value_ptr(mvp));
        glm::vec3 col = glm::mix(glm::vec3(1.f, 0.95f, 0.7f), glm::vec3(1.f, 0.3f, 0.05f), age / 0.55f);
        glUniform3fv(glGetUniformLocation(shaderUnlit, "uColor"), 1, glm::value_ptr(col));
        glUniform1f(glGetUniformLocation(shaderUnlit, "uAlpha"), alpha * 0.65f);
        glBindVertexArray(sphere.vao);
        glDrawElements(GL_TRIANGLES, sphere.indexCount, GL_UNSIGNED_INT, nullptr);
    }
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_TRUE);
}

void Renderer::endScene() {}

} // namespace vd
