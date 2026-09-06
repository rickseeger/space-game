#pragma once
#include "common.h"
#include "ship.h"

namespace vd {

struct ChaseCamera {
    glm::vec3 position{0, 4, 12};
    glm::quat orientation{1, 0, 0, 0};
    float distance = 11.f;
    float height = 3.2f;
    float lookAhead = 8.f;
    float followSmooth = 8.f;
    float fov = 70.f;
    float nearP = 0.3f;
    float farP = 2000.f;

    void update(const Ship& target, float dt);
    glm::mat4 view() const;
    glm::mat4 proj(float aspect) const;
};

} // namespace vd
