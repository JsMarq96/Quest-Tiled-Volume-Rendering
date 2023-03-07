//
// Created by u137524 on 13/12/2022.
//

#include "application.h"
#include "raw_meshes.h"
#include "asset_locator.h"

void ApplicationLogic::config_render_pipeline(Render::sInstance &renderer) {
    // Load cube mesh
    const uint8_t cube_mesh = renderer.get_new_mesh_id();
    renderer.meshes[cube_mesh].init_with_triangles(RawMesh::cube_geometry,
                                                   sizeof(RawMesh::cube_geometry),
                                                   RawMesh::cube_indices,
                                                   sizeof(RawMesh::cube_indices));

    const uint8_t quad_mesh = renderer.get_new_mesh_id();
    renderer.meshes[quad_mesh].init_with_triangles(RawMesh::quad_geometry,
                                                   sizeof(RawMesh::quad_geometry),
                                                   RawMesh::quad_indices,
                                                   sizeof(RawMesh::quad_indices));

    // Create shaders
    /*const uint8_t volume_shader = renderer.material_man.add_raw_shader(RawShaders::basic_vertex,
                                                                       RawShaders::mar_shader);*/

    const uint8_t billboard_shader = renderer.material_man.add_raw_shader(RawShaders::bill_vertex,
                                                                       RawShaders::bill_fragment);
    //const uint8_t plaincolor_shader = renderer.material_man.add_raw_shader(RawShaders::basic_vertex,
    //                                                                        RawShaders::basic_fragment);

    // Load textures async (TODO)
    // For now, just load sync
    char *volume_tex_dir = NULL;
    Assets::get_asset_dir("assets/bonsai_256x256x256_uint8.raw",
                          &volume_tex_dir);
    const uint8_t volume_texture = renderer.material_man.add_volume_texture(volume_tex_dir,
                                                                            256,
                                                                            256,
                                                                            256);
    free(volume_tex_dir);

    // Load the blue noise texutre
    char *blue_noise_tex_dir = NULL;
    Assets::get_asset_dir("assets/blueNoise.png",
                          &blue_noise_tex_dir);

    const uint8_t blue_noise_texture = renderer.material_man.add_texture(blue_noise_tex_dir);

    // Create materials
    /*const uint8_t volumetric_material = renderer.material_man.add_material(volume_shader,
                                                                           {
                                                                                .color_tex = blue_noise_texture,
                                                                                .volume_tex = volume_texture,
                                                                                .enabled_color = true,
                                                                                .enabled_volume = true
                                                                           });*/
    const uint8_t billoard_material = renderer.material_man.add_material(billboard_shader,
                                                                           {
                                                                                   .color_tex = blue_noise_texture,
                                                                                   .volume_tex = volume_texture,
                                                                                   .enabled_color = true,
                                                                                   .enabled_volume = true
                                                                           });


    // Create the render pipeline
    const uint8_t render_pass = renderer.add_render_pass(Render::SCREEN_TARGET,
                                                         0);

    // Set clear color
    renderer.render_passes[render_pass].rgba_clear_values[0] = 0.0f;
    renderer.render_passes[render_pass].rgba_clear_values[1] = 0.0f;
    renderer.render_passes[render_pass].rgba_clear_values[2] = 1.0f;
    renderer.render_passes[render_pass].rgba_clear_values[3] = 1.0f;

    float movement_delta = 0.50f / (256.0f /2.0f);// size of a voxel in world space
    glm::vec3 starting_pos = {0.0f, 0.5f + (movement_delta * 256/2), 0.0f};
    for(uint32_t i = 0; i < 256/2; i++) {
        renderer.add_drawcall_to_pass(render_pass,
                                      {.mesh_id = quad_mesh,
                                              .material_id = billoard_material,
                                              .use_transform = true,
                                              .transform = {
                                                      .position = starting_pos,
                                                      .scale = {0.250f, 0.250f, 0.250f},
                                                      .rotation = glm::angleAxis(glm::radians(90.0f), glm::vec3{-1.0f, 0.0f, 0.0f})
                                              },
                                              .call_state = {
                                                      .depth_test_enabled = true,
                                                      .write_to_depth_buffer = true,
                                                      .culling_enabled = false,
                                                      .culling_mode = GL_FRONT,
                                                      .blending_enabled = true,
                                                      .blend_func_x = GL_SRC_ALPHA,
                                                      .blend_func_y = GL_ONE_MINUS_SRC_ALPHA
                                              },
                                              .enabled = true });
        //models_axis_y[i] = glm::rotate(glm::translate(glm::mat4x4(1.0f), starting_pos), );
        starting_pos.y -= movement_delta;
    }
    //__android_log_print(ANDROID_LOG_VERBOSE, "testio", "rendere %i", renderer.render_passes[render_pass].draw_stack_size);
}

void ApplicationLogic::update_logic(const double delta_time,
                  const sFrameTransforms &frame_transforms) {
    // TODO add controller movement andinteraction logic

}