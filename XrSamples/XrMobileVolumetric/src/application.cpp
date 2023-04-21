//
// Created by u137524 on 13/12/2022.
//

#include "application.h"
#include "raw_meshes.h"
#include "asset_locator.h"
#include "surface_nets.h"

void ApplicationLogic::initial_config_render_pipeline(Render::sInstance &renderer) {
    // Create the render pipeline
    render_pass = renderer.add_render_pass(Render::SCREEN_TARGET,
                                                         0);
    // Load cube mesh
    const uint8_t cube_mesh = renderer.get_new_mesh_id();
    renderer.meshes[cube_mesh].init_with_triangles(RawMesh::cube_geometry,
                                                   sizeof(RawMesh::cube_geometry),
                                                   RawMesh::cube_indices,
                                                   sizeof(RawMesh::cube_indices));

    const uint8_t plain_shader = renderer.material_man.add_raw_shader(RawShaders::basic_vertex,
                                                                      RawShaders::basic_fragment);

    // Create materials
    const uint8_t simple_material = renderer.material_man.add_material(plain_shader,
                                                                       {
                                                                               .enabled_color = false,
                                                                               .enabled_volume = false
                                                                       });

    cube_pass = renderer.add_drawcall_to_pass(render_pass,
                                      {
                                              .mesh_id = cube_mesh,
                                              .material_id = simple_material,
                                              .use_transform = true,
                                              .transform = {
                                                      .position = {-0.250f, 0.250000, -0.250f},
                                                      .scale = {0.50f, 0.50f, 0.50f}
                                              },
                                              .call_state = {
                                                      .depth_test_enabled = true,
                                                      .write_to_depth_buffer = true,
                                                      .culling_enabled = false,
                                              },
                                              .enabled = true });
}

void ApplicationLogic::config_render_pipeline(Render::sInstance &renderer) {
    SurfaceNets::sGenerator mesh_generator = {};

    // Create shaders
    const uint8_t color_shader = renderer.material_man.add_raw_shader(RawShaders::basic_vertex,
                                                                       RawShaders::world_fragment_shader);
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


    // Fill a new MeshBuffer from the generator data
    uint8_t bonsai_mesh = renderer.get_new_mesh_id();

    // Generate the surface from the volume
    mesh_generator.generate_from_volume(renderer.material_man.textures[volume_texture],
                                        200,
                                        &renderer.meshes[bonsai_mesh]);

    // Create materials
    const uint8_t simple_material = renderer.material_man.add_material(color_shader,
                                                                           {
                                                                                .enabled_color = false,
                                                                                .enabled_volume = false
                                                                           });

    // Set clear color
    renderer.render_passes[render_pass].rgba_clear_values[0] = 0.0f;
    renderer.render_passes[render_pass].rgba_clear_values[1] = 0.0f;
    renderer.render_passes[render_pass].rgba_clear_values[2] = 1.0f;
    renderer.render_passes[render_pass].rgba_clear_values[3] = 1.0f;

    glm::vec3 starting_pos = {-0.250f, 0.250000, -0.250f};

    renderer.add_drawcall_to_pass(render_pass,
                                  {
                                    .mesh_id = bonsai_mesh,
                                    .material_id = simple_material,
                                    .use_transform = true,
                                    .transform = {
                                          .position = starting_pos,
                                          .scale = {0.50f, 0.50f, 0.50f}
                                    },
                                    .call_state = {
                                          .depth_test_enabled = true,
                                          .write_to_depth_buffer = true,
                                          .culling_enabled = false,
                                    },
                                    .enabled = true });
    // Disable teh prev drawcall
    renderer.render_passes[render_pass].draw_stack[cube_pass].enabled = false;
}

void ApplicationLogic::update_logic(const double delta_time,
                  const sFrameTransforms &frame_transforms) {
    // TODO add controller movement andinteraction logic

}