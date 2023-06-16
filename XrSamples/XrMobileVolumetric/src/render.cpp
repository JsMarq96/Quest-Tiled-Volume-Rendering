#include "render.h"
#include "raw_meshes.h"
#include "texture.h"
#include <cstdint>

#include <android/log.h>

#include "surface_nets.h"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/matrix_projection.hpp"
#include "../../../glm/glm/vec3.hpp"
#include "../../../glm/glm/mat4x4.hpp"

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


void Render::sInstance::init(sOpenXRFramebuffer *openxr_framebuffer) {
    // Create FBOs from the openxr_framebuffer's swapchain
    for(uint8_t eye = 0; eye < MAX_EYE_NUMBER; eye++) {
        for(uint32_t i = 0; i < openxr_framebuffer[eye].swapchain_length; i++){
            // Color texture
            const uint32_t gl_color_text = openxr_framebuffer[eye].swapchain_images[i].image;

            const uint8_t color_text_id = material_man.get_new_texture();

            sTexture *curr_color_tex = &material_man.textures[color_text_id];
            curr_color_tex->texture_id = gl_color_text;

            curr_color_tex->config(GL_TEXTURE_2D,
                                   false);

            // Depth rbo
            const uint8_t depth_rbo_id = rbo_count++;
            RBO_init(depth_rbo_id,
                     openxr_framebuffer[eye].width,
                     openxr_framebuffer[eye].height,
                     GL_DEPTH_COMPONENT24);

            // Create FBO
            const uint8_t eye_fbo_id = fbo_count++;
            FBO_init(eye_fbo_id,
                     openxr_framebuffer[eye].width,
                     openxr_framebuffer[eye].height);
            glBindFramebuffer(GL_FRAMEBUFFER,
                              fbos[eye_fbo_id].id);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER,
                                      GL_DEPTH_ATTACHMENT,
                                      GL_RENDERBUFFER,
                                      rbos[depth_rbo_id].id);
            glFramebufferTexture2D(GL_FRAMEBUFFER,
                                   GL_COLOR_ATTACHMENT0,
                                   GL_TEXTURE_2D,
                                   gl_color_text,
                                   0);

            assert(glCheckFramebufferStatus(GL_FRAMEBUFFER == GL_FRAMEBUFFER_COMPLETE) && "Failed FBO creation");
            glBindFramebuffer(GL_FRAMEBUFFER,
                              0);

            framebuffer.color_textures[eye][i] = color_text_id;
            framebuffer.depth_rbos[eye][i] = depth_rbo_id;
            framebuffer.fbos[eye][i] = eye_fbo_id;
        }
        framebuffer.openxr_framebufffs = openxr_framebuffer;
    }


    // Set default render config

    current_state.depth_test_enabled = true;
    glEnable(GL_DEPTH_TEST);
    glDepthMask(current_state.write_to_depth_buffer);
    glDepthFunc(current_state.depth_function);

    current_state.culling_enabled = true;
    glEnable(GL_CULL_FACE);
    glCullFace(current_state.culling_mode);
    glFrontFace(current_state.front_face);

    current_state.set_default();

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

double get_time() {
    struct timespec res;
    clock_gettime(CLOCK_REALTIME, &res);
    return 1000.0 * res.tv_sec + (double) res.tv_nsec / 1e6;

}

void Render::sInstance::empty_render() {
    for(uint16_t eye = 0; eye < MAX_EYE_NUMBER; eye++) {
        // Bind an FBO target of the OpenXR swapchain
        uint32_t curr_swapchain_index = framebuffer.openxr_framebufffs[eye].adquire();
        FBO_bind(framebuffer.fbos[eye][curr_swapchain_index]);
        glClearColor(1.0,
                     0.0,
                     0.0,
                     1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        framebuffer.openxr_framebufffs[eye].release();
        FBO_unbind();
    }
}

// https://gitlab.freedesktop.org/monado/demos/xrgears/-/blob/master/src/pipeline_equirect.cpp#L282
void Render::sInstance::render_frame(const bool clean_frame,
                                     const glm::mat4x4 *view_mats,
                                     const glm::mat4x4 *proj_mats,
                                     const glm::mat4x4 *viewproj_mats) {
    for(uint16_t eye = 0; eye < MAX_EYE_NUMBER; eye++) {

        for(uint16_t j = 0; j < render_pass_size; j++) {
            sRenderPass &pass = render_passes[j];

            if (pass.target == FBO_TARGET) {
                // Bind an FBO target
                FBO_bind(pass.fbo_id);
            } else {
                // Bind an FBO target of the OpenXR swapchain
                uint32_t curr_swapchain_index = framebuffer.openxr_framebufffs[eye].adquire();
                FBO_bind(framebuffer.fbos[eye][curr_swapchain_index]);
            }

            // Clear the curent buffer
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
                    shader.set_uniform_matrix4("u_model_mat",
                                               model);
                    shader.set_uniform_matrix4("u_vp_mat",
                                               viewproj_mats[eye]);
                    shader.set_uniform_matrix4("u_view_mat",
                                               view_mats[eye]);
                    shader.set_uniform_matrix4("u_proj_mat",
                                               proj_mats[eye]);

                    //glm::vec3 camera_local = glm::vec3( model_invert * glm::inverse(view_mats[eye]) * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
                    shader.set_uniform_vector("u_camera_eye_local",
                                              (view_mats[eye]) * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
                }

                shader.set_uniform("u_time",
                                   (float) get_time());

                glEnable(GL_CULL_FACE);
                if (mesh.is_indexed) {
                    glDrawElements(mesh.primitive,
                                   mesh.primitive_count,
                                   GL_UNSIGNED_SHORT,
                                   0);
                } else {
                    glDrawArrays(mesh.primitive,
                                 0,
                                 mesh.primitive_count);
                }

                material_man.disable();
            }

            if (pass.target != FBO_TARGET) {
                framebuffer.openxr_framebufffs[eye].release();
            }
        }
    }
    FBO_unbind();
}



// FBO methods ===================
void Render::sInstance::FBO_init(const uint8_t fbo_id,
                                 const uint32_t width_i,
                                 const uint32_t height_i) {
    glGenFramebuffers(1,
                      &fbos[fbo_id].id);
    fbos[fbo_id].width = width_i;
    fbos[fbo_id].height = height_i;
}

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

    return 0;
}


void Render::sInstance::RBO_init(const uint8_t rbo_id,
                                 const uint32_t width_i,
                                 const uint32_t height_i,
                                 const uint32_t buffer_format) {
    glGenRenderbuffers(1,
                       &rbos[rbo_id].id);
    glBindRenderbuffer(GL_RENDERBUFFER,
                       rbos[rbo_id].id);
    glRenderbufferStorage(GL_RENDERBUFFER,
                          buffer_format,
                          width_i,
                          height_i);
    glBindRenderbuffer(GL_RENDERBUFFER,
                       0);

    rbos[rbo_id].width = width_i;
    rbos[rbo_id].height = height_i;
}