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
#include <glm/gtc/type_ptr.hpp>

#include "render.h"
#include "application.h"

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

void render_pipeline_config(const sFrameTransforms &transforms);

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

    app_state.main_thread = gettid();

    // Init renderer with the framebuffer data from OpenXR
    renderer.init(framebuffer);

     // Composite layer
     const XrCompositionLayerBaseHeader *layers = {
             (const XrCompositionLayerBaseHeader* const) &openxr_instance.projection_layer
     };

     XrFrameEndInfo frame_end_info = {
             .type = XR_TYPE_FRAME_END_INFO,
             .next = NULL,
             .displayTime = openxr_instance.frame_state.predictedDisplayTime,
             .environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE,
             .layerCount = 1,
             .layers = &layers,
     };

     sFrameTransforms frame_transforms = {};

     // Config the render pipeline
     // One render pass for each eye :(
     const uint8_t left_eye_pass = renderer.add_render_pass(Render::FBO_TARGET,
                                                            renderer.framebuffer.fbos[0],
                                                            glm::mat4(),
                                                            glm::mat4());

     const uint8_t right_eye_pass = renderer.add_render_pass(Render::FBO_TARGET,
                                                             renderer.framebuffer.fbos[1],
                                                             glm::mat4(),
                                                             glm::mat4());

     // Set a different clear pass color to each eye
     renderer.render_passes[left_eye_pass].rgba_clear_values[0] = 1.0f;
     renderer.render_passes[left_eye_pass].rgba_clear_values[1] = 1.0f;
     renderer.render_passes[left_eye_pass].rgba_clear_values[2] = 1.0f;
     renderer.render_passes[left_eye_pass].rgba_clear_values[3] = 1.0f;

     renderer.render_passes[right_eye_pass].rgba_clear_values[0] = 0.0f;
     renderer.render_passes[right_eye_pass].rgba_clear_values[1] = 0.0f;
     renderer.render_passes[right_eye_pass].rgba_clear_values[2] = 0.0f;
     renderer.render_passes[right_eye_pass].rgba_clear_values[3] = 1.0f;

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

        openxr_instance.update(&app_state,
                               &delta_time,
                               &frame_transforms);

        // Non-runtine Update

        // Render
        renderer.render_passes[left_eye_pass].view_mat = glm::make_mat4(frame_transforms.view[0].m);
        renderer.render_passes[left_eye_pass].projection_mat = glm::make_mat4(frame_transforms.projection[0].m);
        renderer.render_passes[right_eye_pass].view_mat = glm::make_mat4(frame_transforms.view[1].m);
        renderer.render_passes[right_eye_pass].projection_mat = glm::make_mat4(frame_transforms.projection[1].m);

        renderer.render_frame(true);

        frame_end_info.displayTime = openxr_instance.frame_state.predictedDisplayTime;

        //assert(xrEndFrame(openxr_instance.xr_session,
        //                  &frame_end_info) == XR_SUCCESS && "Error submiting frame");
        int code = xrEndFrame(openxr_instance.xr_session,
                              &frame_end_info);
        code +=1, code -=1;
    }



    // Cleanup TODO

    (*app->activity->vm).DetachCurrentThread();
}



void render_pipeline_config(const sFrameTransforms &transforms) {
    // One render pass for each eye :(
    const uint8_t left_eye_pass = renderer.add_render_pass(Render::FBO_TARGET,
                                                           renderer.framebuffer.fbos[0],
                                                           glm::make_mat4(transforms.view[0].m),
                                                           glm::make_mat4(transforms.view[0].m));

    const uint8_t right_eye_pass = renderer.add_render_pass(Render::FBO_TARGET,
                                                           renderer.framebuffer.fbos[1],
                                                           glm::make_mat4(transforms.view[1].m),
                                                           glm::make_mat4(transforms.view[1].m));

    renderer.render_passes[left_eye_pass].rgba_clear_values[0] = 1.0f;
    renderer.render_passes[left_eye_pass].rgba_clear_values[1] = 1.0f;
    renderer.render_passes[left_eye_pass].rgba_clear_values[2] = 1.0f;
    renderer.render_passes[left_eye_pass].rgba_clear_values[3] = 1.0f;

    renderer.render_passes[right_eye_pass].rgba_clear_values[0] = 0.0f;
    renderer.render_passes[right_eye_pass].rgba_clear_values[1] = 0.0f;
    renderer.render_passes[right_eye_pass].rgba_clear_values[2] = 0.0f;
    renderer.render_passes[right_eye_pass].rgba_clear_values[3] = 1.0f;
}