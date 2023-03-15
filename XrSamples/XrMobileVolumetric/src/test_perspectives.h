//
// Created by u137524 on 15/03/2023.
//

#ifndef OCULUSROOT_TEST_PERSPECTIVES_H
#define OCULUSROOT_TEST_PERSPECTIVES_H

#include <glm/glm.hpp>

namespace TestPerspectives {

    static const glm::mat4x4 close_view = glm::mat4x4(glm::vec4{0.99714291, 0.0743882507, -0.0131309833, 0.0},
                                                      glm::vec4{-0.00673236325, -0.0856226683, -0.99630481, 0.0},
                                                      glm::vec4{-0.0752376914, 0.993546664, -0.0848772525, 0.0},
                                                      glm::vec4{-0.0684748143, 0.858300209, -0.139122769, 1.0});

}
#endif //OCULUSROOT_TEST_PERSPECTIVES_H
