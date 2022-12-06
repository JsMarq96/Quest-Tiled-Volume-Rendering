//
// Created by js on 7/11/22.
//

#ifndef __OPENXR_INSTANCE_H__
#define __OPENXR_INSTANCE_H__

#define XR_USE_GRAPHICS_API_OPENGL_ES 1
#define XR_USE_PLATFORM_ANDROID 1
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>

#include <openxr/openxr_oculus.h>
#include <openxr/openxr_oculus_helpers.h>

#include <cstring>
#include <cassert>
#include <GLES3/gl3.h>

#include <android/log.h>

#include "egl_context.h"
#include "app_data.h"
#include "application.h"
#include "device.h"

static XrInstance *global_xr_instance;
#define ALOGE(...) __android_log_print(ANDROID_LOG_ERROR, "OpenXr", __VA_ARGS__)
#define ALOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, "OpenXr", __VA_ARGS__)
inline void OXR_CheckErrors(XrResult result, const char* function, bool failOnError) {
    if (XR_FAILED(result)) {
        char errorBuffer[XR_MAX_RESULT_STRING_SIZE];
        xrResultToString(*global_xr_instance,
                         result,
                         errorBuffer);
        if (failOnError) {
            ALOGE("OpenXR error: %s: %s\n", "OpenXr", errorBuffer);
        } else {
            ALOGV("OpenXR error: %s: %s\n", "OpenXr", errorBuffer);
        }
    }
}
#define OXR(func) OXR_CheckErrors(func, #func, true);

struct sFrameTransforms {
    XrMatrix4x4f view[MAX_EYE_NUMBER];
    XrMatrix4x4f projection[MAX_EYE_NUMBER];
    XrMatrix4x4f viewprojection[MAX_EYE_NUMBER];
} __attribute__((packed));

// ===========================
// ACTIONS & INPUTS
// ==========================
enum eActionSide : int {
    LEFT = 0,
    RIGHT = 1,
    COUNT = 2
};

struct sInputState {
    XrActionSet actionSet{XR_NULL_HANDLE};
    XrAction grabAction{XR_NULL_HANDLE};
    XrAction poseAction{XR_NULL_HANDLE};
    XrAction vibrateAction{XR_NULL_HANDLE};
    XrAction quitAction{XR_NULL_HANDLE};

    XrPath handSubactionPath[COUNT];
    XrSpace handSpace[COUNT];
    float handScale[COUNT] = {1.0f, 1.0f};
    XrBool32 handActive[COUNT];
};

struct sOpenXRFramebuffer {
    uint32_t width = 0;
    uint32_t height = 0;
    // multisamples

    // SwapChain
    uint32_t swapchain_length = 0;
    uint32_t swapchain_index = 0;

    // Color swapchain
    XrSwapchain swapchain_handle;

    XrSwapchainImageOpenGLESKHR *swapchain_images = NULL;
    uint32_t *swapchain_gl_images = NULL;

    uint32_t depth_buffers;
    uint32_t color_buffers;

    void init(XrSession &session,
              const uint32_t i_width,
              const uint32_t i_height,
              const uint32_t array_size,
              const GLenum   color_format){
        width = i_width;
        height = i_height;

        // Skip format verification

        XrSwapchainCreateInfo swapchain_create = {
                .type = XR_TYPE_SWAPCHAIN_CREATE_INFO,
                .next = NULL,
                .usageFlags = XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT | XR_SWAPCHAIN_USAGE_SAMPLED_BIT,
                .format = color_format,
                .sampleCount = 1,
                .width = width,
                .height = height,
                .faceCount = 1, // ??
                .arraySize = array_size,
                .mipCount = 1
        };

        // TODO create FOVeationg
        OXR(xrCreateSwapchain(session,
                              &swapchain_create,
                              &swapchain_handle));

        // Generate the images for the swapchain
        // The number
        OXR(xrEnumerateSwapchainImages(swapchain_handle,
                                        0,
                                        &swapchain_length,
                                        NULL));

        // Allocate
        swapchain_images = (XrSwapchainImageOpenGLESKHR*) malloc(sizeof(XrSwapchainImageOpenGLESKHR) * swapchain_length);
        swapchain_gl_images = (uint32_t*) malloc(sizeof(uint32_t) * swapchain_length);
        // Fill
        for(uint32_t i = 0; i < swapchain_length; i++) {
            swapchain_images[i].type = XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_ES_KHR;
            //swapchain_images[i].next = NULL;
        }
        OXR(xrEnumerateSwapchainImages(swapchain_handle,
                                       swapchain_length,
                                       &swapchain_length,
                                       (XrSwapchainImageBaseHeader*) swapchain_images));
        for(uint32_t i = 0; i < swapchain_length; i++) {
            swapchain_gl_images[i] = swapchain_images[i].image;
        }
    }

    void adquire() {
        // Adquire & load swapchain images
        XrSwapchainImageAcquireInfo swapchain_acq_info = {
                .type = XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO,
                .next = NULL
        };

        XrSwapchainImageWaitInfo swapchain_wait_info = {
                .type = XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO,
                .next = NULL,
                .timeout = 1000000000
        };
        OXR(xrAcquireSwapchainImage(swapchain_handle,
                                    &swapchain_acq_info,
                                    &swapchain_index));

        XrResult res = xrWaitSwapchainImage(swapchain_handle,
                                            &swapchain_wait_info);

        while(res == XR_TIMEOUT_EXPIRED) {
            res = xrWaitSwapchainImage(swapchain_handle,
                                       &swapchain_wait_info);
        }
    }

    void release() {
        XrSwapchainImageReleaseInfo release_info = {
                .type = XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO,
                .next = NULL
        };

        OXR(xrReleaseSwapchainImage(swapchain_handle,
                                    &release_info))
    }
};

struct sOpenXR_Instance {
    // OpenXR session & context data
    XrInstance xr_instance;
    XrSystemId xr_sys_id;
    XrSession xr_session;
    XrSpace xr_stage_space, xr_local_space, xr_head_space;
    XrFrameState frame_state = {};
    sEglContext egl;

    XrEventDataBuffer xr_event_buffer = {};

    sOpenXRFramebuffer *curr_framebuffers;

    XrPosef viewTransform[MAX_EYE_NUMBER];

    XrViewConfigurationView view_configs[MAX_EYE_NUMBER];
    XrViewConfigurationProperties view_config_prop;
    XrView eye_projections[MAX_EYE_NUMBER];

    sInputState input_state;

    XrCompositionLayerProjectionView projection_views[MAX_EYE_NUMBER];
    XrCompositionLayerProjection projection_layer;

    const XrCompositionLayerBaseHeader *layers[10];
    uint8_t layers_count = 0;

    const XrViewConfigurationType stereo_view_config = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;

    void _load_instance() {
        uint32_t extension_count = 3; // TODO just the necessary extesions
        const char *enabled_extensions[12] = {
                XR_KHR_OPENGL_ES_ENABLE_EXTENSION_NAME,
                XR_EXT_PERFORMANCE_SETTINGS_EXTENSION_NAME,
                XR_KHR_ANDROID_THREAD_SETTINGS_EXTENSION_NAME,
                XR_FB_SWAPCHAIN_UPDATE_STATE_EXTENSION_NAME,
                XR_FB_SWAPCHAIN_UPDATE_STATE_OPENGL_ES_EXTENSION_NAME,
                XR_FB_FOVEATION_EXTENSION_NAME,
                XR_FB_FOVEATION_CONFIGURATION_EXTENSION_NAME,
                XR_FB_DISPLAY_REFRESH_RATE_EXTENSION_NAME,
                XR_FB_COLOR_SPACE_EXTENSION_NAME
        };
        XrInstanceCreateInfo instance_info = {
                .type = XR_TYPE_INSTANCE_CREATE_INFO,
                .next = NULL,
                .createFlags = 0,
                .applicationInfo = {
                        .applicationName = APP_NAME,
                        .applicationVersion = APP_VERSION,
                        .engineName = ENGINE_NAME,
                        .engineVersion = ENGINE_VERSION,
                        .apiVersion = XR_CURRENT_API_VERSION
                },
                .enabledApiLayerCount = 0,
                .enabledApiLayerNames = NULL,
                .enabledExtensionCount = extension_count, // to fill
                .enabledExtensionNames = enabled_extensions // to fill
        };

        XrResult initResult;
        OXR(initResult = xrCreateInstance(&instance_info, &xr_instance));
        if (initResult != XR_SUCCESS) {
            ALOGE("Failed to create XR instance: %d.", initResult);
            exit(1);
        }

        //global_xr_instance = &xr_instance;

        XrInstanceProperties instanceInfo;
        instanceInfo.type = XR_TYPE_INSTANCE_PROPERTIES;
        instanceInfo.next = NULL;
        OXR(xrGetInstanceProperties(xr_instance, &instanceInfo));
        ALOGV(
                "Runtime %s: Version : %u.%u.%u",
                instanceInfo.runtimeName,
                XR_VERSION_MAJOR(instanceInfo.runtimeVersion),
                XR_VERSION_MINOR(instanceInfo.runtimeVersion),
                XR_VERSION_PATCH(instanceInfo.runtimeVersion));

        XrSystemGetInfo systemGetInfo = {};
        systemGetInfo.type = XR_TYPE_SYSTEM_GET_INFO;
        systemGetInfo.next = NULL;
        systemGetInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;

        XrSystemId systemId;
        OXR(initResult = xrGetSystem(xr_instance, &systemGetInfo, &systemId));
        if (initResult != XR_SUCCESS) {
            ALOGE("Failed to get system.");
            exit(1);
        }

        XrSystemProperties systemProperties = {};
        systemProperties.type = XR_TYPE_SYSTEM_PROPERTIES;
        OXR(xrGetSystemProperties(xr_instance, systemId, &systemProperties));

        ALOGV(
                "System Properties: Name=%s VendorId=%x",
                systemProperties.systemName,
                systemProperties.vendorId);
        ALOGV(
                "System Graphics Properties: MaxWidth=%d MaxHeight=%d MaxLayers=%d",
                systemProperties.graphicsProperties.maxSwapchainImageWidth,
                systemProperties.graphicsProperties.maxSwapchainImageHeight,
                systemProperties.graphicsProperties.maxLayerCount);
        ALOGV(
                "System Tracking Properties: OrientationTracking=%s PositionTracking=%s",
                systemProperties.trackingProperties.orientationTracking ? "True" : "False",
                systemProperties.trackingProperties.positionTracking ? "True" : "False");

        // Get the graphics requirements.
        PFN_xrGetOpenGLESGraphicsRequirementsKHR pfnGetOpenGLESGraphicsRequirementsKHR = NULL;
        OXR(xrGetInstanceProcAddr(
                xr_instance,
                "xrGetOpenGLESGraphicsRequirementsKHR",
                (PFN_xrVoidFunction*)(&pfnGetOpenGLESGraphicsRequirementsKHR)));

        XrGraphicsRequirementsOpenGLESKHR graphicsRequirements = {};
        graphicsRequirements.type = XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_ES_KHR;
        OXR(pfnGetOpenGLESGraphicsRequirementsKHR(xr_instance, systemId, &graphicsRequirements));

        xr_sys_id = systemId;

        egl.create();

        int eglMajor = 0;
        int eglMinor = 0;
        glGetIntegerv(GL_MAJOR_VERSION, &eglMajor);
        glGetIntegerv(GL_MINOR_VERSION, &eglMinor);
        ALOGE("GLES version %d.%d", eglMajor, eglMinor);
        const XrVersion eglVersion = XR_MAKE_VERSION(eglMajor, eglMinor, 0);
        if (eglVersion < graphicsRequirements.minApiVersionSupported ||
            eglVersion > graphicsRequirements.maxApiVersionSupported) {
            ALOGE("GLES version %d.%d not supported", eglMajor, eglMinor);
            exit(0);
        }
    }

    void _create_session() {
        XrGraphicsBindingOpenGLESAndroidKHR graphics_binding = {
                .type = XR_TYPE_GRAPHICS_BINDING_OPENGL_ES_ANDROID_KHR,
                .next = NULL,
                .display = egl.display,
                .config = egl.config,
                .context = egl.context
        };

        XrSessionCreateInfo session_info = {
                .type = XR_TYPE_SESSION_CREATE_INFO,
                .next = &graphics_binding,
                //.createFlags = 0,
                .systemId = xr_sys_id
        };

        XrResult initResult;
        OXR(initResult = xrCreateSession(xr_instance,
                            &session_info,
                            &xr_session));
        if (initResult != XR_SUCCESS) {
            ALOGE("Failed to create XR session: %d.", initResult);
            exit(1);
        }
    }

    uint32_t _load_viewport_config() {
        const XrViewConfigurationType supportedViewConfigType =
                XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
        uint32_t viewport_config_count = 0;
        OXR(xrEnumerateViewConfigurations(xr_instance,
                                          xr_sys_id,
                                          0,
                                          &viewport_config_count,
                                          NULL));

        //assert(viewport_config_count > 0 && "No enought viewport configs on device");

        XrViewConfigurationType *viewport_configs = (XrViewConfigurationType*) malloc(sizeof(XrViewConfigurationType) * viewport_config_count);
        OXR(xrEnumerateViewConfigurations(xr_instance,
                                          xr_sys_id,
                                          viewport_config_count,
                                          &viewport_config_count,
                                          viewport_configs));

        ALOGV("Available Viewport Configuration Types: %d", viewport_config_count);

        for(uint32_t i = 0; i < viewport_config_count; i++) {
            const XrViewConfigurationType vp_config_type = viewport_configs[i];

            ALOGV(
                    "Viewport configuration type %d : %s",
                    vp_config_type,
                    vp_config_type == supportedViewConfigType ? "Selected" : "");

            XrViewConfigurationProperties viewportConfig;
            viewportConfig.type = XR_TYPE_VIEW_CONFIGURATION_PROPERTIES;
            OXR(xrGetViewConfigurationProperties(
                    xr_instance, xr_sys_id, vp_config_type, &viewportConfig));
            ALOGV(
                    "FovMutable=%s ConfigurationType %d",
                    viewportConfig.fovMutable ? "true" : "false",
                    viewportConfig.viewConfigurationType);

            uint32_t viewCount;
            OXR(xrEnumerateViewConfigurationViews(
                    xr_instance, xr_sys_id, vp_config_type, 0, &viewCount, NULL));

            if (viewCount > 0) {
                auto elements = new XrViewConfigurationView[viewCount];

                for (uint32_t e = 0; e < viewCount; e++) {
                    elements[e].type = XR_TYPE_VIEW_CONFIGURATION_VIEW;
                    elements[e].next = NULL;
                }

                OXR(xrEnumerateViewConfigurationViews(
                        xr_instance, xr_sys_id, vp_config_type, viewCount, &viewCount, elements));

                // Log the view config info for each view type for debugging purposes.
                for (uint32_t e = 0; e < viewCount; e++) {
                    const XrViewConfigurationView* element = &elements[e];

                    ALOGV(
                            "Viewport [%d]: Recommended Width=%d Height=%d SampleCount=%d",
                            e,
                            element->recommendedImageRectWidth,
                            element->recommendedImageRectHeight,
                            element->recommendedSwapchainSampleCount);

                    ALOGV(
                            "Viewport [%d]: Max Width=%d Height=%d SampleCount=%d",
                            e,
                            element->maxImageRectWidth,
                            element->maxImageRectHeight,
                            element->maxSwapchainSampleCount);
                }

                // Cache the view config properties for the selected config type.
                if (vp_config_type == supportedViewConfigType) {
                    assert(viewCount == 2);
                    for (uint32_t e = 0; e < viewCount; e++) {
                        view_configs[e] = elements[e];
                    }
                }

                delete[] elements;
            } else {
                ALOGE("Empty viewport configuration type: %d", viewCount);
            }
        }

        // Get the viewport config
        view_config_prop = {
                .type = XR_TYPE_VIEW_CONFIGURATION_PROPERTIES
        };

        OXR(xrGetViewConfigurationProperties(xr_instance,
                                             xr_sys_id,
                                             stereo_view_config,
                                             &view_config_prop));

        return viewport_config_count;
    }

    void _init_actions() {
        // Create an action set.
        {
            XrActionSetCreateInfo actionSetInfo{XR_TYPE_ACTION_SET_CREATE_INFO};
            strcpy(actionSetInfo.actionSetName, "gameplay");
            strcpy(actionSetInfo.localizedActionSetName, "Gameplay");
            actionSetInfo.priority = 0;
            (xrCreateActionSet(xr_instance, &actionSetInfo, &input_state.actionSet));
        }

        // Get the XrPath for the left and right hands - we will use them as subaction paths.
        (xrStringToPath(xr_instance, "/user/hand/left", &input_state.handSubactionPath[LEFT]));
        (xrStringToPath(xr_instance, "/user/hand/right", &input_state.handSubactionPath[RIGHT]));

        // Create actions.
        {
            // Create an input action for grabbing objects with the left and right hands.
            XrActionCreateInfo actionInfo{XR_TYPE_ACTION_CREATE_INFO};
            actionInfo.actionType = XR_ACTION_TYPE_FLOAT_INPUT;
            strcpy(actionInfo.actionName, "grab_object");
            strcpy(actionInfo.localizedActionName, "Grab Object");
            actionInfo.countSubactionPaths = COUNT;
            actionInfo.subactionPaths = input_state.handSubactionPath;
            (xrCreateAction(input_state.actionSet, &actionInfo, &input_state.grabAction));

            // Create an input action getting the left and right hand poses.
            actionInfo.actionType = XR_ACTION_TYPE_POSE_INPUT;
            strcpy(actionInfo.actionName, "hand_pose");
            strcpy(actionInfo.localizedActionName, "Hand Pose");
            actionInfo.countSubactionPaths = COUNT;
            actionInfo.subactionPaths = input_state.handSubactionPath;
            (xrCreateAction(input_state.actionSet, &actionInfo, &input_state.poseAction));

            // Create output actions for vibrating the left and right controller.
            actionInfo.actionType = XR_ACTION_TYPE_VIBRATION_OUTPUT;
            strcpy(actionInfo.actionName, "vibrate_hand");
            strcpy(actionInfo.localizedActionName, "Vibrate Hand");
            actionInfo.countSubactionPaths = COUNT;
            actionInfo.subactionPaths = input_state.handSubactionPath;
            (xrCreateAction(input_state.actionSet, &actionInfo, &input_state.vibrateAction));

            // Create input actions for quitting the session using the left and right controller.
            // Since it doesn't matter which hand did this, we do not specify subaction paths for it.
            // We will just suggest bindings for both hands, where possible.
            actionInfo.actionType = XR_ACTION_TYPE_BOOLEAN_INPUT;
            strcpy(actionInfo.actionName, "quit_session");
            strcpy(actionInfo.localizedActionName, "Quit Session");
            actionInfo.countSubactionPaths = 0;
            actionInfo.subactionPaths = nullptr;
            (xrCreateAction(input_state.actionSet, &actionInfo, &input_state.quitAction));
        }

        XrPath selectPath[COUNT];
        XrPath squeezeValuePath[COUNT];
        XrPath squeezeForcePath[COUNT];
        XrPath squeezeClickPath[COUNT];
        XrPath posePath[COUNT];
        XrPath hapticPath[COUNT];
        XrPath menuClickPath[COUNT];
        XrPath bClickPath[COUNT];
        XrPath triggerValuePath[COUNT];
        (xrStringToPath(xr_instance, "/user/hand/left/input/select/click", &selectPath[LEFT]));
        (xrStringToPath(xr_instance, "/user/hand/right/input/select/click", &selectPath[RIGHT]));
        (xrStringToPath(xr_instance, "/user/hand/left/input/squeeze/value", &squeezeValuePath[LEFT]));
        (xrStringToPath(xr_instance, "/user/hand/right/input/squeeze/value", &squeezeValuePath[RIGHT]));
        (xrStringToPath(xr_instance, "/user/hand/left/input/squeeze/force", &squeezeForcePath[LEFT]));
        (xrStringToPath(xr_instance, "/user/hand/right/input/squeeze/force", &squeezeForcePath[RIGHT]));
        (xrStringToPath(xr_instance, "/user/hand/left/input/squeeze/click", &squeezeClickPath[LEFT]));
        (xrStringToPath(xr_instance, "/user/hand/right/input/squeeze/click", &squeezeClickPath[RIGHT]));
        (xrStringToPath(xr_instance, "/user/hand/left/input/grip/pose", &posePath[LEFT]));
        (xrStringToPath(xr_instance, "/user/hand/right/input/grip/pose", &posePath[RIGHT]));
        (xrStringToPath(xr_instance, "/user/hand/left/output/haptic", &hapticPath[LEFT]));
        (xrStringToPath(xr_instance, "/user/hand/right/output/haptic", &hapticPath[RIGHT]));
        (xrStringToPath(xr_instance, "/user/hand/left/input/menu/click", &menuClickPath[LEFT]));
        (xrStringToPath(xr_instance, "/user/hand/right/input/menu/click", &menuClickPath[RIGHT]));
        (xrStringToPath(xr_instance, "/user/hand/left/input/b/click", &bClickPath[LEFT]));
        (xrStringToPath(xr_instance, "/user/hand/right/input/b/click", &bClickPath[RIGHT]));
        (xrStringToPath(xr_instance, "/user/hand/left/input/trigger/value", &triggerValuePath[LEFT]));
        (xrStringToPath(xr_instance, "/user/hand/right/input/trigger/value", &triggerValuePath[RIGHT]));
        // Suggest bindings for KHR Simple.
        {
            XrPath khrSimpleInteractionProfilePath;
            (
                    xrStringToPath(xr_instance, "/interaction_profiles/khr/simple_controller", &khrSimpleInteractionProfilePath));
            XrActionSuggestedBinding bindings[8] = {// Fall back to a click input for the grab action.
                    {input_state.grabAction, selectPath[LEFT]},
                    {input_state.grabAction, selectPath[RIGHT]},
                    {input_state.poseAction, posePath[LEFT]},
                    {input_state.poseAction, posePath[RIGHT]},
                    {input_state.quitAction, menuClickPath[LEFT]},
                    {input_state.quitAction, menuClickPath[RIGHT]},
                    {input_state.vibrateAction, hapticPath[LEFT]},
                    {input_state.vibrateAction, hapticPath[RIGHT]}};
            XrInteractionProfileSuggestedBinding suggestedBindings{XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING};
            suggestedBindings.interactionProfile = khrSimpleInteractionProfilePath;
            suggestedBindings.suggestedBindings = bindings;
            suggestedBindings.countSuggestedBindings = 8;
            (xrSuggestInteractionProfileBindings(xr_instance, &suggestedBindings));
        }
        // Suggest bindings for the Oculus Touch.
        {
            XrPath oculusTouchInteractionProfilePath;
            (
                    xrStringToPath(xr_instance, "/interaction_profiles/oculus/touch_controller", &oculusTouchInteractionProfilePath));
            XrActionSuggestedBinding bindings[7] = {{input_state.grabAction, squeezeValuePath[LEFT]},
                                                    {input_state.grabAction, squeezeValuePath[RIGHT]},
                                                    {input_state.poseAction, posePath[LEFT]},
                                                    {input_state.poseAction, posePath[RIGHT]},
                                                    {input_state.quitAction, menuClickPath[LEFT]},
                                                    {input_state.vibrateAction, hapticPath[LEFT]},
                                                    {input_state.vibrateAction, hapticPath[RIGHT]}};
            XrInteractionProfileSuggestedBinding suggestedBindings{XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING};
            suggestedBindings.interactionProfile = oculusTouchInteractionProfilePath;
            suggestedBindings.suggestedBindings = bindings;
            suggestedBindings.countSuggestedBindings = 7;
            (xrSuggestInteractionProfileBindings(xr_instance, &suggestedBindings));
        }
        // Suggest bindings for the Vive Controller.
        /*{
            XrPath viveControllerInteractionProfilePath;
            (
                    xrStringToPath(xr_instance, "/interaction_profiles/htc/vive_controller", &viveControllerInteractionProfilePath));
            std::vector<XrActionSuggestedBinding> bindings{{{input_state.grabAction, triggerValuePath[LEFT]},
                                                            {input_state.grabAction, triggerValuePath[RIGHT]},
                                                            {input_state.poseAction, posePath[LEFT]},
                                                            {input_state.poseAction, posePath[RIGHT]},
                                                            {input_state.quitAction, menuClickPath[LEFT]},
                                                            {input_state.quitAction, menuClickPath[RIGHT]},
                                                            {input_state.vibrateAction, hapticPath[LEFT]},
                                                            {input_state.vibrateAction, hapticPath[RIGHT]}}};
            XrInteractionProfileSuggestedBinding suggestedBindings{XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING};
            suggestedBindings.interactionProfile = viveControllerInteractionProfilePath;
            suggestedBindings.suggestedBindings = bindings.data();
            suggestedBindings.countSuggestedBindings = (uint32_t)bindings.size();
            (xrSuggestInteractionProfileBindings(xr_instance, &suggestedBindings));
        }

        // Suggest bindings for the Valve Index Controller.
        {
            XrPath indexControllerInteractionProfilePath;
            (
                    xrStringToPath(xr_instance, "/interaction_profiles/valve/index_controller", &indexControllerInteractionProfilePath));
            std::vector<XrActionSuggestedBinding> bindings{{{input_state.grabAction, squeezeForcePath[LEFT]},
                                                            {input_state.grabAction, squeezeForcePath[RIGHT]},
                                                            {input_state.poseAction, posePath[LEFT]},
                                                            {input_state.poseAction, posePath[RIGHT]},
                                                            {input_state.quitAction, bClickPath[LEFT]},
                                                            {input_state.quitAction, bClickPath[RIGHT]},
                                                            {input_state.vibrateAction, hapticPath[LEFT]},
                                                            {input_state.vibrateAction, hapticPath[RIGHT]}}};
            XrInteractionProfileSuggestedBinding suggestedBindings{XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING};
            suggestedBindings.interactionProfile = indexControllerInteractionProfilePath;
            suggestedBindings.suggestedBindings = bindings.data();
            suggestedBindings.countSuggestedBindings = (uint32_t)bindings.size();
            (xrSuggestInteractionProfileBindings(xr_instance, &suggestedBindings));
        }

        // Suggest bindings for the Microsoft Mixed Reality Motion Controller.
        {
            XrPath microsoftMixedRealityInteractionProfilePath;
            (xrStringToPath(xr_instance, "/interaction_profiles/microsoft/motion_controller",
                                       &microsoftMixedRealityInteractionProfilePath));
            std::vector<XrActionSuggestedBinding> bindings{{{input_state.grabAction, squeezeClickPath[LEFT]},
                                                            {input_state.grabAction, squeezeClickPath[RIGHT]},
                                                            {input_state.poseAction, posePath[LEFT]},
                                                            {input_state.poseAction, posePath[RIGHT]},
                                                            {input_state.quitAction, menuClickPath[LEFT]},
                                                            {input_state.quitAction, menuClickPath[RIGHT]},
                                                            {input_state.vibrateAction, hapticPath[LEFT]},
                                                            {input_state.vibrateAction, hapticPath[RIGHT]}}};
            XrInteractionProfileSuggestedBinding suggestedBindings{XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING};
            suggestedBindings.interactionProfile = microsoftMixedRealityInteractionProfilePath;
            suggestedBindings.suggestedBindings = bindings.data();
            suggestedBindings.countSuggestedBindings = (uint32_t)bindings.size();
            (xrSuggestInteractionProfileBindings(xr_instance, &suggestedBindings));
        }*/
        XrActionSpaceCreateInfo actionSpaceInfo{XR_TYPE_ACTION_SPACE_CREATE_INFO};
        actionSpaceInfo.action = input_state.poseAction;
        actionSpaceInfo.poseInActionSpace.orientation.w = 1.f;
        actionSpaceInfo.subactionPath = input_state.handSubactionPath[LEFT];
        (xrCreateActionSpace(xr_session, &actionSpaceInfo, &input_state.handSpace[LEFT]));
        actionSpaceInfo.subactionPath = input_state.handSubactionPath[RIGHT];
        (xrCreateActionSpace(xr_session, &actionSpaceInfo, &input_state.handSpace[RIGHT]));

        XrSessionActionSetsAttachInfo attachInfo{XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO};
        attachInfo.countActionSets = 1;
        attachInfo.actionSets = &input_state.actionSet;
        (xrAttachSessionActionSets(xr_session, &attachInfo));
    }

    void _load_reference_spaces() {
        XrReferenceSpaceCreateInfo spaceCreateInfo = {};
        spaceCreateInfo.type = XR_TYPE_REFERENCE_SPACE_CREATE_INFO;
        spaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW;
        spaceCreateInfo.poseInReferenceSpace.orientation.w = 1.0f;
        OXR(xrCreateReferenceSpace(xr_session, &spaceCreateInfo, &xr_head_space));

        spaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
        OXR(xrCreateReferenceSpace(xr_session, &spaceCreateInfo, &xr_local_space));

        spaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_STAGE;
        spaceCreateInfo.poseInReferenceSpace.position.y = 0.0f;
        OXR(xrCreateReferenceSpace(xr_session, &spaceCreateInfo, &xr_stage_space));
        ALOGV("Created stage space");
    }

    void init(sOpenXRFramebuffer *framebuffers) {
        curr_framebuffers = framebuffers;
        // Load extensions ==============================================================
        _load_instance();

        // Create the OpenXR session ===================================================
        _create_session();
        // Load ViewPort data & config  ================================================


        // TODO Color space
        // TODO Refresh rate

        _init_actions();

        _load_viewport_config();

        _load_reference_spaces();


        // CONFIG VIEWS ================================================================
        {
            for(uint16_t i = 0; i < MAX_EYE_NUMBER; i++) { // To fill on the realtime
                memset(&eye_projections[i],
                       0,
                       sizeof(XrView));
                eye_projections[i].type = XR_TYPE_VIEW;
                eye_projections[i].next = NULL;
            }
        }

        // TODO CONFIG ACTION SPACES

        // Create SWAPCHAIN ===========================================================
        framebuffers->init(xr_session,
                             view_configs[0].recommendedImageRectWidth,
                             view_configs[0].recommendedImageRectHeight,
                             2,
                             GL_SRGB8_ALPHA8);

        // TODO: Foveation
    }

    void session_change_state(const XrSessionState &state,
                              Application::sAndroidState *android_state) {
        ALOGE("OpenXR session:%i wanted %i\n", state, XR_SESSION_STATE_READY);
        if (state == XR_SESSION_STATE_READY) {
            // Start the session!
            XrSessionBeginInfo session_start_info = {
                    .type = XR_TYPE_SESSION_BEGIN_INFO,
                    .next = NULL,
                    .primaryViewConfigurationType = view_config_prop.viewConfigurationType
            };

            XrResult result;
            OXR(result = xrBeginSession(xr_session,
                               &session_start_info));
            android_state->session_active = true;

            PFN_xrPerfSettingsSetPerformanceLevelEXT pfnPerfSettingsSetPerformanceLevelEXT = NULL;
            OXR(xrGetInstanceProcAddr(xr_instance,
                                      "xrPerfSettingsSetPerformanceLevelEXT",
                                      (PFN_xrVoidFunction*)(&pfnPerfSettingsSetPerformanceLevelEXT)));

            // Set the CPU performance profile
            const Device::eCPUGPUPower cpu_power_target = Device::LVL_1_LOW_POWER;
            OXR(pfnPerfSettingsSetPerformanceLevelEXT(xr_session,
                                                      XR_PERF_SETTINGS_DOMAIN_CPU_EXT,
                                                      (XrPerfSettingsLevelEXT) cpu_power_target));
            // Set the GPU performance profile
            const Device::eCPUGPUPower gpu_power_target = Device::LVL_2_HIGH_POWER;
            OXR(pfnPerfSettingsSetPerformanceLevelEXT(xr_session,
                                                      XR_PERF_SETTINGS_DOMAIN_GPU_EXT,
                                                      (XrPerfSettingsLevelEXT) gpu_power_target));

            // Set the main thread and the render thread as high priority threads
            PFN_xrSetAndroidApplicationThreadKHR pfnSetAndroidApplicationThreadKHR = NULL;
            OXR(xrGetInstanceProcAddr(xr_instance,
                                      "xrSetAndroidApplicationThreadKHR",
                                      (PFN_xrVoidFunction*)(&pfnSetAndroidApplicationThreadKHR)));

            OXR(pfnSetAndroidApplicationThreadKHR(xr_session,
                                                 XR_ANDROID_THREAD_TYPE_APPLICATION_MAIN_KHR,
                                                 android_state->main_thread));
            OXR(pfnSetAndroidApplicationThreadKHR(xr_session,
                                                  XR_ANDROID_THREAD_TYPE_RENDERER_MAIN_KHR,
                                                  android_state->render_thread));
        } else if (state == XR_SESSION_STATE_STOPPING) {
            xrEndSession(xr_session);
            android_state->session_active = false;
        }
    }

    void handle_events(Application::sAndroidState *app_state) {
        for(;;) {
            XrEventDataBaseHeader *base_header = (XrEventDataBaseHeader*) &xr_event_buffer;
            *base_header = {XR_TYPE_EVENT_DATA_BUFFER};

            XrResult res = xrPollEvent(xr_instance,
                                       &xr_event_buffer);

            if (res != XR_SUCCESS) {
                break;
            }

            const XrEventDataSessionStateChanged* session_state_changed_event = (XrEventDataSessionStateChanged*) base_header;

            ALOGE("OpenXR event type:%i, but we want %i\n", base_header->type, XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED);
            switch(base_header->type) {
                case XR_TYPE_EVENT_DATA_PERF_SETTINGS_EXT: // Change on perf setting
                    // TODO
                    break;
                case XR_TYPE_EVENT_DATA_DISPLAY_REFRESH_RATE_CHANGED_FB: // Change refresh rate
                    // TODO
                    break;
                case XR_TYPE_EVENT_DATA_REFERENCE_SPACE_CHANGE_PENDING: // Change the reference space
                    // TODO
                    break;
                case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED: // Session changed
                    switch(session_state_changed_event->state) {
                        case XR_SESSION_STATE_FOCUSED:
                            app_state->focused = true;
                            break;
                        case XR_SESSION_STATE_VISIBLE:
                            app_state->visible = true;
                            app_state->focused = false;
                            break;
                        case XR_SESSION_STATE_READY:
                        case XR_SESSION_STATE_STOPPING:
                            session_change_state(session_state_changed_event->state,
                                                 app_state);
                            break;
                        default:
                            break;
                    }
                    break;
                default: break;
            }
        }
    }


    void update(Application::sAndroidState *app_state,
                double *delta_time,
                sFrameTransforms *transforms) {
        // Handle XR events
        //handle_events(app_state);

        // get the predicted frametimes from OpenXR
        XrFrameWaitInfo waitFrameInfo = {};
        waitFrameInfo.type = XR_TYPE_FRAME_WAIT_INFO;
        waitFrameInfo.next = NULL;

        frame_state = {};
        frame_state.type = XR_TYPE_FRAME_STATE;
        frame_state.next = NULL;

        OXR(xrWaitFrame(xr_session,
                        &waitFrameInfo,
                        &frame_state));

        // Get the HMD pose, predicted for the middle of the time period during which
        // the new eye images will be displayed. The number of frames predicted ahead
        // depends on the pipeline depth of the engine and the synthesis rate.
        // The better the prediction, the less black will be pulled in at the edges.

        XrFrameBeginInfo begin_frame_desc = {
                .type = XR_TYPE_FRAME_BEGIN_INFO,
                .next = NULL
        };
        OXR(xrBeginFrame(xr_session,
                         &begin_frame_desc));

        // Get space tracking ===
        XrSpaceLocation space_location = {
                .type = XR_TYPE_SPACE_LOCATION,
                .next = NULL
        };
        OXR(xrLocateSpace(xr_stage_space,
                          xr_local_space,
                          frame_state.predictedDisplayTime,
                          &space_location));
        XrPosef pose_stage_from_head = space_location.pose;

        // Get Projection ===
        XrViewLocateInfo projection_info = {
                .type = XR_TYPE_VIEW_LOCATE_INFO,
                .next = NULL,
                .viewConfigurationType = view_config_prop.viewConfigurationType,
                .displayTime = frame_state.predictedDisplayTime,
                .space = xr_head_space
        };

        XrViewState view_state = {
                .type = XR_TYPE_VIEW_STATE,
                .next = NULL
        };

        uint32_t projection_capacity = MAX_EYE_NUMBER;
        OXR(xrLocateViews(xr_session,
                          &projection_info,
                          &view_state,
                          projection_capacity,
                          &projection_capacity,
                          eye_projections));

        *delta_time = FromXrTime(frame_state.predictedDisplayTime);

        // Generate view projections
        for (int eye = 0; eye < MAX_EYE_NUMBER; eye++) {
            XrPosef xfHeadFromEye = eye_projections[eye].pose;
            XrPosef xfStageFromEye = XrPosef_Multiply(pose_stage_from_head,
                                                      xfHeadFromEye);
            viewTransform[eye] = XrPosef_Inverse(xfStageFromEye);

            transforms->view[eye] = XrMatrix4x4f_CreateFromRigidTransform(&viewTransform[eye]);

            const XrFovf fov = eye_projections[eye].fov;
            XrMatrix4x4f_CreateProjectionFov(&transforms->projection[eye],
                                             GRAPHICS_OPENGL_ES,
                                             fov,
                                             0.1f,
                                             0.0f);

            XrMatrix4x4f_Multiply(&transforms->viewprojection[eye],
                                  &transforms->view[eye],
                                  &transforms->projection[eye]);
        }
    }

    void submit_frame() {
        // Realese the swapchain i


        // Composite layer

        XrFrameEndInfo frame_end_info = {
                .type = XR_TYPE_FRAME_END_INFO,
                .next = NULL,
                .displayTime = frame_state.predictedDisplayTime,
                .environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE,
                .layerCount = layers_count,
                .layers = layers,
        };

        {
            projection_layer = {
                    .type = XR_TYPE_COMPOSITION_LAYER_PROJECTION,
                    .next = NULL,
                    .layerFlags = 0,//XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT | XR_COMPOSITION_LAYER_CORRECT_CHROMATIC_ABERRATION_BIT,
                    .space =  xr_stage_space,
                    .viewCount = MAX_EYE_NUMBER,
                    .views = projection_views
            };

            for(uint16_t i = 0; i < MAX_EYE_NUMBER; i++) {
                memset(&projection_views[i],
                       0,
                       sizeof(XrCompositionLayerProjectionView));
                memset(&projection_views[i].subImage,
                       0,
                       sizeof(XrSwapchainSubImage));

                projection_views[i] = {
                        .type = XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW,
                        .next = NULL,
                        .pose = XrPosef_Inverse(viewTransform[i]),
                        .fov = eye_projections[i].fov,
                        .subImage = {
                                .swapchain = curr_framebuffers[i].swapchain_handle,
                                .imageRect = {
                                        .offset = {0,0},
                                        .extent = {
                                                .width = (int32_t) curr_framebuffers[i].width,
                                                .height = (int32_t) curr_framebuffers[i].height
                                        }
                                },
                                .imageArrayIndex = 0
                        }
                };
            }

            layers[layers_count++] =  (const XrCompositionLayerBaseHeader*) &projection_layer;
        }

        global_xr_instance = &xr_instance;
        XrResult result = xrEndFrame(xr_session,
                       &frame_end_info);
        char errorBuffer[XR_MAX_RESULT_STRING_SIZE];
        xrResultToString(*global_xr_instance,
                         result,
                         errorBuffer);
        ALOGE("OpenXR errorsss: %s: %s\n", "OpenXr test", errorBuffer);
    }
};

#endif //__OPENXR_INSTANCE_H__
