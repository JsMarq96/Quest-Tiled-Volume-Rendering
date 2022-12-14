//
// Created by js on 7/11/22.
//

#ifndef OCULUSROOT_APP_DATA_H
#define OCULUSROOT_APP_DATA_H

#define APP_NAME "Understanding_volume_render"
#define ENGINE_NAME "Volatile"
#define APP_VERSION 1
#define ENGINE_VERSION 1

// CONSTANTS =====
#define MAX_EYE_NUMBER 2
#define LEFT_EYE 1
#define RIGHT_EYE 0

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

#endif //OCULUSROOT_APP_DATA_H
