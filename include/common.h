#pragma once
#include <cmath>
#include <cstdint>
#include <vector>
#include <string>
#include <algorithm>
#include <random>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>
#include <glm/gtx/quaternion.hpp>

namespace vd {

inline constexpr float PI = 3.14159265358979323846f;
inline constexpr float DEG = PI / 180.f;

struct RNG {
    std::mt19937 gen{std::random_device{}()};
    float uniform(float a = 0.f, float b = 1.f) {
        return std::uniform_real_distribution<float>(a, b)(gen);
    }
    int uniformi(int a, int b) {
        return std::uniform_int_distribution<int>(a, b)(gen);
    }
    glm::vec3 unit() {
        float z = uniform(-1.f, 1.f);
        float t = uniform(0.f, 2.f * PI);
        float r = std::sqrt(std::max(0.f, 1.f - z * z));
        return {r * std::cos(t), r * std::sin(t), z};
    }
    glm::vec3 inSphere(float radius) { return unit() * uniform(0.f, radius); }
};

inline float clampf(float v, float lo, float hi) {
    return std::max(lo, std::min(hi, v));
}
inline float lerpf(float a, float b, float t) { return a + (b - a) * t; }
inline glm::vec3 safeNormalize(const glm::vec3& v, const glm::vec3& fallback = {0, 0, -1}) {
    float l2 = glm::length2(v);
    if (l2 < 1e-10f) return fallback;
    return v * (1.f / std::sqrt(l2));
}

enum class GameState { Title, Playing, Paused, Dead, WaveClear };

} // namespace vd
