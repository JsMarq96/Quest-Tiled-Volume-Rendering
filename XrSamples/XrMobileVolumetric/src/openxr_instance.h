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

#include "egl_context.h"
#include "app_data.h"
#include "application.h"
#include "device.h"

struct sFrameTransforms {
    XrMatrix4x4f view[MAX_EYE_NUMBER];
    XrMatrix4x4f projection[MAX_EYE_NUMBER];
    XrMatrix4x4f viewprojection[MAX_EYE_NUMBER];
} __attribute__((packed));

struct sOpenXRFramebuffer {
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
                .next = NULL,
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
        assert(xrCreateSwapchain(session,
                          &swapchain_create,
                          &swapchain_handle) == XR_SUCCESS);

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
        assert(xrEnumerateSwapchainImages(swapchain_handle,
                                   swapchain_length,
                                   &swapchain_length,
                                   (XrSwapchainImageBaseHeader*) swapchain_images) == XR_SUCCESS);
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
        xrAcquireSwapchainImage(swapchain_handle,
                                &swapchain_acq_info,
                                &swapchain_index);

        XrResult res = xrWaitSwapchainImage(swapchain_handle,
                                            &swapchain_wait_info);

        while(res == XR_TIMEOUT_EXPIRED) {
            res = xrWaitSwapchainImage(swapchain_handle,
                                       &swapchain_wait_info);
        }
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

    sOpenXRFramebuffer *curr_framebuffers;

    XrPosef viewTransform[MAX_EYE_NUMBER];

    XrViewConfigurationView view_configs[MAX_EYE_NUMBER];
    XrViewConfigurationProperties view_config_prop;
    XrView eye_projections[MAX_EYE_NUMBER];

    XrCompositionLayerProjectionView projection_views[MAX_EYE_NUMBER];
    XrCompositionLayerProjection projection_layer;


    void init(sOpenXRFramebuffer *framebuffers) {
        curr_framebuffers = framebuffers;
        // Load extensions ==============================================================
        {

            XrResult result;
            PFN_xrEnumerateInstanceExtensionProperties xrEnumerateInstanceExtensionProperties;
            (result = xrGetInstanceProcAddr(
                    XR_NULL_HANDLE,
                    "xrEnumerateInstanceExtensionProperties",
                    (PFN_xrVoidFunction*)&xrEnumerateInstanceExtensionProperties));
            if (result != XR_SUCCESS) {
                //ALOGE("Failed to get xrEnumerateInstanceExtensionProperties function pointer.");
                exit(1);
            }

            uint32_t numInputExtensions = 0;
            uint32_t numOutputExtensions = 0;
            (xrEnumerateInstanceExtensionProperties(
                    NULL, numInputExtensions, &numOutputExtensions, NULL));

            numInputExtensions = numOutputExtensions;

            XrExtensionProperties* extensionProperties =
                    (XrExtensionProperties*)malloc(numOutputExtensions * sizeof(XrExtensionProperties));

            for (uint32_t i = 0; i < numOutputExtensions; i++) {
                extensionProperties[i].type = XR_TYPE_EXTENSION_PROPERTIES;
                extensionProperties[i].next = NULL;
            }

            (xrEnumerateInstanceExtensionProperties(
                    NULL, numInputExtensions, &numOutputExtensions, extensionProperties));



            uint32_t extension_count = 9;
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
        }

        // Create EGL context ========================================================
        {
            PFN_xrGetOpenGLESGraphicsRequirementsKHR pfnGetOpenGLESGraphicsRequirementsKHR = NULL;
            xrGetInstanceProcAddr(xr_instance,
                                  "xrGetOpenGLESGraphicsRequirementsKHR",
                                  (PFN_xrVoidFunction*)(&pfnGetOpenGLESGraphicsRequirementsKHR));

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
        {
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
        }

        // Load ViewPort data ===============================================================
        const XrViewConfigurationType stereo_view_config = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
        uint32_t viewport_count = 0;
        {
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
        }

        // Load Viewport configuration =========
        {
            view_config_prop = {
                    .type = XR_TYPE_VIEW_CONFIGURATION_PROPERTIES,
                    .next = NULL
            };

            xrGetViewConfigurationProperties(xr_instance,
                                             xr_sys_id,
                                             stereo_view_config,
                                             &view_config_prop);

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

            space_info.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW;
            space_info.poseInReferenceSpace.orientation.w = 1.0f; // ?
            xrCreateReferenceSpace(xr_session,
                                   &space_info,
                                   &xr_head_space);
        }

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

        // CONFIG ACTION SPACES

        // Create SWAPCHAIN ===========================================================
        framebuffers[0].init(xr_session,
                             view_configs[0].maxImageRectWidth,
                             view_configs[0].maxImageRectHeight,
                             GL_SRGB8_ALPHA8);
        framebuffers[1].init(xr_session,
                             view_configs[0].maxImageRectWidth,
                             view_configs[0].maxImageRectHeight,
                             GL_SRGB8_ALPHA8);

        // TODO: Foveation

        // Composition layers: View projection layer =================================
        {
            projection_layer = {
                    .type = XR_TYPE_COMPOSITION_LAYER_PROJECTION,
                    .next = NULL,
                    .layerFlags = 0, //XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT | XR_COMPOSITION_LAYER_CORRECT_CHROMATIC_ABERRATION_BIT
                    .space = xr_head_space, // or xr_stage_space
                    .viewCount = viewport_count,
                    .views = projection_views
            };

            for(uint16_t i = 0; i < MAX_EYE_NUMBER; i++) {
                projection_views[i] = {
                        .type = XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW,
                        .next = NULL,
                        .subImage = {
                                .swapchain = framebuffers[i].swapchain_handle,
                                .imageRect = {
                                        .offset = {0,0},
                                        .extent = {
                                                .width = (int32_t) view_configs[0].recommendedImageRectWidth,
                                                .height = (int32_t) view_configs[0].recommendedImageRectHeight
                                        }
                                },
                                .imageArrayIndex = 0
                        }
                };
            }
        }
    }

    void session_change_state(const XrSessionState &state,
                              Application::sAndroidState *android_state) const {
        if (state == XR_SESSION_STATE_READY) {
            // Start the session!
            XrSessionBeginInfo session_start_info = {
                    .type = XR_TYPE_SESSION_BEGIN_INFO,
                    .next = NULL,
                    .primaryViewConfigurationType = view_config_prop.viewConfigurationType
            };

            assert(xrBeginSession(xr_session,
                                  &session_start_info) == XR_SUCCESS && "Error starting XR session");

            PFN_xrPerfSettingsSetPerformanceLevelEXT pfnPerfSettingsSetPerformanceLevelEXT = NULL;
            xrGetInstanceProcAddr(xr_instance,
                                  "xrPerfSettingsSetPerformanceLevelEXT",
                                  (PFN_xrVoidFunction*)(&pfnPerfSettingsSetPerformanceLevelEXT));

            // Set the CPU performance profile
            const Device::eCPUGPUPower cpu_power_target = Device::LVL_1_LOW_POWER;
            pfnPerfSettingsSetPerformanceLevelEXT(xr_session,
                                                  XR_PERF_SETTINGS_DOMAIN_CPU_EXT,
                                                  (XrPerfSettingsLevelEXT) cpu_power_target);
            // Set the GPU performance profile
            const Device::eCPUGPUPower gpu_power_target = Device::LVL_2_HIGH_POWER;
            pfnPerfSettingsSetPerformanceLevelEXT(xr_session,
                                                  XR_PERF_SETTINGS_DOMAIN_GPU_EXT,
                                                  (XrPerfSettingsLevelEXT) gpu_power_target);

            // Set the main thread and the render thread as high priority threads
            PFN_xrSetAndroidApplicationThreadKHR pfnSetAndroidApplicationThreadKHR = NULL;
            xrGetInstanceProcAddr(xr_instance,
                                  "xrSetAndroidApplicationThreadKHR",
                                  (PFN_xrVoidFunction*)(&pfnSetAndroidApplicationThreadKHR));

            assert(pfnSetAndroidApplicationThreadKHR(xr_session,
                                                     XR_ANDROID_THREAD_TYPE_APPLICATION_MAIN_KHR,
                                                     android_state->main_thread) == XR_SUCCESS);
            assert(pfnSetAndroidApplicationThreadKHR(xr_session,
                                                     XR_ANDROID_THREAD_TYPE_RENDERER_MAIN_KHR,
                                                     android_state->render_thread) == XR_SUCCESS);

            android_state->session_active = true;
        } else if (state == XR_SESSION_STATE_STOPPING) {
            xrEndSession(xr_session);
            android_state->session_active = false;
        }
    }

    void handle_events(Application::sAndroidState *app_state) {
        XrEventDataBuffer xr_event_buffer = {};

        for(;;) {
            XrEventDataBaseHeader *base_header = (XrEventDataBaseHeader*) &xr_event_buffer;
            base_header->type = XR_TYPE_EVENT_DATA_BUFFER;
            base_header->next = NULL;

            if (xrPollEvent(xr_instance,
                            &xr_event_buffer) != XR_SUCCESS ) {
                break;
            }

            const XrEventDataSessionStateChanged* session_state_changed_event = (XrEventDataSessionStateChanged*) base_header;

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
        handle_events(app_state);

        // get the predicted frametimes from OpenXR
        XrFrameWaitInfo waitFrameInfo = {};
        waitFrameInfo.type = XR_TYPE_FRAME_WAIT_INFO;
        waitFrameInfo.next = NULL;

        frame_state = {};
        frame_state.type = XR_TYPE_FRAME_STATE;
        frame_state.next = NULL;

        xrWaitFrame(xr_session,
                    &waitFrameInfo,
                    &frame_state);

        // Get the HMD pose, predicted for the middle of the time period during which
        // the new eye images will be displayed. The number of frames predicted ahead
        // depends on the pipeline depth of the engine and the synthesis rate.
        // The better the prediction, the less black will be pulled in at the edges.

        XrFrameBeginInfo begin_frame_desc = {
                .type = XR_TYPE_FRAME_BEGIN_INFO,
                .next = NULL
        };
        XrResult resu = xrBeginFrame(xr_session,
                                     &begin_frame_desc);
        assert(resu == XR_SUCCESS);

        // Get space tracking ===
        XrSpaceLocation space_location = {
                .type = XR_TYPE_SPACE_LOCATION,
                .next = NULL
        };
        xrLocateSpace(xr_stage_space,
                      xr_local_space,
                      frame_state.predictedDisplayTime,
                      &space_location);
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
        xrLocateViews(xr_session,
                      &projection_info,
                      &view_state,
                      projection_capacity,
                      &projection_capacity,
                      eye_projections);

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

        for(uint16_t eye = 0; eye < MAX_EYE_NUMBER; eye++) {
            projection_views[eye].pose = XrPosef_Inverse(viewTransform[eye]);
            projection_views[eye].fov = eye_projections[eye].fov;
        }
        // Composite layer
        const XrCompositionLayerBaseHeader *layers = {
                (const XrCompositionLayerBaseHeader* const) &projection_layer
        };

        XrFrameEndInfo frame_end_info = {
                .type = XR_TYPE_FRAME_END_INFO,
                .next = NULL,
                .displayTime = frame_state.predictedDisplayTime,
                .environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE,
                .layerCount = 1,
                .layers = &layers,
        };

        XrResult res = xrEndFrame(xr_session,
                                  &frame_end_info);

        //__android_log_print(ANDROID_LOG_DEBUG, "test", "%i", (int) res);
        assert(res == XR_SUCCESS && "Error submiting frame");
    }
};

#endif //__OPENXR_INSTANCE_H__
