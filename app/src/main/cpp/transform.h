#ifndef TRANSFORM_H_
#define TRANSFORM_H_

#include "glm/ext/matrix_double4x4.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/vector_float3.hpp"
#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>


#include <iostream>
#include <glm/gtx/string_cast.hpp>
struct sTransform {
    glm::vec3 position{0.0f, 0.0f, 0.0f};
    glm::vec3 scale{1.0f, 1.0f, 1.0f};
    glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};

    void set_identity() {
        position = glm::vec3{0.0f, 0.0f, 0.0f};
        scale = glm::vec3{1.0f, 1.0f, 1.0f};
        rotation = glm::quat{1.0f, 0.0f, 0.0f, 0.0f};
    }

    glm::mat4x4 get_model() const {
        glm::mat4x4 rot_mat = glm::mat4_cast(rotation);

        return glm::scale(glm::translate(glm::mat4(1.0f), position) * rot_mat, scale);
    }

    glm::vec3 rotate_vector(const glm::vec3 &vec) const {
        // Use hamiltonian product
        glm::quat P = {0.0f, vec.x, vec.y, vec.z};
        glm::quat rotation_neg = {rotation.w, -rotation.x, -rotation.y, -rotation.z};
        glm::quat rot_vec = rotation * P * rotation_neg;

        return {rot_vec.x, rot_vec.y, rot_vec.z};
    }
};

#endif // TRANSFORM_H_
