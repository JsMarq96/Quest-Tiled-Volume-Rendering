#include "render.h"
#include "raw_meshes.h"
#include "texture.h"
#include <cstdint>

#ifdef __EMSCRIPTEN__
#include <GLES3/gl3.h>
#include <emscripten/emscripten.h>
#include <emscripten/html5_webgl.h>
#include <emscripten/html5.h>
#include <webgl/webgl2.h>
#endif

#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/matrix_projection.hpp"

void Render::sMeshBuffers::init_with_triangles(const float *geometry,
                                               const uint32_t geometry_size,
                                               const uint16_t *indices,
                                               const uint32_t indices_size) {
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, geometry_size, geometry, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices_size, indices, GL_STATIC_DRAW);

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    // Load vertices
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    // Vertex position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*) 0);

    // UV coords
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*) (sizeof(float) * 3));

    // Vertex normal
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*) (sizeof(float) * 5));

    // Load vertex indexing
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindVertexArray(0);

    primitive = GL_TRIANGLES;
    primitive_count = (uint16_t) (indices_size / sizeof(uint16_t));
    is_indexed = true;
}


void Render::sInstance::init() {
    // Set default
    current_state.depth_test_enabled = true;
    glEnable(GL_DEPTH_TEST);
    glDepthMask(current_state.write_to_depth_buffer);
    glDepthFunc(current_state.depth_function);

    current_state.culling_enabled = true;
    glEnable(GL_CULL_FACE);
    glCullFace(current_state.culling_mode);
    glFrontFace(current_state.front_face);

    // Init quad mesh
    quad_mesh_id = meshes_count++;
    meshes[quad_mesh_id].init_with_triangles(RawMesh::quad_geometry,
                                             sizeof(RawMesh::quad_geometry),
                                             RawMesh::quad_indices,
                                             sizeof(RawMesh::quad_indices));
}

void Render::sInstance::change_graphic_state(const sGLState &new_state) {
    // Depth
    if (current_state.depth_test_enabled != new_state.depth_test_enabled) {
        if (new_state.depth_test_enabled) {
            glEnable(GL_DEPTH_TEST);
        } else {
            glDisable(GL_DEPTH_TEST);
        }
       current_state.depth_test_enabled = new_state.depth_test_enabled;
    }

    if (current_state.write_to_depth_buffer != new_state.write_to_depth_buffer) {
        glDepthMask(new_state.write_to_depth_buffer);
        current_state.write_to_depth_buffer = new_state.write_to_depth_buffer;
    }

    if (current_state.depth_function != new_state.depth_function) {
        glDepthFunc(new_state.depth_function);
        current_state.depth_function = new_state.depth_function;
    }

    // Face Culling
    if (current_state.culling_enabled != new_state.culling_enabled) {
        if (current_state.culling_enabled) {
            glEnable(GL_CULL_FACE);
        } else {
            glDisable(GL_CULL_FACE);
        }

        current_state.culling_enabled = new_state.culling_enabled;
    }

    if (current_state.culling_mode != new_state.culling_mode) {
        glCullFace(new_state.culling_mode);
        current_state.culling_mode = new_state.culling_mode;
    }

    if (current_state.front_face != new_state.front_face) {
        glFrontFace(new_state.front_face);
        current_state.front_face = new_state.front_face;
    }

    // Blending
    if (current_state.blending_enabled != new_state.blending_enabled) {
        if (new_state.blending_enabled) {
            glEnable(GL_BLEND);
        } else {
            glDisable(GL_BLEND);
        }

        current_state.blending_enabled = new_state.blending_enabled;

        if (current_state.blend_func_x != new_state.blend_func_x ||
            current_state.blend_func_y != new_state.blend_func_y) {
            glBlendFunc(new_state.blend_func_x, new_state.blend_func_y);
        }

        current_state.blend_func_x = new_state.blend_func_x;
        current_state.blend_func_y = new_state.blend_func_y;
    }
}

void Render::sInstance::render_frame(const glm::mat4x4 &view_proj_mat,
                                     const glm::vec3 &cam_pos,
                                     const int32_t width,
                                     const int32_t heigth,
                                     const bool clean_frame = true) {
    for(uint16_t j = 0; j < render_pass_size; j++) {
        // Bind the render pass
        sRenderPass &pass = render_passes[j];

        if (pass.target == FBO_TARGET) {
            sFBO &curr_fbo = fbos[pass.fbo_id];

            if (curr_fbo.width != width ||
                curr_fbo.height != heigth) {
                curr_fbo.reinit(width, heigth);
            }

            curr_fbo.bind();
        } else {
            glBindFramebuffer(GL_FRAMEBUFFER, base_framebuffer);
        }

        // Celar the curent buffer
        if (pass.clean_viewport && clean_frame) {
            glClearColor(pass.rgba_clear_values[0],
                         pass.rgba_clear_values[1],
                         pass.rgba_clear_values[2],
                         pass.rgba_clear_values[3]);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        }

        // Run the render calls
        glm::mat4x4 model, model_invert;
        for(uint16_t i = 0; i < pass.draw_stack_size; i++) {
            sDrawCall &draw_call = pass.draw_stack[i];

            if (!draw_call.enabled) {
                continue;
            }

            sMaterialInstance &material = material_man.materials[draw_call.material_id];
            sShader &shader = material_man.shaders[material.shader_id];
            sMeshBuffers &mesh = meshes[draw_call.mesh_id];

            model = draw_call.transform.get_model();
            model_invert = glm::inverse(model);

            change_graphic_state(draw_call.call_state);

            material_man.enable(draw_call.material_id);

            glBindVertexArray(mesh.VAO);

            if (draw_call.use_transform) {
                shader.set_uniform_matrix4("u_model_mat", model);
                shader.set_uniform_matrix4("u_vp_mat", view_proj_mat);
                //shader.set_uniform_vector("u_camera_eye_local", cam_pos);
                shader.set_uniform_vector("u_camera_eye_local", glm::vec3(model_invert * glm::vec4(cam_pos, 1.0f)));
            }

            if (mesh.is_indexed) {
                glDrawElements(mesh.primitive, mesh.primitive_count, GL_UNSIGNED_SHORT, 0);
            } else {
                glDrawArrays(mesh.primitive, 0, mesh.primitive_count);
            }

            material_man.disable();
        }
    }
}



// FBO methods ===================
void Render::sInstance::FBO_init_with_single_color(const uint8_t fbo_id,
                                                   const uint32_t width_i,
                                                   const uint32_t height_i) {
    sFBO *fbo = &fbos[fbo_id];
    fbo->attachment_use = JUST_COLOR;

    fbo->width = width_i;
    fbo->height = height_i;

    glGenFramebuffers(1, &fbo->id);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo->id);

    fbo->color_attachment0 = material_man.get_new_texture();
    sTexture *color_attachment0 = &material_man.textures[fbo->color_attachment0];

    color_attachment0->create_empty2D_with_size(fbo->width,
                                                fbo->height);

    glFramebufferTexture2D(GL_FRAMEBUFFER,
                           GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D,
                           color_attachment0->texture_id,
                           0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

}

void Render::sInstance::FBO_init_with_dual_color(const uint8_t fbo_id,
                                                 const uint32_t width_i,
                                                 const uint32_t height_i) {
    sFBO *fbo = &fbos[fbo_id];
    fbo->attachment_use = JUST_COLOR;

    fbo->width = width_i;
    fbo->height = height_i;

    glGenFramebuffers(1, &fbo->id);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo->id);

    fbo->color_attachment0 = material_man.get_new_texture();
    fbo->color_attachment1 = material_man.get_new_texture();
    sTexture *color_attachment0 = &material_man.textures[fbo->color_attachment0];
    sTexture *color_attachment1 = &material_man.textures[fbo->color_attachment1];

    color_attachment0->create_empty2D_with_size(fbo->width,
                                                fbo->height);

   color_attachment1->create_empty2D_with_size(fbo->width,
                                                fbo->height);


    glFramebufferTexture2D(GL_FRAMEBUFFER,
                           GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D,
                           color_attachment0->texture_id,
                           0);

    glFramebufferTexture2D(GL_FRAMEBUFFER,
                           GL_COLOR_ATTACHMENT1,
                           GL_TEXTURE_2D,
                           color_attachment1->texture_id,
                           0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Render::sInstance::FBO_clean(const uint8_t fbo_id) {
    sFBO &fbo = fbos[fbo_id];
    glDeleteTextures(1, &(material_man.textures[fbo.color_attachment0].texture_id));
    glDeleteTextures(1, &(material_man.textures[fbo.color_attachment1].texture_id));
    glDeleteFramebuffers(1, &fbo.id);
}

uint8_t Render::sInstance::FBO_reinit(const uint8_t fbo_id,
                                      const uint32_t width_i,
                                      const uint32_t height_i) {
    FBO_clean(fbo_id);

    switch (fbos[fbo_id].attachment_use) {
        case JUST_COLOR:
            FBO_init_with_single_color(fbo_id,
                                       width_i,
                                       height_i);
            break;
        case JUST_DUAL_COLOR:
            FBO_init_with_dual_color(fbo_id,
                                     width_i,
                                     height_i);
            break;
        case JUST_DEPTH:
            // TODO
            break;
        case COLOR_AND_DEPTH:
            // TODO
            break;
    }

}
