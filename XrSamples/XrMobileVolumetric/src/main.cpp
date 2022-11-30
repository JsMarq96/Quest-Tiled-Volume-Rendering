/************************************************************************************

Filename  : XrCompositor_NativeActivity.c
Content   : This sample uses the Android NativeActivity class.
Created   :
Authors   :

Copyright : Copyright (c) Facebook Technologies, LLC and its affiliates. All rights reserved.

*************************************************************************************/

#include "openxr_instance.h"

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

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>

#include "render.h"

struct sAndroidState {
    ANativeWindow *native_window = NULL;
    bool resumed = false;

    void *application_vm = NULL;
    void *application_activity = NULL;

};

static void app_handle_cmd(struct android_app* app, int32_t cmd) {
    sAndroidState *app_state = (sAndroidState*) app;

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

void render_pipeline_config();

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

    sAndroidState app_state = {};

    app->userData = &app_state;
    app->onAppCmd = app_handle_cmd;

    app_state.application_vm = app->activity->vm;
    app_state.application_activity = app->activity->clazz; // ??

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

    sOpenXRFramebuffer framebuffer;
    openxr_instance.init(&framebuffer);

    // Init renderer with the framebuffer data from OpenXR
    renderer.init(framebuffer);

     // Composite layer, dont need them (right?)
     const XrCompositionLayerBaseHeader *layers = {
             (const XrCompositionLayerBaseHeader* const) &openxr_instance.projection_layer
     };

     XrFrameEndInfo frameEndInfo = {
             .type = XR_TYPE_FRAME_END_INFO,
             .next = NULL,
             .displayTime = openxr_instance.frame_state.predictedDisplayTime,
             .environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE,
             .layerCount = 1,
             .layers = &layers,
     };

     sFrameTransforms frame_transforms = {};

    // Game Loop
    while (app->destroyRequested == 0) {
        // Read all pending android events
        for(;;) {
            int events = 0;
            struct android_poll_source* source = NULL;
            // Check for system events with the androind_native_app_glue, based on the
            // state of the app
            if (ALooper_pollAll(app->destroyRequested  ? 0 : -1,
                                NULL,
                                &events,
                                (void**)&source) < 0) {
                break;
            }

            // Process the detected event
            if (source != NULL) {
                source->process(app,
                                source);
            }

        }
        double delta_time = 0.0;
        // Update and get position & events from the OpenXR runtime

        openxr_instance.update(&delta_time,
                               &frame_transforms);

        // Non-runtine Update

        // Render


        frameEndInfo.displayTime = openxr_instance.frame_state.predictedDisplayTime;

        xrEndFrame(openxr_instance.xr_session,
                   &frameEndInfo);
    }



    // Cleanup TODO

    (*app->activity->vm).DetachCurrentThread();
}



void render_pipeline_config() {

}