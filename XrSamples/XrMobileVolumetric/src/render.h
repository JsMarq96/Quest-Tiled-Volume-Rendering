#ifndef RENDER_H_
#define RENDER_H_

#include <GLES3/gl3.h>
#include <cstdint>
#ifndef __EMSCRIPTEN__
#include <GLES3/gl3.h>
#endif

#include "transform.h"
#include "shader.h"
#include "material.h"
#include "fbo.h"
#include "rbo.h"
#include "raw_shaders.h"
#include "openxr_instance.h"
#define MAX_SWAPCHAIN_SIZE 5
#define MESH_TOTAL_COUNT 20
#define FBO_TOTAL_COUNT 15
#define RBO_TOTAL_COUNT 15
#define DRAW_CALL_STACK_SIZE 30
#define RENDER_PASS_COUNT 5
/**
 * A Wrapper for the rendering backend, for now with webgl
 * TODO:
 *  1) Basic VAO, VBO functionality
 *  2) Add perspective
 *  2) Basic I/O of meshes
 *  3) Basic I/O of textures
 *  4) Scene representation
 *  4) GLTF import
 *  5) Culling (VR culling??)
 *  5) Render graph..?
 *
 *  Try an "stateless" API?
 *  Keep on instance state of the OpenGL flags
 *  Set this flags on each drawcall, and change when necesary
 * */
namespace Render {

    enum eRenderPassTarget : uint8_t {
        SCREEN_TARGET = 0,
        FBO_TARGET
    };

    struct sGLState {
        // Depth test config
        bool depth_test_enabled = true;
        bool write_to_depth_buffer = true;
        uint32_t depth_function = GL_LESS;

        // Culling info
        bool culling_enabled = true;
        uint32_t culling_mode = GL_BACK;
        uint32_t front_face = GL_CCW;

        // Blending
        bool blending_enabled = true;
        uint32_t blend_func_x = GL_ONE;
        uint32_t blend_func_y = GL_ONE_MINUS_SRC_ALPHA;

        void set_default() {
            depth_test_enabled = true;
            write_to_depth_buffer = true;
            depth_function = GL_LESS;

            // Culling info
            culling_enabled = true;
            culling_mode = GL_BACK;
            front_face = GL_CCW;

            // Blending
            blending_enabled = true;
            blend_func_x = GL_ONE;
            blend_func_y = GL_ONE_MINUS_SRC_ALPHA;
        }
    };

    struct sFramebuffer {
        uint8_t fbos[MAX_EYE_NUMBER][MAX_SWAPCHAIN_SIZE] = {};
        uint8_t color_textures[MAX_EYE_NUMBER][MAX_SWAPCHAIN_SIZE] = {};
        uint8_t depth_rbos[MAX_EYE_NUMBER][MAX_SWAPCHAIN_SIZE] = {};
        sOpenXRFramebuffer *openxr_framebufffs;
    };


    // TODO: Parenting
    struct sDrawCall {
        uint8_t mesh_id;
        uint8_t material_id;

        bool use_transform = true;
        sTransform transform;

        sGLState call_state;

        bool enabled = true;
    };

    struct sMeshBuffers {
        uint32_t VAO;
        uint32_t VBO;
        uint32_t EBO;

        uint32_t primitive_count;
        uint32_t primitive;

        bool is_indexed = false;

        void init_with_triangles(const float *geometry,
                                 const uint32_t geometry_size,
                                 const uint16_t *indices,
                                 const uint32_t indices_size);
    };

    struct sRenderPass {
        bool clean_viewport = true;
        uint32_t clean_config;
        float rgba_clear_values[4] = {0.0f, 0.0f, 0.0f, 1.0f};

        eRenderPassTarget target = SCREEN_TARGET;

        bool use_color_attachment0 = false;
        bool use_color_attachment1 = false;

        uint8_t color_attachment_pass0_id = 0;
        uint8_t color_attachment_pass1_id = 0;

        uint8_t fbo_id;

        uint8_t draw_stack_size = 0;
        sDrawCall draw_stack[DRAW_CALL_STACK_SIZE];
    };
    
    struct sInstance {
        sFramebuffer framebuffer = {};

        sGLState current_state;

        uint32_t base_framebuffer = 0;

        uint8_t quad_mesh_id = 0;

        uint8_t fbo_count = 0;
        sFBO fbos[FBO_TOTAL_COUNT];
        uint8_t rbo_count = 0;
        sRBO rbos[RBO_TOTAL_COUNT];
        uint8_t meshes_count = 0;
        sMeshBuffers meshes[MESH_TOTAL_COUNT];

        sMaterialManager material_man = {};

        uint16_t render_pass_size = 0;
        sRenderPass render_passes[RENDER_PASS_COUNT];

        void init(sOpenXRFramebuffer *openxr_framebuffer);
        void change_graphic_state(const sGLState &new_state);
        void render_frame(const bool clean_frame,
                          const glm::mat4x4 *view_mats,
                          const glm::mat4x4 *proj_mats);

        // Inlines
        inline uint8_t add_drawcall_to_pass(const uint8_t pass_id,
                                            const sDrawCall &draw_call) {
            sRenderPass *pass = &render_passes[pass_id];

            pass->draw_stack[pass->draw_stack_size] = draw_call;

            return pass->draw_stack_size++;
        }

        inline uint8_t add_quad_pass(const eRenderPassTarget target,
                                     const uint8_t fbo_id,
                                     const char* fragment_shader,
                                     const sMaterialTexConstructor &mat_constructor) {
            uint8_t quad_mat_id = material_man.add_material(material_man.add_raw_shader(RawShaders::quad_vertex,
                                                                                        fragment_shader),
                                                            mat_constructor);

            render_passes[render_pass_size].target = target;
            render_passes[render_pass_size].fbo_id = fbo_id;

            add_drawcall_to_pass(render_pass_size,
                                 sDrawCall{
                                     .mesh_id = quad_mesh_id,
                                     .material_id = quad_mat_id,
                                     .use_transform = false,
                                     .call_state = sGLState{
                                         .depth_test_enabled = false,
                                         .culling_enabled = false
                                }});

            return render_pass_size++;
        }

        inline uint8_t add_render_pass(const eRenderPassTarget target,
                                       const uint8_t fbo_id) {
            render_passes[render_pass_size].target = target;
            render_passes[render_pass_size].fbo_id = fbo_id;
            return render_pass_size++;
        }

        inline uint8_t add_render_pass(const eRenderPassTarget target,
                                       const uint8_t fbo_id,
                                       const uint8_t input_fbo) {
            render_passes[render_pass_size].target = target;
            render_passes[render_pass_size].fbo_id = fbo_id;


            if (fbos[fbo_id].attachment_use == JUST_COLOR) {
                render_passes[render_pass_size].use_color_attachment0 = true;
                render_passes[render_pass_size].color_attachment_pass0_id = input_fbo;
            } else if (fbos[fbo_id].attachment_use == JUST_DUAL_COLOR) {
                render_passes[render_pass_size].use_color_attachment0 = true;
                render_passes[render_pass_size].color_attachment_pass0_id = input_fbo;
                render_passes[render_pass_size].use_color_attachment1 = true;
                render_passes[render_pass_size].color_attachment_pass1_id = input_fbo;
            }

            return render_pass_size++;
        }

        inline uint8_t get_new_fbo_id() {
            assert(fbo_count < FBO_TOTAL_COUNT && "No more space for FBOs");
            return fbo_count++;
        }


        inline uint8_t get_new_mesh_id() {
            assert(meshes_count < MESH_TOTAL_COUNT && "No more space for meshes");
            return meshes_count++;
        }

        inline void use_drawcall(const uint8_t pass_id,
                                 const uint8_t draw_call,
                                 const bool use) {
            render_passes[pass_id].draw_stack[draw_call].enabled = use;
        }

        inline uint32_t get_texture_of_fbo(const uint8_t fbo_id) const {
            return material_man.textures[fbos[fbo_id].color_attachment0].texture_id;
        }

        inline sDrawCall* get_draw_call(const uint8_t pass_id,
                                        const uint8_t draw_call) {
            return &render_passes[pass_id].draw_stack[draw_call];
        }

        inline void set_transform_of_drawcall(const uint8_t pass_id,
                                              const uint8_t draw_call,
                                              const sTransform &transf) {
            render_passes[pass_id].draw_stack[draw_call].transform = transf;
        }

        // FBO Functions =====
        void FBO_init(const uint8_t fbo_id);

        void FBO_init_with_single_color(const uint8_t fbo_id,
                                        const uint32_t width_i,
                                        const uint32_t height_i);
        void FBO_init_with_dual_color(const uint8_t fbo_id,
                                      const uint32_t width_i,
                                      const uint32_t height_i);
        void FBO_clean(const uint8_t fbo_id);

        uint8_t FBO_reinit(const uint8_t fbo_id,
                           const uint32_t width_i,
                           const uint32_t height_i);

        inline void FBO_bind(const uint8_t fbo_id) const {
            glBindFramebuffer(GL_FRAMEBUFFER, fbos[fbo_id].id);
        }
        inline void FBO_unbind() const {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }

        // RBO Functions
        void RBO_init(const uint8_t rbo_id,
                      const uint32_t width_i,
                      const uint32_t height_i,
                      const uint32_t buffer_format);

        inline void RBO_bind(const uint8_t rbo_id) const {
            rbos[rbo_id].bind(GL_DRAW_FRAMEBUFFER);
        }
        inline void FBO_unbind(const uint8_t rbo_id) const {
            rbos[rbo_id].unbind();
        }
    };

};

#endif // RENDER_H_
