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

#include "egl_context.h"
#include "app_data.h"

struct sFramebuffer {
    uint32_t width = 0;
    uint32_t height = 0;
    // multisamples

    // SwapChain
    uint32_t swapchain_length = 0;
    uint32_t swapchain_index = 0;

    // Color swapchain
    XrSwapchain swapchain_handle;
    uint32_t swapchain_width = 0;
    uint32_t swapchain_height = 0;

    XrSwapchainImageOpenGLESKHR *swapchain_images = NULL;

    uint32_t depth_buffers;
    uint32_t color_buffers;

    void init(XrSession &session,
              const uint32_t i_width,
              const uint32_t i_height,
              const GLenum   color_format){
        width = i_width;
        height = i_height;

        // Skip format verification

        XrSwapchainCreateInfo swapchain_create = {
                .type = XR_TYPE_SWAPCHAIN_CREATE_INFO,
                .usageFlags = XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT | XR_SWAPCHAIN_USAGE_SAMPLED_BIT,
                .format = color_format,
                .sampleCount = 1,
                .width = width,
                .height = height,
                .faceCount = 1, // ??
                .arraySize = 1,
                .mipCount = 1
        };

        // TODO create FOVeationg
        xrCreateSwapchain(session,
                          &swapchain_create,
                          &swapchain_handle);

        // Generate the images for the swapchain
        // The number
        xrEnumerateSwapchainImages(swapchain_handle,
                                   0,
                                   &swapchain_length,
                                   NULL);
        // Allocate
        swapchain_images = (XrSwapchainImageOpenGLESKHR*) malloc(sizeof(XrSwapchainImageOpenGLESKHR) * swapchain_length);
        // Fill
        for(uint32_t i = 0; i < swapchain_length; i++) {
            swapchain_images[i].type = XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_ES_KHR;
            swapchain_images[i].next = NULL;
        }
        xrEnumerateSwapchainImages(swapchain_handle,
                                   swapchain_length,
                                   &swapchain_length,
                                   (XrSwapchainImageBaseHeader*) swapchain_images);

        // Init textures and FBOs??
    }
};

struct sOpenXR_Instance {
    // OpenXR session & context data
    XrInstance xr_instance;
    XrSystemId xr_sys_id;
    XrSession xr_session;
    XrSpace xr_stage_space, xr_local_space;
    sEglContext egl;

    XrViewConfigurationView view_configs[MAX_EYE_NUMBER];
    XrView eye_projections[MAX_EYE_NUMBER];


    void init(sFramebuffer *framebuffer) {
        // Load extensions ==============================================================
        uint32_t extension_count = 9;
        const char *enabled_extensions[12] = {
                XR_KHR_OPENGL_ES_ENABLE_EXTENSION_NAME,
                XR_EXT_PERFORMANCE_SETTINGS_EXTENSION_NAME,
                XR_KHR_ANDROID_THREAD_SETTINGS_EXTENSION_NAME,
                XR_FB_DISPLAY_REFRESH_RATE_EXTENSION_NAME,
                XR_FB_COLOR_SPACE_EXTENSION_NAME,
                XR_FB_SWAPCHAIN_UPDATE_STATE_EXTENSION_NAME,
                XR_FB_SWAPCHAIN_UPDATE_STATE_OPENGL_ES_EXTENSION_NAME,
                XR_FB_FOVEATION_EXTENSION_NAME,
                XR_FB_FOVEATION_CONFIGURATION_EXTENSION_NAME
        };
        XrInstanceCreateInfo instance_info = {
                .type = XR_TYPE_INSTANCE_CREATE_INFO,
                .next = NULL, // To fill
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

        assert(xrCreateInstance(&instance_info,
                                &xr_instance) == XR_SUCCESS && "Error loading OpenXR Instance");

        XrSystemGetInfo system_info = {
                .type = XR_TYPE_SYSTEM_GET_INFO,
                .next = NULL,
                .formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY
        };

        assert(xrGetSystem(xr_instance,
                           &system_info,
                           &xr_sys_id) == XR_SUCCESS && "Error getting system");

        // Create EGL context ========================================================
        egl.create();

        // Create the OpenXR session ===================================================
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
                .createFlags = 0,
                .systemId = xr_sys_id
        };

        assert(xrCreateSession(xr_instance,
                               &session_info,
                               &xr_session) == XR_SUCCESS && "Failed to create session");

        // Load ViewPort data ===============================================================
        const XrViewConfigurationType stereo_view_config = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
        uint32_t viewport_count = 0;
        xrEnumerateViewConfigurations(xr_instance,
                                      xr_sys_id,
                                      0,
                                      &viewport_count,
                                      NULL);

        assert(viewport_count > 0 && "No enought viewport configs on device");

        XrViewConfigurationType *viewport_configs = (XrViewConfigurationType*) malloc(sizeof(XrViewConfigurationType) * viewport_count);
        xrEnumerateViewConfigurations(xr_instance,
                                      xr_sys_id,
                                      viewport_count,
                                      &viewport_count,
                                      viewport_configs);

        for(uint32_t i = 0; i < viewport_count; i++) {
            const XrViewConfigurationType vp_config_type = viewport_configs[i];

            // Only interested in stereo rendering
            if (vp_config_type != stereo_view_config) {
                continue;
            }

            uint32_t view_count = 0;
            xrEnumerateViewConfigurationViews(xr_instance,
                                              xr_sys_id,
                                              vp_config_type,
                                              0,
                                              &view_count,
                                              NULL);
            if (view_count <= 0) {
                continue; // Empty viewport
            }

            assert(view_count == MAX_EYE_NUMBER && "Only need two eyes on stereo rendering!");

            XrViewConfigurationView *views = (XrViewConfigurationView*) malloc(sizeof(XrViewConfigurationView) * view_count);
            for(uint32_t j = 0; j < view_count; j++) {
                views[j].type = XR_TYPE_VIEW_CONFIGURATION_VIEW;
                views[j].next = NULL;
            }

            xrEnumerateViewConfigurationViews(xr_instance,
                                              xr_sys_id,
                                              vp_config_type,
                                              view_count,
                                              &view_count,
                                              views);

            view_configs[0] = views[0];
            view_configs[1] = views[1];
        }


        // Color space???

        //Refresh rate

        // List reference spaces NOTE: trhis probably can be skipped also ==================
        uint32_t output_spaces_count = 0;
        xrEnumerateReferenceSpaces(xr_session,
                                   0, &output_spaces_count,
                                   NULL);

        XrReferenceSpaceType *reference_spaces = (XrReferenceSpaceType *) malloc(
                sizeof(XrReferenceSpaceType) * output_spaces_count);

        xrEnumerateReferenceSpaces(xr_session,
                                   output_spaces_count,
                                   &output_spaces_count,
                                   reference_spaces);

        for (uint32_t i = 0; i < 0; i++) {
            if (reference_spaces[i] == XR_REFERENCE_SPACE_TYPE_STAGE) {
                break; // Supported
            }
        }

        free(reference_spaces);

        // Create the reference spaces for the rendering
        XrReferenceSpaceCreateInfo space_info = {
                .type = XR_TYPE_REFERENCE_SPACE_CREATE_INFO,
                .next = NULL,
                .referenceSpaceType = XR_REFERENCE_SPACE_TYPE_STAGE,
        };
        space_info.poseInReferenceSpace.orientation.w = 0.0f;

        xrCreateReferenceSpace(xr_session,
                               &space_info,
                               &xr_stage_space);

        space_info.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
        space_info.poseInReferenceSpace.orientation.w = 1.0f; // ?
        xrCreateReferenceSpace(xr_session,
                               &space_info,
                               &xr_local_space);

        // CONFIG VIEWS ================================================================
        for(uint16_t i = 0; i < MAX_EYE_NUMBER; i++) { // To fill on the realtime
            memset(&eye_projections[i],
                   0,
                   sizeof(XrView));
            eye_projections[i].type = XR_TYPE_VIEW;
        }

        // CONFIG ACTION SPACES

        // Create SWAPCHAIN ===========================================================

        framebuffer->init(xr_session,
                          view_configs[0].maxImageRectWidth,
                          view_configs[0].maxImageRectHeight,
                          GL_SRGB8_ALPHA8);

        // TODO: Foveation
    }
};

#endif //__OPENXR_INSTANCE_H__
