#include "texture.h"
#ifndef __EMSCRIPTEN__
#include <GL/gl3w.h>
#else
#include <emscripten.h>
#include <webgl/webgl2.h>
#include <iostream>
#include <GLES3/gl3.h>
#endif

#include <cstdlib>

void upload_simple_texture_to_GPU(sTexture *text);


void sTexture::load(const eTextureType text_type,
                    const bool istore_on_RAM,
                    const char *texture_name) {
    store_on_RAM = istore_on_RAM;
    type = text_type;

    if (text_type == CUBEMAP) {
        const char *cubemap_terminations[] = {"right.jpg",
                                        "left.jpg",
                                        "top.jpg",
                                        "bottom.jpg",
                                        "front.jpg",
                                        "back.jpg" };

        glGenTextures(1, &texture_id);
        glBindTexture(GL_TEXTURE_CUBE_MAP, texture_id);

        // Generate buffer based on teh biggest possible size, the bottom terminating png
        char *name_buffer = (char*) malloc(strlen(texture_name) + sizeof("bottom.png") + 1);

        char* raw_data = NULL;
        for(int i = 0; i < 6; i++) {
            int w, h, l;
            memset(name_buffer, '\0', strlen(texture_name) + sizeof("bottom.png") + 1);
            strcat(name_buffer, texture_name);
            strcat(name_buffer, cubemap_terminations[i]);

#ifndef __EMSCRIPTEN__
            //raw_data = stbi_load(name_buffer, &w, &h, &l, 0);
#else
            raw_data = emscripten_get_preloaded_image_data(name_buffer, &w, &h);
            l = 4;
#endif
            //info(" Load cubemap: %s %i %i %i", name_buffer, w, h, l);

            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                         0,
                         GL_RGB,
                         w,
                         h,
                         0,
                         GL_RGB,
                         GL_UNSIGNED_BYTE,
                         raw_data);
            //stbi_image_free(raw_data);
        }

        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glBindTexture(GL_TEXTURE_2D, 0);

        free(name_buffer);

        return;
    }
    int w, h, l;
    //text->raw_data = stbi_load(texture_name, &w, &h, &l, 0);

#ifndef __EMSCRIPTEN__
    //raw_data = stbi_load(name_buffer, &w, &h, &l, 0);
#else
    raw_data = emscripten_get_preloaded_image_data(texture_name, &w, &h);
    l = 4;
#endif

    width = w;
    height = h;

    assert(raw_data != NULL && "Uploading empty texture to GPU");

    glGenTextures(1, &texture_id);

    uint32_t texture_type = (text_type == VOLUME) ? GL_TEXTURE_3D : GL_TEXTURE_2D;

    glBindTexture(texture_type, texture_id);

    glTexParameteri(texture_type, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(texture_type, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(texture_type, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(texture_type, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    if (text_type == STANDART_2D) {
        glTexImage2D(GL_TEXTURE_2D,
                     0,
                     GL_RGBA,
                     w,
                     h,
                     0,
                     GL_RGBA,
                     GL_UNSIGNED_BYTE,
                     raw_data);
        glGenerateMipmap(GL_TEXTURE_2D);
    } else {
        glTexParameteri(texture_type, GL_TEXTURE_WRAP_R, GL_REPEAT);

        glTexImage3D(GL_TEXTURE_3D,
                     0,
                     GL_RGBA,
                     w,
                     h,
                     0,
                     0,
                     GL_RGBA,
                     GL_UNSIGNED_BYTE,
                     raw_data);
    }

    glBindTexture(texture_type, 0);

    //stbi_image_free(text->raw_data);
}


void sTexture::load3D(const char* texture_name,
                      const uint16_t width_i,
                      const uint16_t heigth_i,
                      const uint16_t depth_i) {
    store_on_RAM = false;
    type = VOLUME;
    int w = 0, h = 0, l = 0;
    //text->raw_data = stbi_load(texture_name, &w, &h, &l, 0);

#ifndef __EMSCRIPTEN__
    //raw_data = stbi_load(name_buffer, &w, &h, &l, 0);
#else
    raw_data = emscripten_get_preloaded_image_data(texture_name, &w, &h);
    l = 4;
#endif

    width = w;
    height = h;

    assert(raw_data != NULL && "Uploading empty texture to GPU");

    glGenTextures(1, &texture_id);


    glBindTexture(GL_TEXTURE_3D, texture_id);

    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);

    glTexImage3D(GL_TEXTURE_3D,
                 0,
                 GL_RGBA,
                 width / width_i,
                 height / heigth_i,
                 width / width_i,
                 0,
                 GL_RGBA,
                 GL_UNSIGNED_BYTE,
                 raw_data);

    glBindTexture(GL_TEXTURE_3D, 0);

    //stbi_image_free(text->raw_data);
}

void sTexture::create_empty2D_with_size(const uint32_t w,
                                      const uint32_t h) {
    glGenTextures(1, &texture_id);
    glBindTexture(GL_TEXTURE_2D, texture_id);

    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_RGBA32F,
                 w, h,
                 0,
                 GL_RGBA,
                 GL_FLOAT,
                 NULL);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_2D, 0);
}

void sTexture::load_sphere_volume(const uint16_t size) {
        //free(raw_data);
}

void sTexture::clean() {
    if (store_on_RAM) {
        //stbi_image_free(text->raw_data);
    }

    glDeleteTextures(1, &texture_id);
}

void sTexture::load_empty_volume() {
    // Load empty texture
    glGenTextures(1, &texture_id);

    glBindTexture(GL_TEXTURE_3D, texture_id);

    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    /*
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);*/

    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAX_LEVEL, 0);

    glBindTexture(GL_TEXTURE_3D, 0);
}

void sTexture::load_empty_2D() {
    // Load empty texture
    glGenTextures(1, &texture_id);

    glBindTexture(GL_TEXTURE_2D, texture_id);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glBindTexture(GL_TEXTURE_2D, 0);
};
