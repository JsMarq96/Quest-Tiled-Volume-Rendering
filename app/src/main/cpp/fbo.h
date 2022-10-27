#ifndef FBO_H_
#define FBO_H_
#include <cstdint>
#ifdef __EMSCRIPTEN__
#include <webgl/webgl2.h>
#include <GLES3/gl3.h>
#else
#include <GL/gl3w.h>
#endif

#include "texture.h"

enum eFBOAttachmentUse : uint32_t {
    JUST_COLOR = 0,
    JUST_DUAL_COLOR,
    JUST_DEPTH,
    COLOR_AND_DEPTH
};

struct sFBO {
    uint32_t id = 0;
    uint32_t width = 0;
    uint32_t height = 0;

    eFBOAttachmentUse attachment_use;

    uint8_t color_attachment0;
    uint8_t color_attachment1;
    uint8_t depth_attachment;

    void bind() {
        glBindFramebuffer(GL_FRAMEBUFFER, id);
    }

    void unbind() {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
};

#endif // FBO_H_
