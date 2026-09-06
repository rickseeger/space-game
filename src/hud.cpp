#include "hud.h"
#include <cctype>
#include <cmath>
#include <cstdio>

namespace vd {

bool Hud::init() {
    const char* vs = R"(#version 330 core
layout(location=0) in vec2 aPos;
uniform vec2 uScreen;
void main(){
  vec2 ndc=vec2(aPos.x/uScreen.x*2.0-1.0, 1.0-aPos.y/uScreen.y*2.0);
  gl_Position=vec4(ndc,0.0,1.0);
})";
    const char* fs = R"(#version 330 core
uniform vec4 uColor; out vec4 FragColor; void main(){ FragColor=uColor; })";
    GLuint v = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(v,1,&vs,nullptr); glCompileShader(v);
    GLuint f = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(f,1,&fs,nullptr); glCompileShader(f);
    shader = glCreateProgram();
    glAttachShader(shader,v); glAttachShader(shader,f); glLinkProgram(shader);
    glDeleteShader(v); glDeleteShader(f);
    glGenVertexArrays(1,&vao);
    glGenBuffers(1,&vbo);
    return shader != 0;
}

void Hud::shutdown() {
    if (vbo) glDeleteBuffers(1,&vbo);
    if (vao) glDeleteVertexArrays(1,&vao);
    if (shader) glDeleteProgram(shader);
}

void Hud::drawRect(float x, float y, float w, float h, const glm::vec4& color, int fbW, int fbH) {
    float verts[] = {x,y, x+w,y, x+w,y+h, x,y, x+w,y+h, x,y+h};
    glUseProgram(shader);
    glUniform2f(glGetUniformLocation(shader,"uScreen"), (float)fbW, (float)fbH);
    glUniform4fv(glGetUniformLocation(shader,"uColor"), 1, glm::value_ptr(color));
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,0,(void*)0);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void Hud::drawLine(float x0,float y0,float x1,float y1,const glm::vec4& color,float thickness,int fbW,int fbH) {
    glm::vec2 d(x1-x0,y1-y0);
    float len = glm::length(d);
    if (len < 1e-3f) return;
    glm::vec2 n = glm::vec2(-d.y, d.x) / len * (thickness * 0.5f);
    float verts[] = {
        x0+n.x,y0+n.y, x0-n.x,y0-n.y, x1+n.x,y1+n.y,
        x0-n.x,y0-n.y, x1-n.x,y1-n.y, x1+n.x,y1+n.y
    };
    glUseProgram(shader);
    glUniform2f(glGetUniformLocation(shader,"uScreen"), (float)fbW, (float)fbH);
    glUniform4fv(glGetUniformLocation(shader,"uColor"), 1, glm::value_ptr(color));
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,0,(void*)0);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void Hud::drawRing(float cx,float cy,float r,const glm::vec4& color,int fbW,int fbH,int segs) {
    for (int i=0;i<segs;i++) {
        float a0 = (float)i/segs*2.f*PI;
        float a1 = (float)(i+1)/segs*2.f*PI;
        drawLine(cx+std::cos(a0)*r, cy+std::sin(a0)*r, cx+std::cos(a1)*r, cy+std::sin(a1)*r, color, 1.5f, fbW, fbH);
    }
}

// Minimal 5x7 style strokes for needed charset
void Hud::drawChar(char c, float x, float y, float s, const glm::vec4& color, int fbW, int fbH) {
    auto L = [&](float x0,float y0,float x1,float y1) {
        drawLine(x+x0*s, y+y0*s, x+x1*s, y+y1*s, color, std::max(1.2f, s*0.15f), fbW, fbH);
    };
    c = (char)toupper((unsigned char)c);
    // segments relative 0..4 x, 0..6 y
    switch (c) {
    case 'A': L(0,6,2,0); L(2,0,4,6); L(1,3.5f,3,3.5f); break;
    case 'B': L(0,0,0,6); L(0,0,3,0); L(0,3,3,3); L(0,6,3,6); L(3,0,4,1.5f); L(4,1.5f,3,3); L(3,3,4,4.5f); L(4,4.5f,3,6); break;
    case 'C': L(4,1,2,0); L(2,0,0,2); L(0,2,0,4); L(0,4,2,6); L(2,6,4,5); break;
    case 'D': L(0,0,0,6); L(0,0,2.5f,0); L(2.5f,0,4,2); L(4,2,4,4); L(4,4,2.5f,6); L(2.5f,6,0,6); break;
    case 'E': L(0,0,0,6); L(0,0,4,0); L(0,3,3,3); L(0,6,4,6); break;
    case 'F': L(0,0,0,6); L(0,0,4,0); L(0,3,3,3); break;
    case 'G': L(4,1,2,0); L(2,0,0,2); L(0,2,0,4); L(0,4,2,6); L(2,6,4,5); L(4,5,4,3); L(4,3,2.5f,3); break;
    case 'H': L(0,0,0,6); L(4,0,4,6); L(0,3,4,3); break;
    case 'I': L(1,0,3,0); L(2,0,2,6); L(1,6,3,6); break;
    case 'K': L(0,0,0,6); L(0,3,4,0); L(0,3,4,6); break;
    case 'L': L(0,0,0,6); L(0,6,4,6); break;
    case 'M': L(0,6,0,0); L(0,0,2,3); L(2,3,4,0); L(4,0,4,6); break;
    case 'N': L(0,6,0,0); L(0,0,4,6); L(4,6,4,0); break;
    case 'O': L(1,0,3,0); L(3,0,4,2); L(4,2,4,4); L(4,4,3,6); L(3,6,1,6); L(1,6,0,4); L(0,4,0,2); L(0,2,1,0); break;
    case 'P': L(0,0,0,6); L(0,0,3,0); L(3,0,4,1.5f); L(4,1.5f,3,3); L(3,3,0,3); break;
    case 'R': L(0,0,0,6); L(0,0,3,0); L(3,0,4,1.5f); L(4,1.5f,3,3); L(3,3,0,3); L(2,3,4,6); break;
    case 'S': L(4,1,2,0); L(2,0,0,1); L(0,1,0,2.5f); L(0,2.5f,4,3.5f); L(4,3.5f,4,5); L(4,5,2,6); L(2,6,0,5); break;
    case 'T': L(0,0,4,0); L(2,0,2,6); break;
    case 'U': L(0,0,0,5); L(0,5,2,6); L(2,6,4,5); L(4,5,4,0); break;
    case 'V': L(0,0,2,6); L(2,6,4,0); break;
    case 'W': L(0,0,1,6); L(1,6,2,3); L(2,3,3,6); L(3,6,4,0); break;
    case 'X': L(0,0,4,6); L(4,0,0,6); break;
    case 'Y': L(0,0,2,3); L(4,0,2,3); L(2,3,2,6); break;
    case 'Z': L(0,0,4,0); L(4,0,0,6); L(0,6,4,6); break;
    case '0': L(1,0,3,0); L(3,0,4,1); L(4,1,4,5); L(4,5,3,6); L(3,6,1,6); L(1,6,0,5); L(0,5,0,1); L(0,1,1,0); L(0,5,4,1); break;
    case '1': L(1,1,2,0); L(2,0,2,6); L(1,6,3,6); break;
    case '2': L(0,1,2,0); L(2,0,4,1); L(4,1,0,6); L(0,6,4,6); break;
    case '3': L(0,0,4,0); L(4,0,2,3); L(2,3,4,4); L(4,4,2,6); L(2,6,0,5); break;
    case '4': L(0,0,0,3); L(0,3,4,3); L(3,0,3,6); break;
    case '5': L(4,0,0,0); L(0,0,0,3); L(0,3,3,3); L(3,3,4,4); L(4,4,3,6); L(3,6,0,6); break;
    case '6': L(3,0,0,3); L(0,3,0,5); L(0,5,2,6); L(2,6,4,5); L(4,5,4,4); L(4,4,0,3); break;
    case '7': L(0,0,4,0); L(4,0,1,6); break;
    case '8': L(1,0,3,0); L(3,0,4,1.5f); L(4,1.5f,3,3); L(3,3,1,3); L(1,3,0,1.5f); L(0,1.5f,1,0);
              L(3,3,4,4.5f); L(4,4.5f,3,6); L(3,6,1,6); L(1,6,0,4.5f); L(0,4.5f,1,3); break;
    case '9': L(4,3,0,2); L(0,2,0,1); L(0,1,2,0); L(2,0,4,1); L(4,1,4,5); L(4,5,2,6); L(2,6,0,5); break;
    case '/': L(4,0,0,6); break;
    case '-': L(1,3,3,3); break;
    case '+': L(2,1,2,5); L(0,3,4,3); break;
    case ':': L(2,1.5f,2,2.2f); L(2,4.2f,2,4.9f); break;
    case '.': L(2,5.5f,2.2f,5.7f); break;
    case ' ': break;
    default: break;
    }
}

void Hud::drawText(const std::string& text, float x, float y, float s, const glm::vec4& color, int fbW, int fbH) {
    float cx = x;
    for (char c : text) {
        drawChar(c, cx, y, s, color, fbW, fbH);
        cx += s * 5.2f;
    }
}

void Hud::drawReticle(int fbW, int fbH) {
    float cx = fbW * 0.5f, cy = fbH * 0.5f;
    glm::vec4 c(0.3f, 1.f, 1.f, 0.85f);
    drawLine(cx-18, cy, cx-6, cy, c, 1.5f, fbW, fbH);
    drawLine(cx+6, cy, cx+18, cy, c, 1.5f, fbW, fbH);
    drawLine(cx, cy-18, cx, cy-6, c, 1.5f, fbW, fbH);
    drawLine(cx, cy+6, cx, cy+18, c, 1.5f, fbW, fbH);
    drawRing(cx, cy, 10.f, glm::vec4(0.3f,1.f,1.f,0.35f), fbW, fbH, 24);
}

static bool worldToScreen(const glm::vec3& world, const ChaseCamera& cam, int fbW, int fbH, float& sx, float& sy, float& ndcZ) {
    glm::mat4 vp = cam.proj((float)fbW/fbH) * cam.view();
    glm::vec4 clip = vp * glm::vec4(world, 1.f);
    if (clip.w == 0.f) return false;
    glm::vec3 ndc = glm::vec3(clip) / clip.w;
    ndcZ = ndc.z;
    sx = (ndc.x * 0.5f + 0.5f) * fbW;
    sy = (1.f - (ndc.y * 0.5f + 0.5f)) * fbH;
    return clip.w > 0.f && ndc.z > 0.f && ndc.z < 1.f;
}

void Hud::drawTargetBox(const HudInfo& info, const ChaseCamera& cam, int fbW, int fbH) {
    if (!info.hasTarget) return;
    float sx, sy, z;
    glm::vec4 amber(1.f, 0.75f, 0.2f, 0.9f);
    if (worldToScreen(info.targetWorld, cam, fbW, fbH, sx, sy, z)) {
        float size = clampf(4000.f / std::max(info.targetDist, 1.f), 14.f, 55.f);
        drawLine(sx-size, sy-size, sx-size*0.4f, sy-size, amber, 2.f, fbW, fbH);
        drawLine(sx-size, sy-size, sx-size, sy-size*0.4f, amber, 2.f, fbW, fbH);
        drawLine(sx+size, sy-size, sx+size*0.4f, sy-size, amber, 2.f, fbW, fbH);
        drawLine(sx+size, sy-size, sx+size, sy-size*0.4f, amber, 2.f, fbW, fbH);
        drawLine(sx-size, sy+size, sx-size*0.4f, sy+size, amber, 2.f, fbW, fbH);
        drawLine(sx-size, sy+size, sx-size, sy+size*0.4f, amber, 2.f, fbW, fbH);
        drawLine(sx+size, sy+size, sx+size*0.4f, sy+size, amber, 2.f, fbW, fbH);
        drawLine(sx+size, sy+size, sx+size, sy+size*0.4f, amber, 2.f, fbW, fbH);
        char buf[64];
        std::snprintf(buf, sizeof(buf), "%.0f M", info.targetDist);
        drawText(buf, sx - 20, sy + size + 6, 2.2f, amber, fbW, fbH);
        std::snprintf(buf, sizeof(buf), "%+.0f", info.closingSpeed);
        drawText(buf, sx - 16, sy + size + 22, 2.f, glm::vec4(0.4f,1.f,0.8f,0.85f), fbW, fbH);
    } else {
        // Off-screen indicator
        glm::vec3 to = info.targetWorld - cam.position;
        glm::vec3 local(glm::dot(to, cam.orientation * glm::vec3(1,0,0)),
                        glm::dot(to, cam.orientation * glm::vec3(0,1,0)),
                        glm::dot(to, cam.orientation * glm::vec3(0,0,-1)));
        glm::vec2 dir(local.x, -local.y);
        if (glm::length2(dir) < 1e-4f) dir = glm::vec2(1,0);
        dir = glm::normalize(dir);
        float cx = fbW*0.5f, cy = fbH*0.5f;
        float m = std::min(fbW, fbH) * 0.42f;
        float px = cx + dir.x * m;
        float py = cy + dir.y * m;
        drawLine(px, py, px - dir.x*16 + dir.y*8, py - dir.y*16 - dir.x*8, amber, 2.f, fbW, fbH);
        drawLine(px, py, px - dir.x*16 - dir.y*8, py - dir.y*16 + dir.x*8, amber, 2.f, fbW, fbH);
    }
}

void Hud::drawRadar(const HudInfo& info, int fbW, int fbH) {
    float size = std::min(140.f, fbW * 0.14f);
    float cx = fbW - size - 24.f;
    float cy = fbH - size - 24.f;
    // Background
    drawRect(cx - size, cy - size, size*2, size*2, glm::vec4(0.02f, 0.06f, 0.1f, 0.55f), fbW, fbH);
    drawRing(cx, cy, size, glm::vec4(0.2f, 0.8f, 1.f, 0.45f), fbW, fbH, 48);
    drawRing(cx, cy, size*0.5f, glm::vec4(0.2f, 0.8f, 1.f, 0.25f), fbW, fbH, 32);
    drawLine(cx-size, cy, cx+size, cy, glm::vec4(0.2f,0.7f,0.9f,0.25f), 1.f, fbW, fbH);
    drawLine(cx, cy-size, cx, cy+size, glm::vec4(0.2f,0.7f,0.9f,0.25f), 1.f, fbW, fbH);
    // Forward wedge
    drawLine(cx, cy, cx + info.playerFwd.x * 0.f, cy, glm::vec4(1),1,fbW,fbH); // noop keep
    drawLine(cx, cy - 8, cx, cy, glm::vec4(0.3f,1.f,1.f,0.7f), 2.f, fbW, fbH);

    float range = 280.f;
    // Player at center as cyan triangle
    drawLine(cx, cy-6, cx-5, cy+5, glm::vec4(0.3f,1.f,1.f,1.f), 2.f, fbW, fbH);
    drawLine(cx, cy-6, cx+5, cy+5, glm::vec4(0.3f,1.f,1.f,1.f), 2.f, fbW, fbH);

    for (size_t i = 0; i < info.enemyPositions.size(); i++) {
        if (i < info.enemyAlive.size() && !info.enemyAlive[i]) continue;
        glm::vec3 delta = info.enemyPositions[i] - info.playerPos;
        // Project into player local XZ (right, -forward) and Y up for elevation tint
        float lx = glm::dot(delta, info.playerRight);
        float ly = glm::dot(delta, info.playerUp);
        float lz = glm::dot(delta, info.playerFwd); // forward positive
        float px = cx + (lx / range) * size;
        float py = cy - (lz / range) * size;
        float dist2d = std::sqrt(lx*lx + lz*lz);
        if (dist2d > range) {
            float s = range / dist2d;
            px = cx + (lx / range) * size * s;
            py = cy - (lz / range) * size * s;
        }
        // Clamp to radar circle
        glm::vec2 off(px-cx, py-cy);
        if (glm::length(off) > size - 3.f) off = glm::normalize(off) * (size - 3.f);
        px = cx + off.x; py = cy + off.y;
        float elev = clampf(ly / 80.f, -1.f, 1.f);
        glm::vec4 col = elev > 0.15f ? glm::vec4(1.f,0.85f,0.2f,1.f)
                      : elev < -0.15f ? glm::vec4(1.f,0.35f,0.15f,1.f)
                      : glm::vec4(1.f,0.55f,0.15f,1.f);
        drawRect(px-2.5f, py-2.5f, 5.f, 5.f, col, fbW, fbH);
    }
    drawText("RADAR", cx - size + 4, cy - size - 14, 2.f, glm::vec4(0.3f,0.9f,1.f,0.7f), fbW, fbH);
}

void Hud::draw(const HudInfo& info, const ChaseCamera& cam, int fbW, int fbH) {
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glm::vec4 cyan(0.3f, 1.f, 1.f, 0.9f);
    glm::vec4 amber(1.f, 0.75f, 0.2f, 0.9f);
    glm::vec4 dim(0.3f, 0.8f, 0.9f, 0.55f);

    if (info.state == GameState::Title) {
        drawText("VECTOR DRIFT", fbW*0.5f - 150, fbH*0.22f, 5.5f, cyan, fbW, fbH);
        drawText("3D SPACE COMBAT ARCADE", fbW*0.5f - 145, fbH*0.22f + 45, 2.4f, amber, fbW, fbH);
        float y = fbH * 0.40f;
        float x = fbW * 0.5f - 170;
        drawText("CONTROLS", x, y, 3.f, cyan, fbW, fbH); y += 32;
        drawText("MOUSE  PITCH / YAW", x, y, 2.2f, dim, fbW, fbH); y += 22;
        drawText("Q E    ROLL", x, y, 2.2f, dim, fbW, fbH); y += 22;
        drawText("W      THRUST", x, y, 2.2f, dim, fbW, fbH); y += 22;
        drawText("S      BRAKE / REVERSE", x, y, 2.2f, dim, fbW, fbH); y += 22;
        drawText("LMB / SPACE  FIRE", x, y, 2.2f, dim, fbW, fbH); y += 22;
        drawText("SHIFT / X  DAMP VELOCITY", x, y, 2.2f, dim, fbW, fbH); y += 22;
        drawText("ESC    PAUSE", x, y, 2.2f, dim, fbW, fbH); y += 36;
        drawText("PRESS SPACE TO LAUNCH", fbW*0.5f - 140, y, 2.8f, amber, fbW, fbH);
        drawText("INERTIA IS YOUR WEAPON", fbW*0.5f - 130, fbH*0.88f, 2.2f, glm::vec4(0.5f,0.7f,0.8f,0.6f), fbW, fbH);
        glEnable(GL_DEPTH_TEST);
        return;
    }

    if (info.state == GameState::Paused) {
        drawRect(0, 0, (float)fbW, (float)fbH, glm::vec4(0,0,0,0.45f), fbW, fbH);
        drawText("PAUSED", fbW*0.5f - 70, fbH*0.35f, 5.f, cyan, fbW, fbH);
        float y = fbH*0.48f; float x = fbW*0.5f - 160;
        drawText("MOUSE PITCH/YAW  Q/E ROLL", x, y, 2.f, dim, fbW, fbH); y+=20;
        drawText("W THRUST  S BRAKE  FIRE LMB/SPACE", x, y, 2.f, dim, fbW, fbH); y+=20;
        drawText("SHIFT/X DAMP  ESC RESUME", x, y, 2.f, dim, fbW, fbH);
        glEnable(GL_DEPTH_TEST);
        return;
    }

    if (info.state == GameState::Dead) {
        drawRect(0, 0, (float)fbW, (float)fbH, glm::vec4(0.1f,0,0,0.4f), fbW, fbH);
        drawText("HULL LOST", fbW*0.5f - 90, fbH*0.32f, 5.f, amber, fbW, fbH);
        char buf[64];
        std::snprintf(buf, sizeof(buf), "SCORE %d", info.score);
        drawText(buf, fbW*0.5f - 70, fbH*0.42f, 3.f, cyan, fbW, fbH);
        std::snprintf(buf, sizeof(buf), "WAVE %d  KILLS %d", info.wave, info.kills);
        drawText(buf, fbW*0.5f - 95, fbH*0.42f + 30, 2.4f, dim, fbW, fbH);
        drawText("PRESS R TO RESTART", fbW*0.5f - 120, fbH*0.58f, 2.6f, amber, fbW, fbH);
        glEnable(GL_DEPTH_TEST);
        return;
    }

    // Playing / wave clear HUD
    drawReticle(fbW, fbH);
    drawTargetBox(info, cam, fbW, fbH);
    drawRadar(info, fbW, fbH);

    // Health / shield bars
    float bx = 24, by = fbH - 54;
    drawText("SHIELD", bx, by - 18, 2.f, cyan, fbW, fbH);
    drawRect(bx, by, 180, 10, glm::vec4(0.05f,0.15f,0.2f,0.7f), fbW, fbH);
    drawRect(bx, by, 180.f * (info.shield / std::max(1.f, info.maxShield)), 10, glm::vec4(0.2f,0.85f,1.f,0.85f), fbW, fbH);
    drawText("HULL", bx, by + 16, 2.f, amber, fbW, fbH);
    drawRect(bx, by + 34, 180, 10, glm::vec4(0.15f,0.05f,0.02f,0.7f), fbW, fbH);
    drawRect(bx, by + 34, 180.f * (info.health / std::max(1.f, info.maxHealth)), 10, glm::vec4(1.f,0.45f,0.15f,0.85f), fbW, fbH);

    char buf[64];
    std::snprintf(buf, sizeof(buf), "SPD %.0f", info.playerSpeed);
    drawText(buf, bx, 24, 2.6f, cyan, fbW, fbH);
    std::snprintf(buf, sizeof(buf), "WAVE %d", info.wave);
    drawText(buf, fbW*0.5f - 40, 20, 2.6f, amber, fbW, fbH);
    std::snprintf(buf, sizeof(buf), "SCORE %d", info.score);
    drawText(buf, fbW - 160, 20, 2.6f, cyan, fbW, fbH);

    if (info.state == GameState::WaveClear) {
        drawText("WAVE CLEAR", fbW*0.5f - 90, fbH*0.28f, 4.f, cyan, fbW, fbH);
    }

    glEnable(GL_DEPTH_TEST);
}

} // namespace vd
