#ifndef RENDER_QUAD_H_
#define RENDER_QUAD_H_

#include <GL/gl3w.h>
#include <cstdint>
#include <string.h>
#include <iostream>
#include <cassert>
#include <stdlib.h>
#include <glm/glm.hpp>
#include "glm/ext/matrix_transform.hpp"

#include "glcorearb.h"
#include "glm/fwd.hpp"
#include "texture.h"
#include "material.h"


static const float quad_geometry[] = {
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
        1.0f, -1.0f,  1.0f, 0.0f,

        -1.0f,  1.0f,  0.0f, 1.0f,
        1.0f, -1.0f,  1.0f, 0.0f,
        1.0f,  1.0f,  1.0f, 1.0f
};


// TODO: Add a postprocessing element?
struct sQuadRenderer {
    uint32_t VAO = 0;
    uint32_t VBO = 0;

    sMaterial quad_material;

    glm::mat4x4 models_axis_y[256] = {};
    glm::mat4x4 models_axis_x[256] = {};
    glm::mat4x4 models_axis_z[256] = {};

    glm::mat4x4 models_axis_minus_y[256] = {};
    glm::mat4x4 models_axis_minus_x[256] = {};
    glm::mat4x4 models_axis_minus_z[256] = {};

    void init() {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quad_geometry), quad_geometry, GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*) 0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*) (2 * sizeof(float)));

        glBindVertexArray(0);

        // TODO: Config shader & material
        //quad_material.shader = shader;
        // X aligned models
        float movement_delta = 2.0f / 256.0f;// size of a voxel in world space

        glm::vec3 starting_pos ={0.0f, 0.0f, -1.0f};
        for(uint32_t i = 0; i < 256; i++) {
            models_axis_z[i] = glm::translate(glm::mat4x4(1.0f), starting_pos);
            //models[i] = glm::rotate(glm::mat4x4(1.0f), glm::radians(90.0f), {1.0f, 0.0f, 0.0f});
            starting_pos.z += movement_delta;
        }

        starting_pos = {0.0f, -1.0f, 0.0f};
        for(uint32_t i = 0; i < 256; i++) {
            models_axis_y[i] = glm::rotate(glm::translate(glm::mat4x4(1.0f), starting_pos), glm::radians(90.0f), {1.0f, 0.0f, 0.0f});
            starting_pos.y += movement_delta;
        }

        starting_pos = {-1.0f, 0.0f, 0.0f};
        for(uint32_t i = 0; i < 256; i++) {
            models_axis_x[i] = glm::rotate(glm::translate(glm::mat4x4(1.0f), starting_pos), glm::radians(90.0f), {0.0f, 1.0f, 0.0f});
            starting_pos.x += movement_delta;
        }

        starting_pos = {0.0f, 0.0f, 1.0f};
        for(uint32_t i = 0; i < 256; i++) {
            models_axis_minus_z[i] = glm::rotate(glm::translate(glm::mat4x4(1.0f), starting_pos), glm::radians(180.0f), {1.0f, 0.0f, 0.0f});
            starting_pos.z -= movement_delta;
        }

        starting_pos = {0.0f, 1.0f, 0.0f};
        for(uint32_t i = 0; i < 256; i++) {
            models_axis_minus_y[i] = glm::rotate(glm::translate(glm::mat4x4(1.0f), starting_pos), glm::radians(180.0f), {1.0f, 0.0f, 0.0f});
            starting_pos.y -= movement_delta;
        }

        starting_pos = {1.0f, 0.0f, 0.0f};
        for(uint32_t i = 0; i < 256; i++) {
            models_axis_minus_x[i] = glm::rotate(glm::translate(glm::mat4x4(1.0f), starting_pos), glm::radians(180.0f), {0.0f, 1.0f, 0.0f});
            starting_pos.x -= movement_delta;
        }
    }

    void render(//const glm::mat4x4 &rotation_mat,
            const glm::mat4x4 &view_proj,
            const glm::vec3 &camera_position) const {
        float facing_x  = (glm::dot(glm::vec3{1.0f, 0.0f, 0.0f}, glm::normalize(glm::vec3(0.0f) - camera_position)));
        float facing_y = (glm::dot(glm::vec3{0.0f, 1.0f, 0.0f}, glm::normalize(glm::vec3(0.0f) - camera_position)));
        float facing_z =  (glm::dot(glm::vec3{0.0f, 0.0f, 1.0f}, glm::normalize(glm::vec3(0.0f) - camera_position)));

        float sign_x = glm::sign(facing_x);
        float sign_y = glm::sign(facing_y);
        float sign_z = glm::sign(facing_z);

        facing_x = glm::abs(facing_x);
        facing_z = glm::abs(facing_z);
        facing_y = glm::abs(facing_y);

        const glm::mat4x4 *models = NULL;
        if (facing_z > facing_y) {
            if (facing_y > facing_x) {
                // Facing Z the most
                if (sign_z < 0.0) {
                    models = models_axis_z;
                } else {
                    models = models_axis_minus_z;
                }

            } else {
                // Facing X the most
                if (sign_x < 0.0) {
                    models = models_axis_x;
                } else {
                    models = models_axis_minus_x;
                }
            }
        } else {
            if (facing_z > facing_x) {
                // Facing Y the most
                if (sign_y < 0.0) {
                    models = models_axis_y;
                } else {
                    models = models_axis_minus_y;
                }
            } else {
                // Facing X the most
                if (sign_x < 0.0) {
                    models = models_axis_x;
                } else {
                    models = models_axis_minus_x;
                }
            }
        }

        glEnable(GL_BLEND);
        glDisable(GL_CULL_FACE);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glBindVertexArray(VAO);
        quad_material.enable();

        quad_material.shader.set_uniform_matrix4("u_vp_mat", view_proj);
        quad_material.shader.set_uniform_vector("u_camera_position", camera_position);
        //quad_material.shader.set_uniform_matrix4("u_rotation_mat", rotation_mat);

        for(int i = 0; i < 256; i++) {
            quad_material.shader.set_uniform_matrix4("u_model_mat", models[i]);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }

        //glDrawArrays(GL_TRIANGLES, 0, 6);

        quad_material.disable();
        glBindVertexArray(0);
        glDisable(GL_BLEND);
        glEnable(GL_CULL_FACE);
    }
};

#endif // RENDER_QUAD_H_
