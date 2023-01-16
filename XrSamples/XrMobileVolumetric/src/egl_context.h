//
// Created by js on 7/11/22.
//

#ifndef __EGL_CONTEXT_H__
#define __EGL_CONTEXT_H__

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>
#include <android/log.h>
#include <android/window.h>
#include <cstring>

typedef void (GL_APIENTRYP PFNGLGENQUERIESEXTPROC) (GLsizei n, GLuint *ids);
typedef void (GL_APIENTRYP PFNGLDELETEQUERIESEXTPROC) (GLsizei n, const GLuint *ids);
typedef GLboolean (GL_APIENTRYP PFNGLISQUERYEXTPROC) (GLuint id);
typedef void (GL_APIENTRYP PFNGLBEGINQUERYEXTPROC) (GLenum target, GLuint id);
typedef void (GL_APIENTRYP PFNGLENDQUERYEXTPROC) (GLenum target);
typedef void (GL_APIENTRYP PFNGLQUERYCOUNTEREXTPROC) (GLuint id, GLenum target);
typedef void (GL_APIENTRYP PFNGLGETQUERYIVEXTPROC) (GLenum target, GLenum pname, GLint *params);
typedef void (GL_APIENTRYP PFNGLGETQUERYOBJECTIVEXTPROC) (GLuint id, GLenum pname, GLint *params);
typedef void (GL_APIENTRYP PFNGLGETQUERYOBJECTUIVEXTPROC) (GLuint id, GLenum pname, GLuint *params);
typedef void (GL_APIENTRYP PFNGLGETQUERYOBJECTI64VEXTPROC) (GLuint id, GLenum pname, GLint64 *params);
typedef void (GL_APIENTRYP PFNGLGETQUERYOBJECTUI64VEXTPROC) (GLuint id, GLenum pname, GLuint64 *params);
typedef void (GL_APIENTRYP PFNGLGETINTEGER64VPROC) (GLenum pname, GLint64 *params);

extern PFNGLGENQUERIESEXTPROC glGenQueriesEXT_;
extern PFNGLDELETEQUERIESEXTPROC glDeleteQueriesEXT_;
extern PFNGLISQUERYEXTPROC glIsQueryEXT_;
extern PFNGLBEGINQUERYEXTPROC glBeginQueryEXT_;
extern PFNGLENDQUERYEXTPROC glEndQueryEXT_;
extern PFNGLQUERYCOUNTEREXTPROC glQueryCounterEXT_;
extern PFNGLGETQUERYIVEXTPROC glGetQueryivEXT_;
extern PFNGLGETQUERYOBJECTIVEXTPROC glGetQueryObjectivEXT_;
extern PFNGLGETQUERYOBJECTUIVEXTPROC glGetQueryObjectuivEXT_;
extern PFNGLGETQUERYOBJECTI64VEXTPROC glGetQueryObjecti64vEXT_;
extern PFNGLGETQUERYOBJECTUI64VEXTPROC glGetQueryObjectui64vEXT_;
extern PFNGLGETINTEGER64VPROC glGetInteger64v_;

#define GL_TIME_ELAPSED_EXT   0x88BF
#define GL_QUERY_RESULT_EXT   0x8866
#define GL_TIMESTAMP_EXT      0x8E28
#define GL_GPU_DISJOINT_EXT   0x8FBB

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

        // Load extensions
        //const char* extensions = (const char*) glGetString(GL_EXTENSIONS);

        // Timer extension
        // TODO: this is horrible!! Bad practice!! But strstr is acting fucky, so...
        //if (strstr("GL_EXT_disjoint_timer_query",
        //           extensions) != NULL) {
            // https://github.com/wadetb/shellspace/blob/master/external/ovr_mobile_sdk_0.4.3.1/VRLib/jni/GlUtils.cpp
            glGenQueriesEXT_ = (PFNGLGENQUERIESEXTPROC)eglGetProcAddress("glGenQueriesEXT");
            glDeleteQueriesEXT_ = (PFNGLDELETEQUERIESEXTPROC)eglGetProcAddress("glDeleteQueriesEXT");
            glIsQueryEXT_ = (PFNGLISQUERYEXTPROC)eglGetProcAddress("glIsQueryEXT");
            glBeginQueryEXT_ = (PFNGLBEGINQUERYEXTPROC)eglGetProcAddress("glBeginQueryEXT");
            glEndQueryEXT_ = (PFNGLENDQUERYEXTPROC)eglGetProcAddress("glEndQueryEXT");
            glQueryCounterEXT_ = (PFNGLQUERYCOUNTEREXTPROC)eglGetProcAddress("glQueryCounterEXT");
            glGetQueryivEXT_ = (PFNGLGETQUERYIVEXTPROC)eglGetProcAddress("glGetQueryivEXT");
            glGetQueryObjectivEXT_ = (PFNGLGETQUERYOBJECTIVEXTPROC)eglGetProcAddress("glGetQueryObjectivEXT");
            glGetQueryObjectuivEXT_ = (PFNGLGETQUERYOBJECTUIVEXTPROC)eglGetProcAddress("glGetQueryObjectuivEXT");
            glGetQueryObjecti64vEXT_ = (PFNGLGETQUERYOBJECTI64VEXTPROC)eglGetProcAddress("glGetQueryObjecti64vEXT");
            glGetQueryObjectui64vEXT_  = (PFNGLGETQUERYOBJECTUI64VEXTPROC)eglGetProcAddress("glGetQueryObjectui64vEXT");
            glGetInteger64v_  = (PFNGLGETINTEGER64VPROC)eglGetProcAddress("glGetInteger64v");
        //}

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
