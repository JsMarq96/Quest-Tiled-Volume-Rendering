//
// Created by js on 19/11/22.
//

#ifndef __RBO_H__
#define __RBO_H__
#include <cstdint>
#include <GLES3/gl3.h>

// Use as a FBO but without need for textures
// More optimal for mobile
struct sRBO {
    uint32_t id = 0;
    uint32_t width = 0;
    uint32_t height = 0;

    uint32_t attachment_use = 0;
    uint32_t format = 0;

    inline void bind(const uint32_t buffer_type) const {
        glFramebufferRenderbuffer(buffer_type,
                                  attachment_use,
                                  GL_RENDERBUFFER,
                                  id);
    }

    inline void unbind() const {
        glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER,
                                  attachment_use,
                                  GL_RENDERBUFFER,
                                  0);
    }
};

#endif
