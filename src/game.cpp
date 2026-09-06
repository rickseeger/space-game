#include "game.h"
#include <glm/gtx/quaternion.hpp>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <cstdio>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

namespace vd {

static Game* gGame = nullptr;

static void framebufferSizeCallback(GLFWwindow*, int w, int h) {
    if (gGame && w > 0 && h > 0) {
        gGame->fbW = w; gGame->fbH = h;
        gGame->renderer.resize(w, h);
        glViewport(0, 0, w, h);
    }
}

bool Game::init() {
    gGame = this;
    if (!glfwInit()) {
        std::cerr << "glfwInit failed\n";
        return false;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);

    window = glfwCreateWindow(1280, 720, "Vector Drift", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create window\n";
        glfwTerminate();
        return false;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSwapInterval(1);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "gladLoadGLLoader failed\n";
        return false;
    }
    glfwGetFramebufferSize(window, &fbW, &fbH);
    glEnable(GL_MULTISAMPLE);

    if (!renderer.init(fbW, fbH)) return false;
    if (!hud.init()) return false;
    audio.init(); // OK if fails (headless)
    stars.init(4500, rng);

    player.isPlayer = true;
    player.id = 1;
    player.color = glm::vec3(0.35f, 0.95f, 1.f);
    player.cfg.thrustAccel = 62.f;
    player.cfg.reverseAccel = 40.f;
    player.cfg.dampAssist = 3.2f;
    player.cfg.pitchYawRate = 2.8f;
    player.cfg.angDamp = 5.8f;
    player.cfg.maxSpeed = 100.f;
    player.cfg.linearDrag = 0.28f;
    player.reset(glm::vec3(0, 0, 0));

    playerWeapon.color = glm::vec3(0.35f, 1.f, 1.f);
    playerWeapon.fireInterval = 0.14f;
    playerWeapon.damage = 20.f;
    playerWeapon.muzzleSpeed = 240.f;
    playerWeapon.range = 280.f;

    state = GameState::Title;
    camera.position = glm::vec3(0, 5, 14);

    // Auto screenshot after a bit when env set
    if (const char* env = std::getenv("VD_SCREENSHOT")) {
        (void)env;
        autoScreenshotFrames = 90;
    }
    return true;
}

void Game::shutdown() {
    audio.shutdown();
    hud.shutdown();
    renderer.shutdown();
    if (window) glfwDestroyWindow(window);
    glfwTerminate();
    gGame = nullptr;
}

void Game::startGame() {
    state = GameState::Playing;
    score = 0; kills = 0; wave = 0; survivalTime = 0.f;
    lasers.clear(); enemies.clear(); particles.particles.clear();
    debris.pieces.clear(); explosionSpheres.clear();
    player.reset(glm::vec3(0, 0, 0));
    playerWeapon.cooldown = 0.f;
    camera.position = player.position + glm::vec3(0, 4, 12);
    spawnGrace = 4.0f; // breathing room at spawn
    spawnWave(1);
    audio.playUI();
    mouseCaptured = true;
    firstMouse = true;
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void Game::spawnWave(int w) {
    wave = w;
    enemies.clear();
    int count = 1 + (w > 1 ? 1 : 0) + (w - 1) / 2;
    count = std::min(count, 6);
    // Start with 1-2, escalate
    if (w == 1) count = 1;
    if (w == 2) count = 2;

    for (int i = 0; i < count; i++) {
        Enemy e;
        e.ship.isPlayer = false;
        e.ship.id = 100 + i + w * 10;
        e.ship.color = glm::vec3(1.f, 0.55f + rng.uniform(0.f, 0.25f), 0.18f);
        e.ship.cfg.thrustAccel = 42.f + w * 1.5f;
        e.ship.cfg.maxSpeed = 72.f + w * 2.f;
        e.ship.cfg.linearDrag = 0.4f;
        e.ship.cfg.pitchYawRate = 2.1f;
        e.ship.cfg.maxHealth = 70.f + w * 8.f;
        e.ship.cfg.maxShield = 40.f + w * 5.f;
        e.ship.cfg.radius = 1.7f;
        e.ship.cfg.angAccel = 11.f;
        // Spawn far-ish, not surrounding
        float angle = rng.uniform(0.f, 2.f * PI);
        float dist = 160.f + rng.uniform(20.f, 100.f) + i * 30.f;
        float elev = rng.uniform(-40.f, 40.f);
        glm::vec3 pos = player.position
            + glm::vec3(std::cos(angle) * dist, elev, std::sin(angle) * dist);
        // Face roughly toward player arena center
        glm::vec3 toOrigin = safeNormalize(player.position - pos);
        glm::quat ori = glm::rotation(glm::vec3(0,0,-1), toOrigin);
        e.ship.reset(pos, ori);
        e.weapon.color = glm::vec3(1.f, 0.5f, 0.15f);
        e.weapon.fireInterval = 0.22f;
        e.weapon.damage = 12.f + w * 1.5f;
        e.weapon.muzzleSpeed = 200.f;
        e.weapon.range = 220.f;
        e.aimJitter = std::max(0.06f, 0.18f - w * 0.015f);
        e.aggression = clampf(0.55f + w * 0.05f, 0.55f, 0.9f);
        e.state = AIState::Search;
        e.stateTimer = 1.5f + rng.uniform(0.f, 1.f);
        e.searchTarget = player.position + rng.inSphere(100.f);
        e.fireHold = spawnGrace; // can't fire during grace
        enemies.push_back(e);
    }
    waveClearTimer = 0.f;
}

void Game::onPlayerDeath() {
    state = GameState::Dead;
    particles.emitExplosion(player.position, glm::vec3(0.4f, 0.9f, 1.f), rng);
    debris.spawn(player.position, player.color, rng);
    explosionSpheres.push_back({player.position, 0.f});
    audio.playExplosion();
    renderer.flash.intensity = 1.f;
    renderer.flash.color = glm::vec3(0.5f, 0.8f, 1.f);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    mouseCaptured = false;
}

void Game::handleInput(float dt) {
    if (state == GameState::Title) {
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS ||
            glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
            startGame();
        }
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, 1);
        return;
    }

    static bool escWas = false;
    bool esc = glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS;
    if (esc && !escWas) {
        if (state == GameState::Playing) {
            state = GameState::Paused;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            mouseCaptured = false;
            audio.playUI();
        } else if (state == GameState::Paused) {
            state = GameState::Playing;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            mouseCaptured = true;
            firstMouse = true;
            audio.playUI();
        }
    }
    escWas = esc;

    if (state == GameState::Dead) {
        if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS)
            startGame();
        return;
    }

    if (state == GameState::Paused || state == GameState::WaveClear) {
        // still allow camera during wave clear — treat as playing for ship if wave clear briefly
        if (state == GameState::Paused) return;
    }

    // Mouse look
    player.pitchInput = player.yawInput = player.rollInput = 0.f;
    if (mouseCaptured && state == GameState::Playing) {
        double mx, my;
        glfwGetCursorPos(window, &mx, &my);
        if (firstMouse) { lastMouseX = mx; lastMouseY = my; firstMouse = false; }
        float dx = (float)(mx - lastMouseX);
        float dy = (float)(my - lastMouseY);
        lastMouseX = mx; lastMouseY = my;
        float sens = 0.0028f;
        // Map mouse delta to desired angular rates (orientation only — not velocity)
        float rateScale = 1.15f;
        player.yawInput = clampf(-dx * sens * rateScale / (dt + 1e-4f) / player.cfg.pitchYawRate, -1.f, 1.f);
        player.pitchInput = clampf(-dy * sens * rateScale / (dt + 1e-4f) / player.cfg.pitchYawRate, -1.f, 1.f);
        player.angVel.y += -dx * sens * 12.f;
        player.angVel.x += -dy * sens * 12.f;
    }

    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) player.rollInput -= 1.f;
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) player.rollInput += 1.f;
    player.thrustInput = 0.f;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) player.thrustInput += 1.f;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) player.thrustInput -= 1.f;
    player.dampAssist = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS
                     || glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS;

    bool fire = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS
             || glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    if (state == GameState::Playing && fire) {
        if (playerWeapon.tryFire(player, lasers)) {
            audio.playLaser(true);
            for (int i = 0; i < 2; i++)
                particles.emitBurst(player.position + player.forward() * 3.f,
                    playerWeapon.color, 3, 8.f, 0.12f, rng);
        }
    }
    (void)dt;
}

void Game::updateCombat(float dt) {
    // Laser vs ships
    for (auto& l : lasers) {
        if (!l.alive) continue;
        if (l.ownerId != player.id && laserHitsShip(l, player)) {
            if (spawnGrace > 0.f) { l.alive = false; continue; }
            player.applyDamage(l.damage);
            l.alive = false;
            particles.emitSparks(l.position, -l.velocity, rng);
            audio.playHit();
            renderer.flash.intensity = std::max(renderer.flash.intensity, 0.35f);
            if (player.health < 30.f) audio.playWarning();
            if (!player.alive) onPlayerDeath();
            continue;
        }
        for (auto& e : enemies) {
            if (laserHitsShip(l, e.ship)) {
                e.ship.applyDamage(l.damage);
                l.alive = false;
                particles.emitSparks(l.position, -l.velocity, rng);
                audio.playImpact();
                if (!e.ship.alive) {
                    kills++;
                    score += 100 + wave * 25;
                    particles.emitExplosion(e.ship.position, e.ship.color, rng);
                    debris.spawn(e.ship.position, e.ship.color, rng);
                    explosionSpheres.push_back({e.ship.position, 0.f});
                    audio.playExplosion();
                    renderer.flash.intensity = 0.7f;
                }
                break;
            }
        }
    }

    // Ship collisions (soft)
    for (auto& e : enemies) {
        if (!e.ship.alive || !player.alive) continue;
        float d = glm::length(e.ship.position - player.position);
        float minD = e.ship.cfg.radius + player.cfg.radius;
        if (d < minD && d > 1e-3f) {
            glm::vec3 n = (player.position - e.ship.position) / d;
            float pen = minD - d;
            player.position += n * pen * 0.5f;
            e.ship.position -= n * pen * 0.5f;
            player.velocity += n * 20.f;
            e.ship.velocity -= n * 20.f;
            if (spawnGrace <= 0.f) {
                player.applyDamage(8.f * dt);
                e.ship.applyDamage(8.f * dt);
            }
        }
    }
}

void Game::update(float dt) {
    time += dt;
    handleInput(dt);

    if (state == GameState::Title) {
        // Idle camera spin
        camera.position = glm::vec3(std::sin(time*0.2f)*16.f, 6.f, std::cos(time*0.2f)*16.f);
        camera.orientation = glm::quatLookAt(safeNormalize(glm::vec3(0)-camera.position), glm::vec3(0,1,0));
        return;
    }
    if (state == GameState::Paused) return;

    if (state == GameState::Playing || state == GameState::WaveClear) {
        spawnGrace = std::max(0.f, spawnGrace - dt);
        if (state == GameState::Playing && player.alive) {
            survivalTime += dt;
            score += (int)(dt * 2.f); // tiny survival drip via accumulation below
            playerWeapon.update(dt);
            player.update(dt);
            if (player.thrustInput > 0.1f) audio.playThrust(player.thrustInput);

            // Engine particles
            particles.emitEngine(player.position - player.forward()*2.6f, -player.forward(), player.engineGlow, rng);
            particles.emitDust(camera.position, player.velocity, rng);

            for (auto& e : enemies) {
                if (!e.ship.alive) continue;
                if (spawnGrace > 0.f) e.fireHold = std::max(e.fireHold, spawnGrace);
                updateEnemyAI(e, player, lasers, dt, rng);
                particles.emitEngine(e.ship.position - e.ship.forward()*2.4f, -e.ship.forward(), e.ship.engineGlow * 0.8f, rng);
                // Enemy laser SFX occasionally
            }
            // Detect new enemy lasers for SFX — approximate: if any enemy fired
            static size_t lastLaserCount = 0;
            if (lasers.size() > lastLaserCount) {
                for (size_t i = lastLaserCount; i < lasers.size(); i++)
                    if (lasers[i].ownerId != player.id) audio.playLaser(false);
            }
            lastLaserCount = lasers.size();

            updateLasers(lasers, dt);
            updateCombat(dt);
            camera.update(player, dt);

            // Wave clear?
            bool any = false;
            for (auto& e : enemies) if (e.ship.alive) { any = true; break; }
            if (!any && !enemies.empty()) {
                state = GameState::WaveClear;
                waveClearTimer = 2.2f;
                score += 250 * wave;
            }
        }

        if (state == GameState::WaveClear) {
            waveClearTimer -= dt;
            playerWeapon.update(dt);
            player.update(dt);
            updateLasers(lasers, dt);
            camera.update(player, dt);
            if (waveClearTimer <= 0.f) {
                state = GameState::Playing;
                spawnGrace = 2.0f;
                spawnWave(wave + 1);
            }
        }
    }

    if (state == GameState::Dead) {
        updateLasers(lasers, dt);
        for (auto& e : enemies) if (e.ship.alive) updateEnemyAI(e, player, lasers, dt, rng);
        camera.followSmooth = 3.f;
        // Keep looking at wreck
        Ship ghost = player; ghost.alive = true;
        camera.update(ghost, dt);
        camera.followSmooth = 8.f;
    }

    // Survival score tick
    static float scoreAcc = 0.f;
    if (state == GameState::Playing) {
        scoreAcc += dt * 2.f;
        if (scoreAcc >= 1.f) { score += (int)scoreAcc; scoreAcc -= (int)scoreAcc; }
    }

    particles.update(dt);
    debris.update(dt);
    for (auto& sp : explosionSpheres) sp.second += dt;
    explosionSpheres.erase(std::remove_if(explosionSpheres.begin(), explosionSpheres.end(),
        [](const auto& p){ return p.second > 0.75f; }), explosionSpheres.end());
    audio.update(dt);
}

void Game::render() {
    float aspect = (float)fbW / std::max(1, fbH);
    renderer.beginFrame(camera, aspect);
    renderer.drawStars(stars, camera, player.velocity);

    if (state != GameState::Title) {
        renderer.drawShip(player, renderer.playerMesh);
        for (auto& e : enemies)
            renderer.drawShip(e.ship, renderer.enemyMesh);
        for (auto& l : lasers)
            renderer.drawLaser(l, renderer.laserMesh);
        renderer.drawDebris(debris, renderer.shardMesh);
        renderer.drawExplosionSpheres(explosionSpheres, renderer.sphereMesh);
        renderer.drawParticles(particles);
    } else {
        // Show a demo player ship on title
        Ship demo = player;
        demo.alive = true;
        demo.position = glm::vec3(0,0,0);
        demo.orientation = glm::angleAxis(time * 0.7f, glm::vec3(0,1,0))
                         * glm::angleAxis(std::sin(time)*0.3f, glm::vec3(1,0,0));
        demo.engineGlow = 0.6f + 0.3f * std::sin(time * 3.f);
        renderer.drawShip(demo, renderer.playerMesh);
        renderer.drawParticles(particles);
    }

    HudInfo hi;
    hi.state = state;
    hi.score = score;
    hi.wave = std::max(1, wave);
    hi.kills = kills;
    hi.survivalTime = survivalTime;
    hi.playerSpeed = player.speed();
    hi.health = player.health; hi.shield = player.shield;
    hi.maxHealth = player.cfg.maxHealth; hi.maxShield = player.cfg.maxShield;
    hi.playerPos = player.position;
    hi.playerFwd = player.forward();
    hi.playerUp = player.up();
    hi.playerRight = player.right();
    hi.playerVel = player.velocity;
    float best = 1e9f;
    int bestIdx = -1;
    for (size_t i = 0; i < enemies.size(); i++) {
        hi.enemyPositions.push_back(enemies[i].ship.position);
        hi.enemyAlive.push_back(enemies[i].ship.alive);
        if (!enemies[i].ship.alive) continue;
        float d = glm::length(enemies[i].ship.position - player.position);
        if (d < best) { best = d; bestIdx = (int)i; }
    }
    if (bestIdx >= 0) {
        hi.hasTarget = true;
        hi.targetWorld = enemies[bestIdx].ship.position;
        hi.targetDist = best;
        glm::vec3 to = safeNormalize(hi.targetWorld - player.position);
        hi.closingSpeed = glm::dot(player.velocity - enemies[bestIdx].ship.velocity, to);
    }
    hud.draw(hi, camera, fbW, fbH);
    renderer.endScene();
}

void Game::captureScreenshot(const std::string& path) {
    std::vector<unsigned char> pixels(fbW * fbH * 4);
    glReadPixels(0, 0, fbW, fbH, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    // flip vertical
    std::vector<unsigned char> flipped(fbW * fbH * 4);
    for (int y = 0; y < fbH; y++)
        memcpy(&flipped[y*fbW*4], &pixels[(fbH-1-y)*fbW*4], fbW*4);
    stbi_write_png(path.c_str(), fbW, fbH, 4, flipped.data(), fbW*4);
    std::cout << "Screenshot: " << path << "\n";
}

void Game::run() {
    double last = glfwGetTime();
    while (!glfwWindowShouldClose(window)) {
        double now = glfwGetTime();
        float dt = (float)(now - last);
        last = now;
        dt = std::min(dt, 0.05f);

        glfwPollEvents();
        update(dt);
        render();
        glfwSwapBuffers(window);

        static bool shotTitle = false, shotPlay = false, shotCombat = false;
        if (std::getenv("VD_SCREENSHOT")) {
            if (!shotTitle && time > 1.0f && state == GameState::Title) {
                captureScreenshot("/workspace/space-game/screenshots/title.png");
                shotTitle = true;
                startGame();
            }
            if (shotTitle && !shotPlay && time > 3.0f) {
                captureScreenshot("/workspace/space-game/screenshots/gameplay.png");
                shotPlay = true;
            }
            if (shotPlay && !shotCombat && time > 5.5f) {
                captureScreenshot("/workspace/space-game/screenshots/combat.png");
                shotCombat = true;
                glfwSetWindowShouldClose(window, 1);
            }
        }
        if (screenshotQueued) {
            captureScreenshot("/workspace/space-game/screenshots/manual.png");
            screenshotQueued = false;
        }
    }
}

} // namespace vd
