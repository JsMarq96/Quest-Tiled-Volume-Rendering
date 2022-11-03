#ifndef APPLICATION_H_
#define APPLICATION_H_

#include <cstdint>
#include <glm/glm.hpp>
#include "transform.h"

#define TRANSFORMS_TOTAL_COUNT 20


namespace Application {
    enum eControllers : uint8_t {
        LEFT_HAND = 0,
        RIGHT_HAND,
        TOTAL_CONTROLLER_COUNT
    };

    struct sInstance {
        // CONTROLLER STATE ============
        bool enabled_controllers[TOTAL_CONTROLLER_COUNT];
        glm::vec3 controller_position[TOTAL_CONTROLLER_COUNT];
        glm::quat controller_rotation[TOTAL_CONTROLLER_COUNT];
        // TODO add controller key state

        // APPLICATION STATE ==========
        uint8_t first_render_pass_id = 0;
        uint8_t final_render_pass_id = 0;
        uint8_t volumetric_drawcall_id = 0;
        uint8_t left_controller_drawcall_id = 0;
        uint8_t right_controller_drawcall_id = 0;
        uint8_t controller_drawcalls[TOTAL_CONTROLLER_COUNT] = {};

        uint8_t volume_material_inside = 0;
        uint8_t volume_material_outside = 0;


        void get_current_state();
    };
}

#endif // APPLICATION_H_
