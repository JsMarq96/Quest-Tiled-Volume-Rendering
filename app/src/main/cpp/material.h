#ifndef MATERIAL_H_
#define MATERIAL_H_

#include <cstddef>
#include <cstdint>
#include "texture.h"
#include "shader.h"
#include "fbo.h"
#include "volumetric_octree.h"

#define MAX_TEXTURE_COUNT 15
#define MAX_SHADER_COUNT 15
#define MAX_MATERIAL_COUNT 15
#define TEXTURE_SIZE 3

enum eTextureMapType : int {
    COLOR_MAP = 0,
    NORMAL_MAP,
    METALLIC_ROUGHNESS_MAP,
    VOLUME_MAP,
    COLOR_ATTACHMENT0,
    COLOR_ATTACHMENT1,
    TEXTURE_MAP_TYPE_COUNT
};

const char texture_uniform_LUT[TEXTURE_MAP_TYPE_COUNT][26] = {
   "u_albedo_map",
   "u_normal_map",
   "u_metallic_rough_map",
   "u_volume_map",
   "u_frame_color_attachment0",
   "u_frame_color_attachment1"
};

 struct sMaterialTexConstructor {
     union {
         uint8_t texture_ids[TEXTURE_MAP_TYPE_COUNT];
         struct {
             uint8_t color_tex = 0;
             uint8_t normal_tex = 0;
             uint8_t metallic_rough_tex = 0;
             uint8_t volume_tex = 0;
             uint8_t color_attach_tex0 = 0;
             uint8_t color_attach_tex1 = 0;
         };
     };

     union {
        bool            enabled_textures[TEXTURE_MAP_TYPE_COUNT];
        struct {
            bool enabled_color = false;
            bool enabled_normal = false;
            bool enabled_metallic_rough = false;
            bool enabled_volume = false;
            bool enabled_color_attach0 = false;
            bool enabled_color_attach1 = false;
        };
    };
};

struct sMaterialInstance {
    uint8_t shader_id;
    uint8_t texture_ids[TEXTURE_MAP_TYPE_COUNT];
    bool    enabled_textures[TEXTURE_MAP_TYPE_COUNT];
};

struct sMaterialManager {
    // For isosurface rendering; a global setting.. ?
    float density_threshold = 0.005f;

    sTexture        textures[MAX_TEXTURE_COUNT];
    uint8_t         texture_count = 0;
    sShader         shaders[MAX_TEXTURE_COUNT];
    uint8_t         shader_count = 0;

    sMaterialInstance  materials[MAX_MATERIAL_COUNT];
    uint8_t            materials_count = 0;

    uint8_t add_shader(const char     *vertex_shader,
                    const char     *fragment_shader);
    uint8_t add_raw_shader(const char     *vertex_shader,
                    const char     *fragment_shader);

    void add_volume_texture(const char* text_dir,
                            const uint16_t tile_width,
                            const uint16_t tile_heigth,
                            const uint16_t tile_depth);

    uint8_t load_async_texture3D(const char* dir,
                              const uint16_t width,
                              const uint16_t heigth,
                              const uint16_t depth);

    uint8_t add_texture(const char*          text_dir);

    void add_raw_texture(const char* raw_data,
                         const size_t width,
                         const size_t height,
                         const GLenum format,
                         const GLenum type,
                         const eTextureType text_type);

    void add_cubemap_texture(const char  *text_dir);

    void add_color_attachment_from_fbo(const uint8_t material_id,
                                       const sFBO &fbo,
                                       const int attachment_id) {
        materials[material_id].enabled_textures[COLOR_ATTACHMENT0 + attachment_id] = true;
        //textures[COLOR_ATTACHMENT] = fbo.color_attachment;
    }

    inline uint8_t get_new_texture() {
        return texture_count++;
    }

    inline uint8_t add_material(const uint8_t shader_id,
                                const sMaterialTexConstructor &mat_construct) {
        memcpy(materials[materials_count].texture_ids, mat_construct.texture_ids, sizeof(sMaterialInstance::texture_ids));
        memcpy(materials[materials_count].enabled_textures, mat_construct.enabled_textures, sizeof(sMaterialInstance::enabled_textures));
        materials[materials_count].shader_id = shader_id;

        return materials_count++;
    }


    /**
    * Binds the textures on Opengl
    *  COLOR - Texture 0
    *  NORMAL - Texture 1
    *  SPECULAR - TEXTURE 2
    * */
    void enable(const uint8_t material_id) const;

    void disable() const;
};

#endif // MATERIAL_H_
