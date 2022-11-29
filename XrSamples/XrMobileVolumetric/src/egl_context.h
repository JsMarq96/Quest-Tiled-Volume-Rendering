//
// Created by js on 7/11/22.
//

#ifndef __EGL_CONTEXT_H__
#define __EGL_CONTEXT_H__

#include <EGL/egl.h>
#include <EGL/eglext.h>

struct sEglContext {
    EGLDisplay display;
    EGLConfig config;
    EGLContext context;
    EGLSurface surface;
    EGLint major_version;
    EGLint minor_version;

    void create() {
        //info("get EGL display");
        display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
        if (display == EGL_NO_DISPLAY) {
            //error("can't get EGL display: %s", egl_get_error_string(eglGetError()));
            exit(EXIT_FAILURE);
        }

        //info("initialize EGL display");
        if (eglInitialize(display,
                          &major_version,
                          &minor_version) == EGL_FALSE) {
            //error("can't initialize EGL display: %s", egl_get_error_string(eglGetError()));
            exit(EXIT_FAILURE);
        }

        const int MAX_CONFIG = 1024;
        EGLConfig configs[MAX_CONFIG];
        //info("get number of EGL configs");
        EGLint num_configs = 0;
        if (eglGetConfigs(display,
                          configs,
                          MAX_CONFIG,
                          &num_configs) == EGL_FALSE) {
            //error("can't get number of EGL configs: %s",egl_get_error_string(eglGetError()));
            exit(EXIT_FAILURE);
        }

        //info("choose EGL config");
        static const EGLint CONFIG_ATTRIBS[] = {
                EGL_RED_SIZE, 8,
                EGL_GREEN_SIZE, 8,
                EGL_BLUE_SIZE, 8,
                EGL_ALPHA_SIZE, 8, // need alpha for timewarp
                EGL_DEPTH_SIZE, 0,
                EGL_STENCIL_SIZE, 0,
                EGL_SAMPLES, 0,
                EGL_NONE,
        };
        for (int i = 0; i < num_configs; ++i) {
            EGLConfig it_config = configs[i];

            //info("get EGL config renderable type");
            EGLint renderable_type = 0;

            if (eglGetConfigAttrib(display,
                                   it_config,
                                   EGL_RENDERABLE_TYPE,
                                   &renderable_type) == EGL_FALSE) {
                //error("can't get EGL config renderable type: %s", egl_get_error_string(eglGetError()));
                exit(EXIT_FAILURE);
            }
            if ((renderable_type & EGL_OPENGL_ES3_BIT_KHR) == 0) {
                continue;
            }

            //info("get EGL config surface type");
            EGLint surface_type = 0;
            if (eglGetConfigAttrib(display,
                                   it_config,
                                   EGL_SURFACE_TYPE,
                                   &surface_type) == EGL_FALSE) {
                //error("can't get EGL config surface type: %s", egl_get_error_string(eglGetError()));
                exit(EXIT_FAILURE);
            }
            if ((renderable_type & EGL_PBUFFER_BIT) == 0) {
                continue;
            }
            if ((renderable_type & EGL_WINDOW_BIT) == 0) {
                continue;
            }

            const EGLint *attrib = CONFIG_ATTRIBS;
            while (attrib[0] != EGL_NONE) {
                //info("get EGL config attrib");
                EGLint value = 0;
                if (eglGetConfigAttrib(display,
                                       it_config,
                                       attrib[0],
                                       &value) == EGL_FALSE) {
                    //error("can't get EGL config attrib: %s", egl_get_error_string(eglGetError()));
                    exit(EXIT_FAILURE);
                }
                if (value != attrib[1]) {
                    break;
                }
                attrib += 2;
            }
            if (attrib[0] != EGL_NONE) {
                continue;
            }

            config = it_config;
            break;
        }

        if (config == NULL) {
            //error("can't choose EGL config");
            exit(EXIT_FAILURE);
        }


        //info("create EGL context");
        static const EGLint CONTEXT_ATTRIBS[] = {EGL_CONTEXT_CLIENT_VERSION, 3,
                                                 EGL_NONE};
        context = eglCreateContext(display,
                                   config,
                                   EGL_NO_CONTEXT,
                                   CONTEXT_ATTRIBS);
        if (context == EGL_NO_CONTEXT) {
            //error("can't create EGL context: %s", egl_get_error_string(eglGetError()));
            exit(EXIT_FAILURE);
        }

        //info("create EGL surface");
        static const EGLint SURFACE_ATTRIBS[] = {
                EGL_WIDTH, 16,
                EGL_HEIGHT, 16,
                EGL_NONE,
        };
        surface = eglCreatePbufferSurface(display,
                                          config,
                                          SURFACE_ATTRIBS);
        if (surface == EGL_NO_SURFACE) {
            //error("can't create EGL pixel buffer surface: %s", egl_get_error_string(eglGetError()));
            exit(EXIT_FAILURE);
        }

        //info("make EGL context current");
        if (eglMakeCurrent(display,
                           surface,
                           surface,
                           context) == EGL_FALSE) {
            //error("can't make EGL context current: %s", egl_get_error_string(eglGetError()));
        }
    }

    void destroy() {
        //info("make EGL context no longer current");
        eglMakeCurrent(display,
                       EGL_NO_SURFACE,
                       EGL_NO_SURFACE,
                       EGL_NO_CONTEXT);

        //info("destroy EGL surface");
        eglDestroySurface(display,
                          surface);

        //info("destroy EGL context");
        eglDestroyContext(display,
                          context);

        //info("terminate EGL display");
        eglTerminate(display);
    }
};

#endif //__EGL_CONTEXT_H__
