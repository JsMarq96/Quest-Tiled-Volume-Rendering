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
    prctl(PR_SET_NAME, (long)"OVR::Main", 0, 0, 0);

    sAndroidState app_state = {};

    app->userData = &app_state;
    app->onAppCmd = app_handle_cmd;

    app_state.application_vm = app->activity->vm;
    app_state.application_activity = app->activity->clazz; // ??

    sOpenXRFramebuffer framebuffer;
    openxr_instance.init(&framebuffer);

    // Init renderer with the framebuffer data from OpenXR

    app->userData = &app_state;
    app->onAppCmd = app_handle_cmd;

    // Game Loop
    while (app->destroyRequested == 0) {
        // Read all pending events.
        for (;;) {
            int events;
            struct android_poll_source* source;
            // If the timeout is zero, returns immediately without blocking.
            // If the timeout is negative, waits indefinitely until an event appears.
            const int timeoutMilliseconds =
                    (app_state.resumed == false && app->destroyRequested == 0)
                    ? -1
                    : 0;
            if (ALooper_pollAll(timeoutMilliseconds, NULL, &events, (void**)&source) < 0) {
                break;
            }

            // Process this event.
            if (source != NULL) {
                source->process(app, source);
            }
        }

        //ovrApp_HandleXrEvents(&appState);

        /*if (appState.SessionActive == false) {
            continue;
        }*/



        // NOTE: OpenXR does not use the concept of frame indices. Instead,
        // XrWaitFrame returns the predicted display time.
        /*XrFrameWaitInfo waitFrameInfo = {};
        waitFrameInfo.type = XR_TYPE_FRAME_WAIT_INFO;
        waitFrameInfo.next = NULL;

        XrFrameState frameState = {};
        frameState.type = XR_TYPE_FRAME_STATE;
        frameState.next = NULL;

        OXR(xrWaitFrame(appState.Session, &waitFrameInfo, &frameState));*/

        // Get the HMD pose, predicted for the middle of the time period during which
        // the new eye images will be displayed. The number of frames predicted ahead
        // depends on the pipeline depth of the engine and the synthesis rate.
        // The better the prediction, the less black will be pulled in at the edges.

        //



        // update input information (controller state)

        // OpenXR input
        {
        }

        // Set-up the compositor layers for this frame.
        // NOTE: Multiple independent layers are allowed, but they need to be added
        // in a depth consistent order.



        // Render the world-view layer (simple ground plane)


        // Build the cylinder layer


        // Build the quad layer


        // Compose the layers for this frame.
        /*const XrCompositionLayerBaseHeader* layers[ovrMaxLayerCount] = {};
        for (int i = 0; i < appState.LayerCount; i++) {
            layers[i] = (const XrCompositionLayerBaseHeader*)&appState.Layers[i];
        }

        XrFrameEndInfo endFrameInfo = {};
        endFrameInfo.type = XR_TYPE_FRAME_END_INFO;
        endFrameInfo.displayTime = frameState.predictedDisplayTime;
        endFrameInfo.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
        endFrameInfo.layerCount = appState.LayerCount;
        endFrameInfo.layers = layers;

        OXR(xrEndFrame(appState.Session, &endFrameInfo));*/
    }

    //ovrRenderer_Destroy(&appState.Renderer);


    //free(projections);

    /*ovrScene_Destroy(&appState.Scene);
    ovrEgl_DestroyContext(&appState.Egl);

    OXR(xrDestroySpace(appState.HeadSpace));
    OXR(xrDestroySpace(appState.LocalSpace));
    // StageSpace is optional.
    if (appState.StageSpace != XR_NULL_HANDLE) {
        OXR(xrDestroySpace(appState.StageSpace));
    }*/
    //OXR(xrDestroySpace(appState.FakeStageSpace));
    //appState.CurrentSpace = XR_NULL_HANDLE;
    //OXR(xrDestroySession(appState.Session));
    //OXR(xrDestroyInstance(appState.Instance));

    //ovrApp_Destroy(&appState);

    (*app->activity->vm).DetachCurrentThread();
}