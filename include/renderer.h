#pragma once
#include "common.h"
#include "mesh.h"
#include "ship.h"
#include "weapon.h"
#include "particles.h"
#include "camera.h"
#include "enemy_ai.h"

namespace vd {

struct StarField {
    std::vector<glm::vec3> stars;
    std::vector<float> brightness;
    void init(int count, RNG& rng);
};

struct FlashFX {
    float intensity = 0.f;
    glm::vec3 color{1};
};

class Renderer {
public:
    bool init(int w, int h);
    void resize(int w, int h);
    void shutdown();

    void beginFrame(const ChaseCamera& cam, float aspect);
    void drawStars(const StarField& sf, const ChaseCamera& cam, const glm::vec3& playerVel);
    void drawShip(const Ship& ship, const Mesh& mesh, bool highlight = false);
    void drawLaser(const Laser& l, const Mesh& mesh);
    void drawParticles(const ParticleSystem& ps);
    void drawDebris(const DebrisField& df, const Mesh& shard);
    void drawExplosionSpheres(const std::vector<std::pair<glm::vec3, float>>& spheres, const Mesh& sphere);
    void endScene();

    GLuint shaderSolid = 0;
    GLuint shaderStar = 0;
    GLuint shaderParticle = 0;
    GLuint shaderUnlit = 0;

    Mesh playerMesh, enemyMesh, laserMesh, sphereMesh, quadMesh, shardMesh;
    int width = 1280, height = 720;
    glm::mat4 view{1}, proj{1};
    glm::vec3 camPos{0};
    FlashFX flash;

private:
    GLuint compile(GLenum type, const char* src);
    GLuint link(GLuint vs, GLuint fs);
    GLuint loadProgram(const std::string& vsPath, const std::string& fsPath);
    std::string readFile(const std::string& path);
};

} // namespace vd
