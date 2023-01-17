//
// Created by u137524 on 17/01/2023.
//

#ifndef OCULUSROOT_COMPUTE_SHADER_H
#define OCULUSROOT_COMPUTE_SHADER_H

#include "shader.h"

struct sComputeShader : sShader {
    sComputeShader() {};

    void load_shader(const char* raw_compute);
};

#endif //OCULUSROOT_COMPUTE_SHADER_H
