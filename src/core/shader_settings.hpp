//
// Alexander Scheid-Rehder
// alexanderb@scheid-rehder.de
// https://www.alexsr.de
// https://github.com/alexsr
//

#pragma once

#include "math/glm_helper.hpp"

namespace tostf
{
    struct Phong_settings {
        glm::vec4 ambient_color;
        glm::vec4 light_pos;
        glm::vec4 light_color;
        glm::vec4 color;
        float shininess;
    };

    struct Diffuse_settings {
        glm::vec4 ambient_color;
        glm::vec4 light_pos;
        glm::vec4 light_color;
    };

    struct Wireframe_settings {
        float line_width = 1.0;
        glm::vec3 color = glm::vec3(1.0f);
        glm::vec3 color_selected = glm::vec3(1.0f, 0.0f, 0.0f);
    };
}
