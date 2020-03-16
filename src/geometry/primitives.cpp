//
// Alexander Scheid-Rehder
// alexanderb@scheid-rehder.de
// https://www.alexsr.de
// https://github.com/alexsr
//

#include "primitives.hpp"

tostf::Rectangle::Rectangle(const float x, const float y) {
    vertices = {
        {-x / 2.0f, -y / 2.0f, 0.0f, 1.0f},
        {x / 2.0f, -y / 2.0f, 0.0f, 1.0f},
        {-x / 2.0f, y / 2.0f, 0.0f, 1.0f},
        {x / 2.0f, -y / 2.0f, 0.0f, 1.0f},
        {-x / 2.0f, y / 2.0f, 0.0f, 1.0f},
        {x / 2.0f, y / 2.0f, 0.0f, 1.0f}
    };
    normals = {
        {0.0f, 0.0f, 1.0f, 0.0f},
        {0.0f, 0.0f, 1.0f, 0.0f},
        {0.0f, 0.0f, 1.0f, 0.0f},
        {0.0f, 0.0f, 1.0f, 0.0f},
        {0.0f, 0.0f, 1.0f, 0.0f},
        {0.0f, 0.0f, 1.0f, 0.0f}
    };
    uv_coords = {
        {0.0f, 0.0f},
        {1.0f, 0.0f},
        {0.0f, 1.0f},
        {1.0f, 0.0f},
        {0.0f, 1.0f},
        {1.0f, 1.0f}
    };
    indices = {0, 1, 2, 3, 4, 5};
}

tostf::Rectangle_strip::Rectangle_strip(const float x, const float y) {
    vertices = {
        {-x / 2.0f, -y / 2.0f, 0.0f, 1.0f},
        {x / 2.0f, -y / 2.0f, 0.0f, 1.0f},
        {-x / 2.0f, y / 2.0f, 0.0f, 1.0f},
        {x / 2.0f, y / 2.0f, 0.0f, 1.0f}
    };
    normals = {
        {0.0f, 0.0f, 1.0f, 0.0f},
        {0.0f, 0.0f, 1.0f, 0.0f},
        {0.0f, 0.0f, 1.0f, 0.0f},
        {0.0f, 0.0f, 1.0f, 0.0f}
    };
    uv_coords = {
        {0.0f, 0.0f},
        {1.0f, 0.0f},
        {0.0f, 1.0f},
        {1.0f, 0.0f}
    };
    indices = {0, 1, 2, 1, 3, 2};
}

tostf::Cube::Cube(const float r) : Cube(r, r, r) {}

tostf::Cube::Cube(const float x, const float y, const float z) {
    float x_lower = -x / 2.0f;
    float x_upper = x / 2.0f;
    float y_lower = -y / 2.0f;
    float y_upper = y / 2.0f;
    float z_lower = -z / 2.0f;
    float z_upper = z / 2.0f;
    vertices = {
        {x_lower, y_lower, z_upper, 1.0f},
        {x_upper, y_lower, z_upper, 1.0f},
        {x_upper, y_upper, z_upper, 1.0f},
        {x_upper, y_upper, z_upper, 1.0f},
        {x_lower, y_upper, z_upper, 1.0f},
        {x_lower, y_lower, z_upper, 1.0f},

        {x_lower, y_lower, z_lower, 1.0f},
        {x_lower, y_lower, z_upper, 1.0f},
        {x_lower, y_upper, z_upper, 1.0f},
        {x_lower, y_upper, z_upper, 1.0f},
        {x_lower, y_upper, z_lower, 1.0f},
        {x_lower, y_lower, z_lower, 1.0f},

        {x_upper, y_lower, z_upper, 1.0f},
        {x_upper, y_lower, z_lower, 1.0f},
        {x_upper, y_upper, z_lower, 1.0f},
        {x_upper, y_upper, z_lower, 1.0f},
        {x_upper, y_upper, z_upper, 1.0f},
        {x_upper, y_lower, z_upper, 1.0f},

        {x_lower, y_upper, z_upper, 1.0f},
        {x_upper, y_upper, z_upper, 1.0f},
        {x_upper, y_upper, z_lower, 1.0f},
        {x_upper, y_upper, z_lower, 1.0f},
        {x_lower, y_upper, z_lower, 1.0f},
        {x_lower, y_upper, z_upper, 1.0f},

        {x_lower, y_lower, z_lower, 1.0f},
        {x_upper, y_lower, z_lower, 1.0f},
        {x_upper, y_lower, z_upper, 1.0f},
        {x_upper, y_lower, z_upper, 1.0f},
        {x_lower, y_lower, z_upper, 1.0f},
        {x_lower, y_lower, z_lower, 1.0f},

        {x_lower, y_upper, z_lower, 1.0f},
        {x_upper, y_upper, z_lower, 1.0f},
        {x_upper, y_lower, z_lower, 1.0f},
        {x_upper, y_lower, z_lower, 1.0f},
        {x_lower, y_lower, z_lower, 1.0f},
        {x_lower, y_upper, z_lower, 1.0f},
    };
    normals = {
        {0.0f, 0.0f, 1.0f, 0.0f},
        {0.0f, 0.0f, 1.0f, 0.0f},
        {0.0f, 0.0f, 1.0f, 0.0f},
        {0.0f, 0.0f, 1.0f, 0.0f},
        {0.0f, 0.0f, 1.0f, 0.0f},
        {0.0f, 0.0f, 1.0f, 0.0f},

        {-1.0f, 0.0f, 0.0f, 0.0f},
        {-1.0f, 0.0f, 0.0f, 0.0f},
        {-1.0f, 0.0f, 0.0f, 0.0f},
        {-1.0f, 0.0f, 0.0f, 0.0f},
        {-1.0f, 0.0f, 0.0f, 0.0f},
        {-1.0f, 0.0f, 0.0f, 0.0f},

        {1.0f, 0.0f, 0.0f, 0.0f},
        {1.0f, 0.0f, 0.0f, 0.0f},
        {1.0f, 0.0f, 0.0f, 0.0f},
        {1.0f, 0.0f, 0.0f, 0.0f},
        {1.0f, 0.0f, 0.0f, 0.0f},
        {1.0f, 0.0f, 0.0f, 0.0f},

        {0.0f, 1.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f, 0.0f},

        {0.0f, -1.0f, 0.0f, 0.0f},
        {0.0f, -1.0f, 0.0f, 0.0f},
        {0.0f, -1.0f, 0.0f, 0.0f},
        {0.0f, -1.0f, 0.0f, 0.0f},
        {0.0f, -1.0f, 0.0f, 0.0f},
        {0.0f, -1.0f, 0.0f, 0.0f},

        {0.0f, 0.0f, -1.0f, 0.0f},
        {0.0f, 0.0f, -1.0f, 0.0f},
        {0.0f, 0.0f, -1.0f, 0.0f},
        {0.0f, 0.0f, -1.0f, 0.0f},
        {0.0f, 0.0f, -1.0f, 0.0f},
        {0.0f, 0.0f, -1.0f, 0.0f}
    };
    uv_coords = {
        {0.0f, 0.0f},
        {1.0f, 0.0f},
        {1.0f, 1.0f},
        {1.0f, 1.0f},
        {0.0f, 1.0f},
        {0.0f, 0.0f},

        {0.0f, 0.0f},
        {1.0f, 0.0f},
        {1.0f, 1.0f},
        {1.0f, 1.0f},
        {0.0f, 1.0f},
        {0.0f, 0.0f},

        {0.0f, 0.0f},
        {1.0f, 0.0f},
        {1.0f, 1.0f},
        {1.0f, 1.0f},
        {0.0f, 1.0f},
        {0.0f, 0.0f},

        {0.0f, 0.0f},
        {1.0f, 0.0f},
        {1.0f, 1.0f},
        {1.0f, 1.0f},
        {0.0f, 1.0f},
        {0.0f, 0.0f},

        {0.0f, 0.0f},
        {1.0f, 0.0f},
        {1.0f, 1.0f},
        {1.0f, 1.0f},
        {0.0f, 1.0f},
        {0.0f, 0.0f},

        {0.0f, 0.0f},
        {1.0f, 0.0f},
        {1.0f, 1.0f},
        {1.0f, 1.0f},
        {0.0f, 1.0f},
        {0.0f, 0.0f}
    };
    indices = {
        0, 1, 2, 3, 4, 5,
        6, 7, 8, 9, 10, 11,
        12, 13, 14, 15, 16, 17,
        18, 19, 20, 21, 22, 23,
        24, 25, 26, 27, 28, 29,
        30, 31, 32, 33, 34, 35
    };
}

tostf::Sphere::Sphere(const float r, const int hres, const int vres) {
    vertices.resize(hres * vres * 6);
    normals.resize(hres * vres * 6);
    uv_coords.resize(hres * vres * 6);
    indices.resize(hres * vres * 6);
    const auto d_h = 2 * glm::pi<float>() / static_cast<float>(hres);
    const auto d_v = glm::pi<float>() / static_cast<float>(vres);
    int n = 0;
    // Vertices are created inside this loop.
    for (int i = 0; i < hres; i++) {
        const auto h = i * d_h;
        const auto hn = h + d_h;
        for (int j = 0; j < vres; j++) {
            const auto v = j * d_v;
            const auto vn = v + d_v;

            // The sphere is consists of multiple triangles where 2 triangles make a plane.
            // These 4 points are the corners of said plane. To create a triangle 3 of these corners are
            // used counterclockwise with the 2nd triangle's first point being the 1st last point.
            // Normal vectors are the same as the points without the radius multiplied.
            const glm::vec4 p0(glm::vec3(glm::cos(h) * glm::sin(v), glm::sin(h) * glm::sin(v),
                                         glm::cos(v)) * r, 1.0f);
            const glm::vec4 p1(glm::vec3(glm::cos(h) * glm::sin(vn), glm::sin(h) * glm::sin(vn),
                                         glm::cos(vn)) * r, 1.0f);
            const glm::vec4 p2(glm::vec3(glm::cos(hn) * glm::sin(v), glm::sin(hn) * glm::sin(v),
                                         glm::cos(v)) * r, 1.0f);
            const glm::vec4 p3(glm::vec3(glm::cos(hn) * glm::sin(vn), glm::sin(hn) * glm::sin(vn),
                                         glm::cos(vn)) * r, 1.0f);
            vertices.at(n) = p0;
            normals.at(n++) = normalize(glm::vec4(p0.x, p0.y, p0.z, 0.0f));
            vertices.at(n) = p1;
            normals.at(n++) = normalize(glm::vec4(p1.x, p1.y, p1.z, 0.0f));
            vertices.at(n) = p3;
            normals.at(n++) = normalize(glm::vec4(p3.x, p3.y, p3.z, 0.0f));
            vertices.at(n) = p3;
            normals.at(n++) = normalize(glm::vec4(p3.x, p3.y, p3.z, 0.0f));
            vertices.at(n) = p2;
            normals.at(n++) = normalize(glm::vec4(p2.x, p2.y, p2.z, 0.0f));
            vertices.at(n) = p0;
            normals.at(n++) = normalize(glm::vec4(p0.x, p0.y, p0.z, 0.0f));
        }
    }
}
