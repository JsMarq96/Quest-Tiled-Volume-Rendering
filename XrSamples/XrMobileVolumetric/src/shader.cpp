#include "shader.h"

#ifndef __EMSCRIPTEN__

#include <GLES3/gl3.h>
#include <android/log.h>
#else
#include <webgl/webgl2.h>
#endif

#include <iostream>
#include <errno.h>
#include <cstring>
#include <glm/gtc/type_ptr.hpp>

// To do: Show programming errors
sShader::sShader(const char* vertex_shader_raw,
               const char* fragment_shader_raw) {
    int vertex_id, fragment_id;
    int compile_successs;
    char compile_log[512];

    is_compute = false;

    // Create and compile the shaders
    vertex_id = glCreateShader(GL_VERTEX_SHADER);

    glShaderSource(vertex_id, 1, &vertex_shader_raw, NULL);
    glCompileShader(vertex_id);
    glGetShaderiv(vertex_id, GL_COMPILE_STATUS, &compile_successs);

    if (!compile_successs) {
        glGetShaderInfoLog(vertex_id, 512, NULL, compile_log);
        __android_log_print(ANDROID_LOG_ERROR,
                            "Shader",
                            "%s", compile_log);
        assert(">>>>>Error comiling vertex shader" && false);
    }

    fragment_id = glCreateShader(GL_FRAGMENT_SHADER);

    glShaderSource(fragment_id, 1, &fragment_shader_raw, NULL);
    glCompileShader(fragment_id);
    glGetShaderiv(fragment_id, GL_COMPILE_STATUS, &compile_successs);

    if (!compile_successs) {
        glGetShaderInfoLog(fragment_id, 512, NULL, compile_log);
        __android_log_print(ANDROID_LOG_ERROR,
                            "Shader",
                            "%s", compile_log);
        assert(">>>>>Error comiling fragment shader" && false);
    }

    // Create the shader program
    ID = glCreateProgram();
    glAttachShader(ID, vertex_id);
    glAttachShader(ID, fragment_id);
    glLinkProgram(ID);
    glGetProgramiv(ID, GL_LINK_STATUS, &compile_successs);

    if (!compile_successs) {
        glGetProgramInfoLog(ID, 512, NULL, compile_log);
        __android_log_print(ANDROID_LOG_ERROR,
                            "Shader",
                            "%s", compile_log);
        assert(">>>>>Shader Linking error" && false);
    }

    // Cleanup
    glDeleteShader(vertex_id);
    glDeleteShader(fragment_id);
}

void sShader::load_file_graphic_shaders(const char*     v_shader_dir,
                                        const char*     f_shader_dir) {
    FILE *vert_file, *frag_file;
    int vert_size, frag_size;
    char* raw_vert_shader, *raw_frag_shader;

    vert_file = fopen(v_shader_dir, "r");
    frag_file = fopen(f_shader_dir, "r");


    assert(vert_file != NULL && "Failed to open file vertex shader on gameobject"); // Load the vertex shader
    assert(frag_file != NULL && "Failed to open file fragment shader on gameobject");

    // Get sizes of the raw shader code
    fseek(vert_file, 0L, SEEK_END);
    fseek(frag_file, 0L, SEEK_END);
    vert_size = ftell(vert_file);
    frag_size = ftell(frag_file);

    rewind(vert_file);
    rewind(frag_file);

    raw_vert_shader = (char*) malloc((vert_size) + 1);
    raw_frag_shader = (char*) malloc((frag_size) + 1);

    memset(raw_vert_shader, '\0', vert_size + 1);
    memset(raw_frag_shader, '\0', frag_size + 1);

    // read the raw file
    fread(raw_vert_shader, 1, vert_size, vert_file);
    fread(raw_frag_shader, 1, frag_size, frag_file);

    fclose(vert_file);
    fclose(frag_file);

    load_graphic_shaders(raw_vert_shader, raw_frag_shader);

    free(raw_vert_shader);
    free(raw_frag_shader);
}

void sShader::load_graphic_shaders(const char*   vertex_shader_raw,
                                   const char*   fragment_shader_raw) {
    int vertex_id, fragment_id;
    int compile_successs;
    char compile_log[512];

    is_compute = false;

    // Create and compile the shaders
    vertex_id = glCreateShader(GL_VERTEX_SHADER);

    glShaderSource(vertex_id, 1, &vertex_shader_raw, NULL);
    glCompileShader(vertex_id);
    glGetShaderiv(vertex_id, GL_COMPILE_STATUS, &compile_successs);

    if (!compile_successs) {
        glGetShaderInfoLog(vertex_id, 512, NULL, compile_log);
        __android_log_print(ANDROID_LOG_ERROR,
                            "Shader",
                            "%s", compile_log);
        assert(">>>>>Error comiling vertex shader" && false);
    }

    fragment_id = glCreateShader(GL_FRAGMENT_SHADER);

    glShaderSource(fragment_id, 1, &fragment_shader_raw, NULL);
    glCompileShader(fragment_id);
    glGetShaderiv(fragment_id, GL_COMPILE_STATUS, &compile_successs);

    if (!compile_successs) {
        glGetShaderInfoLog(fragment_id, 512, NULL, compile_log);
        __android_log_print(ANDROID_LOG_ERROR,
                            "Shader",
                            "%s", compile_log);
        assert(">>>>>Error comiling fragment shader" && false);
    }

    // Create the shader program
    ID = glCreateProgram();
    glAttachShader(ID, vertex_id);
    glAttachShader(ID, fragment_id);
    glLinkProgram(ID);
    glGetProgramiv(ID, GL_LINK_STATUS, &compile_successs);

    if (!compile_successs) {
        glGetProgramInfoLog(ID, 512, NULL, compile_log);
        __android_log_print(ANDROID_LOG_ERROR,
                            "Shader",
                            "%s", compile_log);
        assert(">>>>>Shader Linking error" && false);
    }

    // Cleanup
    glDeleteShader(vertex_id);
    glDeleteShader(fragment_id);
};

void sShader::load_compute_shader(const char* raw_compute) {
    int compile_successs;
    char compile_log[512];

    is_compute = true;

    // Create and compile the shaders
    uint32_t compute_id = glCreateShader(GL_COMPUTE_SHADER);

    glShaderSource(compute_id,
                   1,
                   &raw_compute,
                   NULL);
    glCompileShader(compute_id);
    glGetShaderiv(compute_id,
                  GL_COMPILE_STATUS,
                  &compile_successs);

    if (!compile_successs) {
        glGetShaderInfoLog(compute_id,
                           512,
                           NULL,
                           compile_log);
        __android_log_print(ANDROID_LOG_ERROR,
                            "Compute Shader compilation error",
                            "%s", compile_log);
        assert(">>>>>Error comiling vertex shader" && false);
    }

    ID = glCreateProgram();
    glLinkProgram(ID);
    glGetProgramiv(ID,
                   GL_LINK_STATUS,
                   &compile_successs);

    if (!compile_successs) {
        glGetProgramInfoLog(ID,
                            512,
                            NULL,
                            compile_log);
        __android_log_print(ANDROID_LOG_ERROR,
                            "Compute Shader linking error",
                            "%s", compile_log);
        assert(">>>>>Shader Linking error" && false);
    }

    // Cleanup
    glDeleteShader(compute_id);
}



void sShader::activate() const {
    glUseProgram(ID);
}

void sShader::deactivate() const {
    glUseProgram(0);
}

void sShader::dispatch(const uint32_t dispatch_x,
                       const uint32_t dispatch_y,
                       const uint32_t dispatch_z) const {
    if (!is_compute) {
        return;
    }

    glDispatchCompute(dispatch_x,
                      dispatch_y,
                      dispatch_z);
}

void sShader::set_uniform(const char* name,
                        const float value) const {
    glUniform1f(glGetUniformLocation(ID, name), value);

}
void sShader::set_uniform(const char* name,
                        const int value) const {
    glUniform1i(glGetUniformLocation(ID, name), value);
}

void sShader::set_uniform(const char* name,
                        const bool value) const {
    glUniform1i(glGetUniformLocation(ID, name), (int) value);
}

void sShader::set_uniform_vector2D(const char*     name,
                                   const float     value[2]) const {
    glUniform4fv(glGetUniformLocation(ID, name), 1, value);
}

void sShader::set_uniform_vector(const char* name,
                        const float value[4]) const {
    glUniform4fv(glGetUniformLocation(ID, name), 1, value);
}

void sShader::set_uniform_vector(const char* name, const glm::vec4 &value) const {
    glUniform4fv(glGetUniformLocation(ID, name), 1, &value.x);
}

void sShader::set_uniform_vector(const char* name, const glm::vec3 &value) const {
    glUniform3fv(glGetUniformLocation(ID, name), 1, &value.x);
}


void sShader::set_uniform_matrix3(const char* name,
                                  const glm::mat3x3 &matrix) const {
    glUniformMatrix3fv(glGetUniformLocation(ID, name), 1, false, (float*) &matrix);
}

void sShader::set_uniform_matrix4(const char* name, const glm::mat4x4 &matrix) const {
    glUniformMatrix4fv(glGetUniformLocation(ID, name), 1, false, glm::value_ptr(matrix));
}

void sShader::set_uniform_matrix4(const char* name, const float* matrix) const {
    glUniformMatrix4fv(glGetUniformLocation(ID, name), 1, false, matrix);
}

#include <iostream>
void sShader::set_uniform_texture(const char* name,
                                  const int tex_spot) const {
    glUniform1i(glGetUniformLocation(ID, name), tex_spot);
}
