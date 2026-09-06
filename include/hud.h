#pragma once
#include "common.h"
#include "ship.h"
#include "enemy_ai.h"
#include "camera.h"

namespace vd {

struct HudInfo {
    GameState state = GameState::Title;
    int score = 0;
    int wave = 1;
    int kills = 0;
    float survivalTime = 0.f;
    float playerSpeed = 0.f;
    float health = 100.f, shield = 100.f;
    float maxHealth = 100.f, maxShield = 100.f;
    bool hasTarget = false;
    glm::vec3 targetWorld{0};
    float targetDist = 0.f;
    float closingSpeed = 0.f;
    std::vector<glm::vec3> enemyPositions;
    std::vector<bool> enemyAlive;
    glm::vec3 playerPos{0};
    glm::vec3 playerFwd{0,0,-1};
    glm::vec3 playerUp{0,1,0};
    glm::vec3 playerRight{1,0,0};
    glm::vec3 playerVel{0};
};

class Hud {
public:
    bool init();
    void shutdown();
    void draw(const HudInfo& info, const ChaseCamera& cam, int fbW, int fbH);

private:
    GLuint shader = 0;
    GLuint vao = 0, vbo = 0;
    void drawRect(float x, float y, float w, float h, const glm::vec4& color, int fbW, int fbH);
    void drawLine(float x0, float y0, float x1, float y1, const glm::vec4& color, float thickness, int fbW, int fbH);
    void drawRing(float cx, float cy, float r, const glm::vec4& color, int fbW, int fbH, int segs = 32);
    // Simple bitmap-ish text via rects (7-seg style digits + letters for menus)
    void drawChar(char c, float x, float y, float s, const glm::vec4& color, int fbW, int fbH);
    void drawText(const std::string& text, float x, float y, float s, const glm::vec4& color, int fbW, int fbH);
    void drawRadar(const HudInfo& info, int fbW, int fbH);
    void drawReticle(int fbW, int fbH);
    void drawTargetBox(const HudInfo& info, const ChaseCamera& cam, int fbW, int fbH);
};

} // namespace vd
