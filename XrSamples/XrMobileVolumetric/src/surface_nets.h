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
        union {
            uint32_t ssbos[4] = {0,0,0};
            struct {
                uint32_t mesh_area_SSBO;
                uint32_t mesh_vertices_SSBO;
                uint32_t vertex_counter_SSBO;
                uint32_t tmp;
            };
        };
        uint32_t vertices_count = 0;

        sShader  mesh_vertex_finder = {};
        sShader  mesh_vertex_generator = {};
        sShader  mesh_index_generator = {};
        sShader  mesh_index_voxelizator = {};

        void free() {
            glDeleteBuffers(3, ssbos);
        }

            void generate_from_volume(const sTexture &volume_texture,
                                      const uint32_t sampling_rate,
                                      Render::sMeshBuffers *new_mesh) {

                mesh_vertex_finder.load_compute_shader(RawShaders::surface_find_ess);
                mesh_index_voxelizator.load_compute_shader(RawShaders::surface_to_boxes);


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

                // SECOND: Triangulate OR bOXELIZE
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


                glBindBuffer(GL_SHADER_STORAGE_BUFFER, vertex_counter_SSBO);
                uint32_t *raw_vert_count = (uint32_t *) glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, sizeof(uint32_t), GL_MAP_READ_BIT);
                vertices_count = *raw_vert_count;
                glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

                // Generate the Mesh buffer structure, with the volume's mesh data
                new_mesh->VBO = mesh_vertices_SSBO;
                new_mesh->is_indexed = false;
                new_mesh->primitive = GL_TRIANGLES;
                new_mesh->primitive_count = vertices_count;
                glGenBuffers(1, &new_mesh->VAO);
                glBindVertexArray(new_mesh->VAO);
                glBindBuffer(GL_ARRAY_BUFFER, new_mesh->VBO);

                // Vertex position
                glEnableVertexAttribArray(0);
                glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*) 0);

                glBindVertexArray(0);
            }
    };

};

#endif // SURF_NETS_
