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

    struct sAndroidState {
        ANativeWindow *native_window = NULL;
        bool resumed = false;
        bool visible = false;
        bool focused = false;

        bool session_active = false;

        int main_thread = 0;
        int render_thread = 0;

        void *application_vm = NULL;
        void *application_activity = NULL;

    };
}

#endif // APPLICATION_H_
