#ifndef APPLICATION_H_
#define APPLICATION_H_

#include <cstdint>

#include "openxr_instance.h"
#include "render.h"

namespace ApplicationLogic {

    static uint8_t render_pass = 0;
    static uint8_t cube_pass = 0;
    void initial_config_render_pipeline(Render::sInstance &renderer);
    void config_render_pipeline(Render::sInstance &renderer);

    void update_logic(const double delta_time,
                      const sFrameTransforms &frame_transforms);
};


#endif // APPLICATION_H_
