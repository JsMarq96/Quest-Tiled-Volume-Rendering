#ifndef TEXTURE_H_
#define TEXTURE_H_

#include <string.h>
#include <cassert>
#include <cstdint>

#define DEFAULT_TEXT_FIDELITY 0

enum eTextureType : uint8_t {
   STANDART_2D = 0,
   CUBEMAP,
   VOLUME
};

struct sTexture {
    bool             store_on_RAM = false;
    eTextureType     type;

    // Raw data
    int             width     = 0;
    int             height    = 0;
    int             depth     = 0;
    int             layers    = 0;
    char            *raw_data = NULL;

    // OpenGL id
    unsigned int     texture_id;

    void create_empty2D_with_size(const uint32_t width,
                                const uint32_t height);

    void load(const eTextureType text_type,
              const bool store_on_RAM,
              const char *texture_name);

    void load3D(const char *texture_name,
                const uint16_t width,
                const uint16_t heigth,
                const uint16_t depth);

    void load_sphere_volume(const uint16_t size);

    void load_empty_volume();

    void load_empty_2D();

    void clean();
};



#endif // TEXTURE_H_
