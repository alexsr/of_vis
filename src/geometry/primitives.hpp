//
// Alexander Scheid-Rehder
// alexanderb@scheid-rehder.de
// https://www.alexsr.de
// https://github.com/alexsr
//

#pragma once

#include "geometry.hpp"

namespace tostf
{
    struct Rectangle : Geometry {
        explicit Rectangle(float x = 1.0f, float y = 1.0f);
    };

    struct Rectangle_strip : Geometry {
        explicit Rectangle_strip(float x = 1.0f, float y = 1.0f);
    };

    struct Cube : Geometry {
        explicit Cube(float r);
        Cube(float x, float y, float z);
    };

    struct Sphere : Geometry {
        explicit Sphere(float r = 1.0f, int hres = 10, int vres = 10);
    };
}
