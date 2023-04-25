/************************************************************************************

Filename  : XrCompositor_NativeActivity.c
Content   : This sample uses the Android NativeActivity class.
Created   :
Authors   :

Copyright : Copyright (c) Facebook Technologies, LLC and its affiliates. All rights reserved.

*************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/prctl.h>
#include <android/asset_manager_jni.h>
#include <android/asset_manager.h>
#include <android/log.h>
#include <android/native_window_jni.h>
#include <android_native_app_glue.h>
#include <assert.h>
#include <android/log.h>
#include <chrono>
#include <thread>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>
#include <glm/gtc/type_ptr.hpp>

#include "render.h"
#include "app_data.h"
#include "application.h"
#include "asset_locator.h"
#include "egl_context.h"
#include "openxr_instance.h"

PFNGLGENQUERIESEXTPROC glGenQueriesEXT_;
PFNGLDELETEQUERIESEXTPROC glDeleteQueriesEXT_;
PFNGLISQUERYEXTPROC glIsQueryEXT_;
PFNGLBEGINQUERYEXTPROC glBeginQueryEXT_;
PFNGLENDQUERYEXTPROC glEndQueryEXT_;
PFNGLQUERYCOUNTEREXTPROC glQueryCounterEXT_;
PFNGLGETQUERYIVEXTPROC glGetQueryivEXT_;
PFNGLGETQUERYOBJECTIVEXTPROC glGetQueryObjectivEXT_;
PFNGLGETQUERYOBJECTUIVEXTPROC glGetQueryObjectuivEXT_;
PFNGLGETQUERYOBJECTI64VEXTPROC glGetQueryObjecti64vEXT_;
PFNGLGETQUERYOBJECTUI64VEXTPROC glGetQueryObjectui64vEXT_;
PFNGLGETINTEGER64VPROC glGetInteger64v_;

static void app_handle_cmd(struct android_app* app, int32_t cmd) {
    Application::sAndroidState *app_state = (Application::sAndroidState*) app->userData;

    switch (cmd) {
        // There is no APP_CMD_CREATE. The ANativeActivity creates the
        // application thread from onCreate(). The application thread
        // then calls android_main().
        case APP_CMD_START: {
            //ALOGV("onStart()");
            //ALOGV("    APP_CMD_START");
            break;
        }
        case APP_CMD_RESUME: {
            //ALOGV("onResume()");
            //ALOGV("    APP_CMD_RESUME");
            app_state->resumed = true;
            break;
        }
        case APP_CMD_PAUSE: {
            //ALOGV("onPause()");
            //ALOGV("    APP_CMD_PAUSE");
            app_state->resumed = false;
            break;
        }
        case APP_CMD_STOP: {
            //ALOGV("onStop()");
            //ALOGV("    APP_CMD_STOP");
            break;
        }
        case APP_CMD_DESTROY: {
            //ALOGV("onDestroy()");
            //ALOGV("    APP_CMD_DESTROY");
            app_state->native_window = NULL;
            break;
        }
        case APP_CMD_INIT_WINDOW: {
            //ALOGV("surfaceCreated()");
            //ALOGV("    APP_CMD_INIT_WINDOW");
            app_state->native_window = app->window;
            break;
        }
        case APP_CMD_TERM_WINDOW: {
            //ALOGV("surfaceDestroyed()");
            //ALOGV("    APP_CMD_TERM_WINDOW");
            app_state->native_window = NULL;
            break;
        }
    }
}

/**
 *  OPENXR
 * */

sOpenXR_Instance openxr_instance = {};

Render::sInstance renderer = {};

/**
 * This is the main entry point of a native application that is using
 * android_native_app_glue.  It runs in its own thread, with its own
 * event loop for receiving input events and doing other things.
 */
// https://www.khronos.org/files/openxr-10-reference-guide.pdf
// https://github.com/QCraft-CC/OpenXR-Quest-sample/tree/main/app/src/main/cpp/hello_xr
// https://github.com/JsMarq96/Understanding-Tiled-Volume-Rendering/blob/ee294f2407da501274c6abd301fbfd8eec5575fc/XrSamples/XrMobileVolumetric/src/main.cpp
void android_main(struct android_app* app) {
    JNIEnv* Env;
    (*app->activity->vm).AttachCurrentThread( &Env, NULL);

    // Note that AttachCurrentThread will reset the thread name.
    prctl(PR_SET_NAME, (long)"VRMain", 0, 0, 0);

    Application::sAndroidState app_state = {};

    app->userData = &app_state;
    app->onAppCmd = app_handle_cmd;

    app_state.application_vm = app->activity->vm;
    app_state.application_activity = app->activity->clazz; // ??

    // Init the asset locator
    Assets::fetch_asset_locator()->init(Env,
                                        app->activity);

    PFN_xrInitializeLoaderKHR xrInitializeLoaderKHR;
    xrGetInstanceProcAddr(XR_NULL_HANDLE,
                          "xrInitializeLoaderKHR",
                          (PFN_xrVoidFunction*)&xrInitializeLoaderKHR);
    if (xrInitializeLoaderKHR != NULL) {
        XrLoaderInitInfoAndroidKHR loaderInitializeInfoAndroid;
        memset(&loaderInitializeInfoAndroid,
               0,
               sizeof(loaderInitializeInfoAndroid));
        loaderInitializeInfoAndroid.type = XR_TYPE_LOADER_INIT_INFO_ANDROID_KHR;
        loaderInitializeInfoAndroid.next = NULL;
        loaderInitializeInfoAndroid.applicationVM = app->activity->vm;
        loaderInitializeInfoAndroid.applicationContext = app->activity->clazz;

        xrInitializeLoaderKHR((XrLoaderInitInfoBaseHeaderKHR*)&loaderInitializeInfoAndroid);
    }
    

    sOpenXRFramebuffer framebuffers[2];
    openxr_instance.init(framebuffers);

    app_state.main_thread = gettid();

    // Init renderer with the framebuffer data from OpenXR
    renderer.init(framebuffers);

    sFrameTransforms frame_transforms = {};

    ApplicationLogic::config_render_pipeline(renderer);

    uint64_t render_time;
    uint32_t gl_time_queries[4];

#define TIME_RENDER 0

    glGenQueriesEXT_(2,
                    gl_time_queries);

    int frame_counter = 0;
    // Game Loop
    while (app->destroyRequested == 0) {
        // Read all pending android events
        for(;;) {
            int events = 0;
            struct android_poll_source* source;
            // If the timeout is zero, returns immediately without blocking.
            // If the timeout is negative, waits indefinitely until an event appears.
            const int timeoutMilliseconds =
                    (!app_state.resumed && !app_state.session_active && app->destroyRequested == 0) ? -1 : 0;
            if (ALooper_pollAll(timeoutMilliseconds, nullptr, &events, (void**)&source) < 0) {
                break;
            }

            // Process the detected event
            if (source != NULL) {
                source->process(app,
                                source);
            }

        }
        openxr_instance.handle_events(&app_state);

        //__android_log_print(ANDROID_LOG_VERBOSE, "Openxr test", "test frame %i %i", app_state.resumed, app_state.session_active);
        if (!app_state.session_active) {
            // Throttle loop since xrWaitFrame won't be called.
            std::this_thread::sleep_for(std::chrono::milliseconds(450));
            continue;
        }

        int disjoint_occurred = 0;
        // (Timing) Clear, if disjoint erro ocurred
        glGetIntegerv(GL_GPU_DISJOINT_EXT,
                      &disjoint_occurred);


        double delta_time = 0.0;
        __android_log_print(ANDROID_LOG_VERBOSE, "Openxr test", "starting frame");

        auto update_method_start = std::chrono::steady_clock::now();
        // Update and get position & events from the OpenXR runtime
        openxr_instance.update(&app_state,
                               &delta_time,
                               &frame_transforms);

        // Non-XR runtine Update
        ApplicationLogic::update_logic(delta_time,
                                       frame_transforms);
        auto update_method_end = std::chrono::steady_clock::now();
        double update_timing = std::chrono::duration_cast<std::chrono::nanoseconds>(update_method_end - update_method_start).count();

        // Render (& timing)
        glBeginQueryEXT_(GL_TIME_ELAPSED_EXT,
                         gl_time_queries[TIME_RENDER]);

        if (frame_counter > 2) {
            renderer.render_frame(true,
                                  frame_transforms.view,
                                  frame_transforms.projection,
                                  frame_transforms.viewprojection);
        } else {
            frame_counter++;
        }


        glEndQueryEXT_(GL_TIME_ELAPSED_EXT);

        openxr_instance.submit_frame();

        // Query processing ===========================================
        int available = 0;
        while (!available) {
            glGetQueryObjectivEXT_(gl_time_queries[TIME_RENDER], GL_QUERY_RESULT_AVAILABLE, &available);
        }


        glGetIntegerv(GL_GPU_DISJOINT_EXT,
                      &disjoint_occurred);
        if (!disjoint_occurred) {
            glGetQueryObjectui64vEXT_(gl_time_queries[TIME_RENDER], GL_QUERY_RESULT, &render_time);

            __android_log_print(ANDROID_LOG_VERBOSE,
                                "FRAME_STATS",
                                "Render time: %f; update time: %f",
                                ((double)render_time) / 1000000.0,
                                update_timing / 1000000.0);
        } else {
            __android_log_print(ANDROID_LOG_VERBOSE, "FRAME_STATS", "Render time: invalid");
        }

        __android_log_print(ANDROID_LOG_VERBOSE, "FRAME", "==================");
    }

    // Cleanup TODO

    (*app->activity->vm).DetachCurrentThread();
}