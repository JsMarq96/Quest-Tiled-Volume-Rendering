#ifndef SHADER_H_
#define SHADER_H_

#ifndef __EMSCRIPTEN__
#include <GLES3/gl3.h>
#include <GLES3/gl32.h>
#else
#include <webgl/webgl2.h>
#endif
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/mat3x3.hpp>

#include <stddef.h>
#include <cassert>
#include <stdio.h>


/**
 * Basic OpenGL Shader Class, for shader's IO boilerplate
 * Juan S. Marquerie
*/
struct sShader {
    unsigned int ID;

    bool is_compute = false;

    sShader() {};
    sShader(const char* vertex_shader, const char* fragment_shader);

    void load_file_graphic_shaders(const char* v_shader_dir, const char* f_shader_dir);
    void load_graphic_shaders(const char* vertex_shader, const char* frag_shader_dir);
    void load_compute_shader(const char* raw_compute);

    void activate() const;
    void deactivate() const;

    void dispatch(const uint32_t dispatch_x,
                  const uint32_t dispatch_y,
                  const uint32_t dispatch_z) const;

    // Setters for the shader's uniforms
    void set_uniform(const char* name, const float value) const;
    void set_uniform(const char* name, const int value) const;
    void set_uniform(const char* name, const bool value) const;
    void set_uniform_vector2D(const char* name, const float value[2]) const;
    void set_uniform_vector(const char* name, const float value[4]) const;
    void set_uniform_vector(const char* name, const glm::vec4 &value) const;
    void set_uniform_vector(const char* name, const glm::vec3 &value) const;
    void set_uniform_matrix3(const char* name, const glm::mat3x3 &matrix) const;
    void set_uniform_matrix4(const char* name, const glm::mat4x4 &matrix) const;
    void set_uniform_matrix4(const char* name, const float* matrix) const;
    // Samplers / textures
    void set_uniform_texture(const char* name, const int tex_name) const;
};


#endif // SHADER_H_
