#pragma once
#include "common.h"
#include "ship.h"
#include "weapon.h"
#include "enemy_ai.h"
#include "camera.h"
#include "renderer.h"
#include "hud.h"
#include "audio.h"
#include "particles.h"

namespace vd {

class Game {
public:
    bool init();
    void shutdown();
    void run();

    GLFWwindow* window = nullptr;
    int fbW = 1280, fbH = 720;
    Renderer renderer;

private:
    void update(float dt);
    void render();
    void handleInput(float dt);
    void startGame();
    void spawnWave(int wave);
    void updateCombat(float dt);
    void onPlayerDeath();
    void captureScreenshot(const std::string& path);

    Hud hud;
    Audio audio;
    ChaseCamera camera;
    StarField stars;
    ParticleSystem particles;
    DebrisField debris;
    RNG rng;

    Ship player;
    Weapon playerWeapon;
    std::vector<Enemy> enemies;
    std::vector<Laser> lasers;
    std::vector<std::pair<glm::vec3, float>> explosionSpheres;

    GameState state = GameState::Title;
    int score = 0;
    int wave = 0;
    int kills = 0;
    float survivalTime = 0.f;
    float waveClearTimer = 0.f;
    float spawnGrace = 0.f;
    float time = 0.f;
    bool mouseCaptured = false;
    double lastMouseX = 0, lastMouseY = 0;
    bool firstMouse = true;
    bool screenshotQueued = false;
    int autoScreenshotFrames = 0;
};

} // namespace vd
