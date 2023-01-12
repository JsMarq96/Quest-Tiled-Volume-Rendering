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

    // Create shaders
    const uint8_t volume_shader = renderer.material_man.add_raw_shader(RawShaders::basic_vertex,
                                                                       RawShaders::volumetric_fragment_outside);
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

    // Create materials
    const uint8_t volumetric_material = renderer.material_man.add_material(volume_shader,
                                                                           {  .volume_tex = volume_texture,
                                                                              .enabled_volume = true });


    // Create the render pipeline
    const uint8_t render_pass = renderer.add_render_pass(Render::SCREEN_TARGET,
                                                         0);

    // Set clear color
    renderer.render_passes[render_pass].rgba_clear_values[0] = 0.0f;
    renderer.render_passes[render_pass].rgba_clear_values[1] = 0.0f;
    renderer.render_passes[render_pass].rgba_clear_values[2] = 1.0f;
    renderer.render_passes[render_pass].rgba_clear_values[3] = 1.0f;

    renderer.add_drawcall_to_pass(render_pass,
                                  {.mesh_id = cube_mesh,
                                    .material_id = volumetric_material,
                                    .use_transform = true,
                                    .transform = {
                                          .position = {0.5f, 1.0f, -0.50f},
                                          .scale = {0.50f, 0.50f, 0.50f}
                                    },
                                    .call_state = {
                                          .depth_test_enabled = false,
                                          .write_to_depth_buffer = true,
                                          .culling_enabled = true,
                                          .culling_mode = GL_FRONT
                                    },
                                    .enabled = true });
}

void ApplicationLogic::update_logic(const double delta_time,
                  const sFrameTransforms &frame_transforms) {
    // TODO add controller movement andinteraction logic

}