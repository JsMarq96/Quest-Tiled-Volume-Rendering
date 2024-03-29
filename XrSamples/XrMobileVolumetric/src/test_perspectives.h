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

    static const glm::mat4x4 near_view = glm::mat4x4(glm::vec4{0.9999246, 0.00484526437, -0.0112854596, 0.0},
                                                     glm::vec4{-0.012182829, 0.274996221, -0.961368083, 0.0},
                                                     glm::vec4{-0.00155462418, 0.961433053, 0.275034547, 0.0},
                                                     glm::vec4{-0.0431376211, 1.32213426, 0.0299457405, 1.0});

    static const glm::mat4x4 near_1_view = glm::mat4x4(glm::vec4{0.990894436, -0.134571373, -0.00433339924, 0.0},
                                                     glm::vec4{0.00288253278, 0.0533804893, -0.998570144, 0.0},
                                                     glm::vec4{0.134610265, 0.989465057, 0.0532823801, 0.0},
                                                     glm::vec4{-0.049074512, 1.07085085, 0.00871993881, 1.0});


    static const glm::mat4x4 near_far_view = glm::mat4x4(glm::vec4{0.99714291, -0.0141582564, 0.056670256, 0.0},
                                                      glm::vec4{0.0574234501, 0.415619671, -0.907724082, 0.0},
                                                      glm::vec4{-0.0107014906, 0.909428417, 0.415723026, 0.0},
                                                      glm::vec4{-0.0279345326, 1.56813979, 0.0279776659, 1.0});

    static const glm::mat4x4 far_view = glm::mat4x4(glm::vec4{0.996096014, 0.0689616278, -0.0551095828, 0.0},
                                                         glm::vec4{-0.0692624822, 0.223489523, -0.972242534, 0.0},
                                                         glm::vec4{-0.054730989, 0.972263872, 0.227393508, 0.0},
                                                         glm::vec4{-0.0376491882, 2.67884994, 0.0468178503, 1.0});

    static const glm::mat4x4 no_view = glm::mat4x4(glm::vec4{0.447677672, 0.319176674, 0.835290968, 0.0},
                                                    glm::vec4{-0.755516648, 0.634681344, 0.162401468, 0.0},
                                                    glm::vec4{-0.478308856, -0.703779697, 0.525275946, 0.0},
                                                    glm::vec4{0.041105058, 1.71572995, 0.108409137, 1.0});
}
#endif //OCULUSROOT_TEST_PERSPECTIVES_H
