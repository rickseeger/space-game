#include "camera.h"

namespace vd {

void ChaseCamera::update(const Ship& target, float dt) {
    glm::vec3 back = -target.forward();
    glm::vec3 desired = target.position + back * distance + target.up() * height;
    // Soft follow
    float t = 1.f - std::exp(-followSmooth * dt);
    position = glm::mix(position, desired, t);

    glm::vec3 lookAt = target.position + target.forward() * lookAhead + target.velocity * 0.05f;
    glm::vec3 fwd = safeNormalize(lookAt - position);
    glm::vec3 worldUp = glm::mix(glm::vec3(0,1,0), target.up(), 0.65f);
    worldUp = safeNormalize(worldUp, glm::vec3(0,1,0));
    glm::vec3 r = safeNormalize(glm::cross(fwd, worldUp), target.right());
    glm::vec3 u = glm::cross(r, fwd);
    glm::mat3 rot(r, u, -fwd);
    orientation = glm::normalize(glm::quat_cast(rot));
}

glm::mat4 ChaseCamera::view() const {
    return glm::lookAt(position, position + (orientation * glm::vec3(0,0,-1)), orientation * glm::vec3(0,1,0));
}

glm::mat4 ChaseCamera::proj(float aspect) const {
    return glm::perspective(glm::radians(fov), aspect, nearP, farP);
}

} // namespace vd
