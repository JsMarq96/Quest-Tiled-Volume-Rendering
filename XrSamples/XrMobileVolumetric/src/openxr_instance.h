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

#include <glm/glm.hpp>

#include "test_perspectives.h"
#include "egl_context.h"
#include "app_data.h"
#include "device.h"
#include "glm/gtc/type_ptr.hpp"

struct sFrameTransforms {
    glm::mat4x4 view[MAX_EYE_NUMBER];
    glm::mat4x4 projection[MAX_EYE_NUMBER];
    glm::mat4x4 viewprojection[MAX_EYE_NUMBER];
};

// Logging gunctions
static XrInstance *global_xr_instance;
#define ALOGE(...) __android_log_print(ANDROID_LOG_ERROR, "OpenXr", __VA_ARGS__)
#define ALOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, "OpenXr", __VA_ARGS__)

inline void OXR_CheckErrors(XrResult result, const char *function, bool failOnError) {
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

namespace OpenXRHelpers {
    // From https://github.com/jherico/OpenXR-Samples/blob/master/src/examples/sdl2_gl_single_file_example.cpp
    inline void pose_to_glm_mat(const XrPosef &origin,
                                glm::mat4x4 *result) {
        glm::mat4 orientation = (glm::mat4_cast((glm::quat(origin.orientation.w,
                                                           origin.orientation.x,
                                                           origin.orientation.y,
                                                           origin.orientation.z))));
        glm::mat4 translation = glm::translate(glm::mat4{1.0f},
                                               glm::vec3(origin.position.x,
                                                         origin.position.y,
                                                         origin.position.z));

        glm::mat4 scale = glm::scale(glm::mat4{1.0f},
                                     glm::vec3(1.0f,
                                               1.0f,
                                               1.0f));
        *result = translation * orientation * scale;
    }

    inline void create_glm_projection(const XrFovf &fov,
                                      const float near,
                                      const float far,
                                      glm::mat4x4 *result) {
        const float tanAngleRight = tanf(fov.angleRight);
        const float tanAngleLeft = tanf(fov.angleLeft);
        const float tanAngleUp = tanf(fov.angleUp);
        const float tanAngleDown = tanf(fov.angleDown);

        const float tanAngleWidth = tanAngleRight - tanAngleLeft;
        const float tanAngleHeight = (tanAngleDown - tanAngleUp ); // (tanAngleUp  - tanAngleDown)
        const float offsetZ = 0;

        float *result_as_arr = (float *) &result[0][0];
        // normal projection
        result_as_arr[0] = 2 / tanAngleWidth;
        result_as_arr[4] = 0;
        result_as_arr[8] = (tanAngleRight + tanAngleLeft) / tanAngleWidth;
        result_as_arr[12] = 0;

        result_as_arr[1] = 0;
        result_as_arr[5] = 2 / tanAngleHeight;
        result_as_arr[9] = (tanAngleUp + tanAngleDown) / tanAngleHeight;
        result_as_arr[13] = 0;

        result_as_arr[2] = 0;
        result_as_arr[6] = 0;
        result_as_arr[10] = -(far + offsetZ) / (far - near);
        result_as_arr[14] = -(far * (near + offsetZ)) / (far - near);

        result_as_arr[3] = 0;
        result_as_arr[7] = 0;
        result_as_arr[11] = -1;
        result_as_arr[15] = 0;
    }
}

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

    uint32_t depth_buffers;
    uint32_t color_buffers;

    void init(XrSession &session,
              const uint32_t i_width,
              const uint32_t i_height,
              const GLenum color_format) {
        width = i_width;
        height = i_height;

        // Skip format verification

        XrSwapchainCreateInfo swapchain_create = {
                .type = XR_TYPE_SWAPCHAIN_CREATE_INFO,
                .next = NULL,
                .usageFlags = XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT |
                              XR_SWAPCHAIN_USAGE_SAMPLED_BIT,
                .format = color_format,
                .sampleCount = 1,
                .width = width,
                .height = height,
                .faceCount = 1, // ??
                .arraySize = 1,
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
        swapchain_images = (XrSwapchainImageOpenGLESKHR *) malloc(
                sizeof(XrSwapchainImageOpenGLESKHR) * swapchain_length);
        // Fill
        for (uint32_t i = 0; i < swapchain_length; i++) {
            swapchain_images[i].type = XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_ES_KHR;
            swapchain_images[i].next = NULL;
        }
        OXR(xrEnumerateSwapchainImages(swapchain_handle,
                                       swapchain_length,
                                       &swapchain_length,
                                       (XrSwapchainImageBaseHeader *) swapchain_images));
    }

    uint32_t adquire() {
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

        while (res == XR_TIMEOUT_EXPIRED) {
            res = xrWaitSwapchainImage(swapchain_handle,
                                       &swapchain_wait_info);
        }

        return swapchain_index;
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
    XrSpace xr_reference_space;
    bool space_stage_enabled = false;
    XrFrameState frame_state = {};
    sEglContext egl;

    XrEventDataBuffer xr_event_buffer = {};

    sOpenXRFramebuffer *curr_framebuffers;

    XrViewConfigurationView view_configs[MAX_EYE_NUMBER];
    XrViewConfigurationProperties view_config_prop;
    XrView eye_projections[MAX_EYE_NUMBER];

    sInputState input_state;

    XrCompositionLayerProjectionView projection_views[MAX_EYE_NUMBER];
    XrCompositionLayerProjection projection_layer;
    XrCompositionLayerImageLayoutFB projection_correction;

    const XrCompositionLayerBaseHeader *layers[10];
    uint8_t layers_count = 0;

    const XrViewConfigurationType stereo_view_config = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;

    void _load_instance() {
        XrResult result;
        PFN_xrEnumerateInstanceExtensionProperties xrEnumerateInstanceExtensionProperties;
        (result = xrGetInstanceProcAddr(
                XR_NULL_HANDLE,
                "xrEnumerateInstanceExtensionProperties",
                (PFN_xrVoidFunction *) &xrEnumerateInstanceExtensionProperties));
        if (result != XR_SUCCESS) {
            //ALOGE("Failed to get xrEnumerateInstanceExtensionProperties function pointer.");
            exit(1);
        }

        uint32_t numInputExtensions = 0;
        uint32_t numOutputExtensions = 0;
        (xrEnumerateInstanceExtensionProperties(NULL,
                                                numInputExtensions,
                                                &numOutputExtensions,
                                                NULL));

        numInputExtensions = numOutputExtensions;

        XrExtensionProperties *extensionProperties =
                (XrExtensionProperties *) malloc(
                        numOutputExtensions * sizeof(XrExtensionProperties));

        for (uint32_t i = 0; i < numOutputExtensions; i++) {
            extensionProperties[i].type = XR_TYPE_EXTENSION_PROPERTIES;
            extensionProperties[i].next = NULL;
        }

        OXR(xrEnumerateInstanceExtensionProperties(NULL,
                                                   numInputExtensions,
                                                   &numOutputExtensions,
                                                   extensionProperties));


        uint32_t extension_count = 9;// TODO just the necessary extesions
        const char *enabled_extensions[12] = {
                XR_KHR_OPENGL_ES_ENABLE_EXTENSION_NAME,
                XR_EXT_PERFORMANCE_SETTINGS_EXTENSION_NAME,
                XR_KHR_ANDROID_THREAD_SETTINGS_EXTENSION_NAME,
                XR_FB_COMPOSITION_LAYER_IMAGE_LAYOUT_EXTENSION_NAME,
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

        OXR(xrCreateInstance(&instance_info,
                             &xr_instance));

        global_xr_instance = &xr_instance;

        XrSystemGetInfo system_info = {
                .type = XR_TYPE_SYSTEM_GET_INFO,
                .next = NULL,
                .formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY
        };

        OXR(xrGetSystem(xr_instance,
                        &system_info,
                        &xr_sys_id));
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

        OXR(xrCreateSession(xr_instance,
                            &session_info,
                            &xr_session));
    }

    uint32_t _load_viewport_config() {
        uint32_t viewport_count = 0;
        OXR(xrEnumerateViewConfigurations(xr_instance,
                                          xr_sys_id,
                                          0,
                                          &viewport_count,
                                          NULL));

        assert(viewport_count > 0 && "No enought viewport configs on device");

        XrViewConfigurationType *viewport_configs = (XrViewConfigurationType *) malloc(
                sizeof(XrViewConfigurationType) * viewport_count);
        OXR(xrEnumerateViewConfigurations(xr_instance,
                                          xr_sys_id,
                                          viewport_count,
                                          &viewport_count,
                                          viewport_configs));

        for (uint32_t i = 0; i < viewport_count; i++) {
            const XrViewConfigurationType vp_config_type = viewport_configs[i];

            // Only interested in stereo rendering
            if (vp_config_type != stereo_view_config) {
                continue;
            }

            uint32_t view_count = 0;
            OXR(xrEnumerateViewConfigurationViews(xr_instance,
                                                  xr_sys_id,
                                                  vp_config_type,
                                                  0,
                                                  &view_count,
                                                  NULL));
            if (view_count <= 0) {
                continue; // Empty viewport
            }

            assert(view_count == MAX_EYE_NUMBER && "Only need two eyes on stereo rendering!");

            XrViewConfigurationView *views = (XrViewConfigurationView *) malloc(
                    sizeof(XrViewConfigurationView) * view_count);
            for (uint32_t j = 0; j < view_count; j++) {
                views[j].type = XR_TYPE_VIEW_CONFIGURATION_VIEW;
                views[j].next = NULL;
            }

            OXR(xrEnumerateViewConfigurationViews(xr_instance,
                                                  xr_sys_id,
                                                  vp_config_type,
                                                  view_count,
                                                  &view_count,
                                                  views));

            view_configs[0] = views[0];
            view_configs[1] = views[1];
        }

        view_config_prop = {
                .type = XR_TYPE_VIEW_CONFIGURATION_PROPERTIES,
                .next = NULL
        };

        OXR(xrGetViewConfigurationProperties(xr_instance,
                                             xr_sys_id,
                                             stereo_view_config,
                                             &view_config_prop));

        return viewport_count;
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
        (xrStringToPath(xr_instance, "/user/hand/right",
                        &input_state.handSubactionPath[RIGHT]));

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
        (xrStringToPath(xr_instance, "/user/hand/right/input/select/click",
                        &selectPath[RIGHT]));
        (xrStringToPath(xr_instance, "/user/hand/left/input/squeeze/value",
                        &squeezeValuePath[LEFT]));
        (xrStringToPath(xr_instance, "/user/hand/right/input/squeeze/value",
                        &squeezeValuePath[RIGHT]));
        (xrStringToPath(xr_instance, "/user/hand/left/input/squeeze/force",
                        &squeezeForcePath[LEFT]));
        (xrStringToPath(xr_instance, "/user/hand/right/input/squeeze/force",
                        &squeezeForcePath[RIGHT]));
        (xrStringToPath(xr_instance, "/user/hand/left/input/squeeze/click",
                        &squeezeClickPath[LEFT]));
        (xrStringToPath(xr_instance, "/user/hand/right/input/squeeze/click",
                        &squeezeClickPath[RIGHT]));
        (xrStringToPath(xr_instance, "/user/hand/left/input/grip/pose", &posePath[LEFT]));
        (xrStringToPath(xr_instance, "/user/hand/right/input/grip/pose", &posePath[RIGHT]));
        (xrStringToPath(xr_instance, "/user/hand/left/output/haptic", &hapticPath[LEFT]));
        (xrStringToPath(xr_instance, "/user/hand/right/output/haptic", &hapticPath[RIGHT]));
        (xrStringToPath(xr_instance, "/user/hand/left/input/menu/click", &menuClickPath[LEFT]));
        (xrStringToPath(xr_instance, "/user/hand/right/input/menu/click",
                        &menuClickPath[RIGHT]));
        (xrStringToPath(xr_instance, "/user/hand/left/input/b/click", &bClickPath[LEFT]));
        (xrStringToPath(xr_instance, "/user/hand/right/input/b/click", &bClickPath[RIGHT]));
        (xrStringToPath(xr_instance, "/user/hand/left/input/trigger/value",
                        &triggerValuePath[LEFT]));
        (xrStringToPath(xr_instance, "/user/hand/right/input/trigger/value",
                        &triggerValuePath[RIGHT]));
        // Suggest bindings for KHR Simple.
        {
            XrPath khrSimpleInteractionProfilePath;
            (
                    xrStringToPath(xr_instance, "/interaction_profiles/khr/simple_controller",
                                   &khrSimpleInteractionProfilePath));
            XrActionSuggestedBinding bindings[8] = {// Fall back to a click input for the grab action.
                    {input_state.grabAction,    selectPath[LEFT]},
                    {input_state.grabAction,    selectPath[RIGHT]},
                    {input_state.poseAction,    posePath[LEFT]},
                    {input_state.poseAction,    posePath[RIGHT]},
                    {input_state.quitAction,    menuClickPath[LEFT]},
                    {input_state.quitAction,    menuClickPath[RIGHT]},
                    {input_state.vibrateAction, hapticPath[LEFT]},
                    {input_state.vibrateAction, hapticPath[RIGHT]}};
            XrInteractionProfileSuggestedBinding suggestedBindings{
                    XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING};
            suggestedBindings.interactionProfile = khrSimpleInteractionProfilePath;
            suggestedBindings.suggestedBindings = bindings;
            suggestedBindings.countSuggestedBindings = 8;
            (xrSuggestInteractionProfileBindings(xr_instance, &suggestedBindings));
        }
        // Suggest bindings for the Oculus Touch.
        {
            XrPath oculusTouchInteractionProfilePath;
            (
                    xrStringToPath(xr_instance, "/interaction_profiles/oculus/touch_controller",
                                   &oculusTouchInteractionProfilePath));
            XrActionSuggestedBinding bindings[7] = {{input_state.grabAction,    squeezeValuePath[LEFT]},
                                                    {input_state.grabAction,    squeezeValuePath[RIGHT]},
                                                    {input_state.poseAction,    posePath[LEFT]},
                                                    {input_state.poseAction,    posePath[RIGHT]},
                                                    {input_state.quitAction,    menuClickPath[LEFT]},
                                                    {input_state.vibrateAction, hapticPath[LEFT]},
                                                    {input_state.vibrateAction, hapticPath[RIGHT]}};
            XrInteractionProfileSuggestedBinding suggestedBindings{
                    XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING};
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
        uint32_t output_spaces_count = 0;
        OXR(xrEnumerateReferenceSpaces(xr_session,
                                       0, &output_spaces_count,
                                       NULL));

        XrReferenceSpaceType *reference_spaces = (XrReferenceSpaceType *) malloc(
                sizeof(XrReferenceSpaceType) * output_spaces_count);

        OXR(xrEnumerateReferenceSpaces(xr_session,
                                       output_spaces_count,
                                       &output_spaces_count,
                                       reference_spaces));


        for (uint32_t i = 0; i < 0; i++) {
            if (reference_spaces[i] == XR_REFERENCE_SPACE_TYPE_STAGE) {
                ALOGE("SUPPORTED BEIBI");
                space_stage_enabled = true;
                break; // Supported
            }
        }
        if (!space_stage_enabled) {
            ALOGE("SPACE STAGE UNSUPPORTED");
        }

        free(reference_spaces);

        // Create the reference spaces for the rendering
        // https://gitlab.freedesktop.org/monado/demos/openxr-simple-example/-/blob/master/main.c
        XrReferenceSpaceCreateInfo space_info = {
                .type = XR_TYPE_REFERENCE_SPACE_CREATE_INFO,
                .next = NULL,
                .referenceSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW,
                .poseInReferenceSpace = { .orientation = {0.0f, 0.0f, 0.0f, 1.0f},
                                           .position = {0.0f, 0.0f, 0.0f}
                                          }
        };

        //space_info.poseInReferenceSpace.orientation.w = 1.0f;
        //space_info.poseInReferenceSpace.orientation.y = -0.51f;
        //space_info.poseInReferenceSpace.orientation.x = 0.0f;



        if (space_stage_enabled) {
            space_info.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_STAGE;
            space_info.poseInReferenceSpace.position.y = 0.0f;
            OXR(xrCreateReferenceSpace(xr_session,
                                       &space_info,
                                       &xr_reference_space));
        } else {
            // Use a fake stage space
            space_info.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
            space_info.poseInReferenceSpace.position.y = -1.6750f;
            OXR(xrCreateReferenceSpace(xr_session,
                                       &space_info,
                                       &xr_reference_space));
        }

    }

    void init(sOpenXRFramebuffer *framebuffers) {
        curr_framebuffers = framebuffers;
        // Load extensions ==============================================================
        _load_instance();

        // Create EGL context ========================================================
        {
            PFN_xrGetOpenGLESGraphicsRequirementsKHR pfnGetOpenGLESGraphicsRequirementsKHR = NULL;
            OXR(xrGetInstanceProcAddr(xr_instance,
                                      "xrGetOpenGLESGraphicsRequirementsKHR",
                                      (PFN_xrVoidFunction *) (&pfnGetOpenGLESGraphicsRequirementsKHR)));

            XrGraphicsRequirementsOpenGLESKHR graphics_requirements = {
                    .type = XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_ES_KHR,
                    .next = NULL
            };
            pfnGetOpenGLESGraphicsRequirementsKHR(xr_instance,
                                                  xr_sys_id,
                                                  &graphics_requirements);
            egl.create();
        }

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
            for (uint16_t i = 0; i < MAX_EYE_NUMBER; i++) { // To fill on the realtime
                memset(&eye_projections[i],
                       0,
                       sizeof(XrView));
                eye_projections[i].type = XR_TYPE_VIEW;
                eye_projections[i].next = NULL;
            }
        }

        // TODO CONFIG ACTION SPACES

        // Create SWAPCHAIN ===========================================================
        framebuffers[0].init(xr_session,
                             view_configs[0].recommendedImageRectWidth,
                             view_configs[0].recommendedImageRectHeight,
                             GL_SRGB8_ALPHA8);
        framebuffers[1].init(xr_session,
                             view_configs[0].recommendedImageRectWidth,
                             view_configs[0].recommendedImageRectHeight,
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
                                      (PFN_xrVoidFunction *) (&pfnPerfSettingsSetPerformanceLevelEXT)));

            // Set the CPU performance profile
            const Device::eCPUGPUPower cpu_power_target = Device::LVL_2_HIGH_POWER;
            OXR(pfnPerfSettingsSetPerformanceLevelEXT(xr_session,
                                                      XR_PERF_SETTINGS_DOMAIN_CPU_EXT,
                                                      (XrPerfSettingsLevelEXT) cpu_power_target));
            // Set the GPU performance profile
            const Device::eCPUGPUPower gpu_power_target = Device::LVL_3_BOOST;
            OXR(pfnPerfSettingsSetPerformanceLevelEXT(xr_session,
                                                      XR_PERF_SETTINGS_DOMAIN_GPU_EXT,
                                                      (XrPerfSettingsLevelEXT) gpu_power_target));

            // Set the main thread and the render thread as high priority threads
            PFN_xrSetAndroidApplicationThreadKHR pfnSetAndroidApplicationThreadKHR = NULL;
            OXR(xrGetInstanceProcAddr(xr_instance,
                                      "xrSetAndroidApplicationThreadKHR",
                                      (PFN_xrVoidFunction *) (&pfnSetAndroidApplicationThreadKHR)));

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
        for (;;) {
            XrEventDataBaseHeader *base_header = (XrEventDataBaseHeader *) &xr_event_buffer;
            *base_header = {XR_TYPE_EVENT_DATA_BUFFER};

            XrResult res = xrPollEvent(xr_instance,
                                       &xr_event_buffer);

            if (res != XR_SUCCESS) {
                break;
            }

            const XrEventDataSessionStateChanged *session_state_changed_event = (XrEventDataSessionStateChanged *) base_header;

            ALOGE("OpenXR event type:%i, but we want %i\n", base_header->type,
                  XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED);
            switch (base_header->type) {
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
                    switch (session_state_changed_event->state) {
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
                default:
                    break;
            }
        }
    }


    void update(Application::sAndroidState *app_state,
                double *delta_time,
                sFrameTransforms *transforms) {
        // Handle XR events
        handle_events(app_state);

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
        /*OXR(xrLocateSpace(xr_reference_space,
                          xr_local_space,
                          frame_state.predictedDisplayTime,
                          &space_location));*/
        //XrPosef pose_stage_from_head = space_location.pose;

        // Get Projection ===
        XrViewLocateInfo projection_info = {
                .type = XR_TYPE_VIEW_LOCATE_INFO,
                .next = NULL,
                .viewConfigurationType = view_config_prop.viewConfigurationType,
                .displayTime = frame_state.predictedDisplayTime,
                .space = xr_reference_space
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
        ALOGE("VIEW COUNT %i", projection_capacity);

        // Generate view projections
        for (int eye = 0; eye < MAX_EYE_NUMBER; eye++) {
            glm::mat4x4 view_eye = {}, view_stage = {};
            OpenXRHelpers::pose_to_glm_mat(eye_projections[eye].pose,
                                           &view_eye);

            OpenXRHelpers::pose_to_glm_mat(space_location.pose,
                                           &view_stage);

            const XrFovf fov = eye_projections[eye].fov;
            OpenXRHelpers::create_glm_projection(fov,
                                                 0.01f,
                                                 500.0f,
                                                 &transforms->projection[eye]);

            //view_eye = glm::mat4x4(glm::vec4{0.56224364, 0.0354339331, 0.826212465, 0.0},
            //                        glm::vec4{0.458378375, 0.818209171, -0.347020268, 0.0},
            //                        glm::vec4{-0.688310921, 0.573827684, 0.443790317, 0.0},
            //                        glm::vec4{0.281410813, 1.52490151, 0.0160850752, 1.0});
            //transforms->view[eye] = glm::inverse(view_eye);
            //
             transforms->view[eye] = glm::inverse(TestPerspectives::close_view);

            transforms->viewprojection[eye] = (transforms->projection[eye] * transforms->view[eye]);
            // https://github.com/maluoi/OpenXRSamples/blob/master/SingleFileExample/main.cpp

        }
    }

    void submit_frame() {
        // Generate layers
        layers_count = 0;
        // Projection layer correction
        projection_correction = {
                .type = XR_TYPE_COMPOSITION_LAYER_IMAGE_LAYOUT_FB,
                .next = NULL,
                .flags = XR_COMPOSITION_LAYER_IMAGE_LAYOUT_VERTICAL_FLIP_BIT_FB
        };
        projection_layer = {
                .type = XR_TYPE_COMPOSITION_LAYER_PROJECTION,
                .next = &projection_correction,
                .layerFlags = XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT |
                              XR_COMPOSITION_LAYER_CORRECT_CHROMATIC_ABERRATION_BIT,
                .space =  xr_reference_space,
                .viewCount = MAX_EYE_NUMBER,
                .views = projection_views
        };

        // Config projection layers
        for (uint16_t i = 0; i < MAX_EYE_NUMBER; i++) {
            memset(&projection_views[i],
                   0,
                   sizeof(XrCompositionLayerProjectionView));

            projection_views[i] = {
                    .type = XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW,
                    .next = NULL,
                    .pose = eye_projections[i].pose,
                    .fov = eye_projections[i].fov,
                    .subImage = {
                            .swapchain = curr_framebuffers[i].swapchain_handle,
                            .imageRect = {
                                    .offset = {0, 0},
                                    .extent = {
                                            .width = (int32_t) curr_framebuffers[i].width,
                                            .height = (int32_t) curr_framebuffers[i].height
                                    }
                            },
                            .imageArrayIndex = 0
                    }
            };
        }

        // Add layers to the frame´s dispatch list
        layers[layers_count++] = (const XrCompositionLayerBaseHeader *) &projection_layer;

        XrFrameEndInfo frame_end_info = {
                .type = XR_TYPE_FRAME_END_INFO,
                .next = NULL,
                .displayTime = frame_state.predictedDisplayTime,
                .environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE,
                .layerCount = layers_count,
                .layers = layers,
        };


        global_xr_instance = &xr_instance;
        OXR(xrEndFrame(xr_session,
                       &frame_end_info));
    }
};

#endif //__OPENXR_INSTANCE_H__