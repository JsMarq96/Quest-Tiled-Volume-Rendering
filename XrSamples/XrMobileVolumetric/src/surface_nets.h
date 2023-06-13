#ifndef SURF_NETS_
#define SURF_NETS_
#include <cstdint>
#include <GLES3/gl3.h>

#include "texture.h"


namespace SurfaceNets {
    struct sSurfacesPoint {
        float is_surface;
        glm::vec3 position;
    };

    struct sGenerator {
            uint32_t index_count = 0;

            uint32_t ssbos[4] = {0, 0, 0, 0};

            sShader  mesh_vertex_finder = {};
            sShader  mesh_vertex_generator = {};
            sShader  mesh_index_generator = {};
            sShader  mesh_index_voxelizator = {};

            void generate_from_volume(const sTexture &volume_texture,
                                      const uint32_t sampling_rate,
                                      sMeshBuffers *mesh) {
    #ifdef _WIN32
                mesh_vertex_finder.load_file_compute_shader("..\\resources\\shaders\\surface_find.cs");
                mesh_vertex_generator.load_file_compute_shader("..\\resources\\shaders\\surface_triangulize_to_list.cs");
                mesh_index_generator.load_file_compute_shader("..\\resources\\shaders\\surface_indices_to_mesh.cs");
                mesh_index_voxelizator.load_file_compute_shader("..\\resource\\shaders\\surface_to_boxes.cs");
    #else
                mesh_vertex_finder.load_file_compute_shader(get_path("../resources/shaders/surface_find.cs"));
                mesh_vertex_generator.load_file_compute_shader(get_path("../resources/shaders/surface_triangulize_to_list.cs"));
                mesh_index_generator.load_file_compute_shader(get_path("../resources/shaders/surface_indices_to_mesh.cs"));
                mesh_index_voxelizator.load_file_compute_shader(get_path("../resources/shaders/surface_to_boxes.cs"));
    #endif


                uint32_t max_vertex_count = sampling_rate * sampling_rate * sampling_rate * 36;
                size_t vertices_byte_size = sizeof(sSurfacesPoint) * max_vertex_count;
                size_t mesh_byte_size = sizeof(glm::vec3) * 3 * max_vertex_count;
                size_t index_byte_size = sizeof(uint32_t) * max_vertex_count;

                // Generate the SSBOs & allocate
                {
                    glGenBuffers(4, ssbos);
                    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbos[0]);
                    glBufferData(GL_SHADER_STORAGE_BUFFER, vertices_byte_size, NULL, GL_DYNAMIC_COPY);

                    // Allocate memory for the second SSBO
                    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbos[1]);
                    glBufferData(GL_SHADER_STORAGE_BUFFER, mesh_byte_size + sizeof(uint32_t), NULL, GL_DYNAMIC_COPY);

                    // Allocate memory for the index SSBO
                    uint32_t empty = 0;
                    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbos[2]);
                    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(uint32_t), &empty, GL_DYNAMIC_COPY);

                    // Allocate memory for the index buffer
                    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbos[3]);
                    glBufferData(GL_SHADER_STORAGE_BUFFER, index_byte_size, NULL, GL_DYNAMIC_COPY);
                }

                // FIRST PASS: Detect the surface
                {
                    mesh_vertex_finder.activate();

                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_3D, volume_texture.texture_id);
                    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssbos[0]);

                    mesh_vertex_finder.set_uniform_texture("u_volume_map", 0);
                    mesh_vertex_finder.dispatch(sampling_rate,
                                                sampling_rate,
                                                sampling_rate,
                                                true);
                    mesh_vertex_finder.deactivate();
                }

                // SECOND: Triangulate
                {
                    mesh_index_voxelizator.activate();

                    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssbos[0]);
                    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssbos[1]);
                    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, ssbos[3]);
                    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, ssbos[2]);

                    mesh_index_voxelizator.dispatch(sampling_rate,
                                                sampling_rate,
                                                sampling_rate,
                                                true);
                    mesh_index_voxelizator.deactivate();
                }

                glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbos[2]);
                glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(uint32_t), &index_count);
                std::cout << index_count << " indices count" << std::endl;

                mesh->is_indexed = false;
                mesh->primitige = GL_TRIANGLE;
                mesh->primitive_count = index_count;
                mesh->VBO = ssbos[1];

                // THIRD: Inflate
                /*{
                    mesh_index_inflator.activate();

                    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssbos[0]);
                    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, ssbos[3]);

                    mesh_index_inflator.dispatch(index_count,
                                                1,
                                                1,
                                                true);
                    mesh_index_inflator.deactivate();
                }*/

                // Forth: To Mesh
                /*{
                    mesh_index_voxelizator.activate();
                    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssbos[0]);
                    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssbos[1]);
                    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, ssbos[2]);
                    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, ssbos[3]);
                    mesh_index_voxelizator.dispatch(index_count,
                                                1,
                                                1,
                                                true);
                    mesh_index_voxelizator.deactivate();
                }*/

                //renderer->config_from_buffers(ssbos[1], index_count * 3);
            }
        };

};

#endif // SURF_NETS_
