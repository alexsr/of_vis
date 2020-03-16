//
// Alexander Scheid-Rehder
// alexanderb@scheid-rehder.de
// https://www.alexsr.de
// https://github.com/alexsr
//

#pragma once

#include "math/glm_helper.hpp"
#include "window.hpp"

#undef near
#undef far

namespace tostf
{
    // Base class for all camera types containing both basic functionality as
    // well as common parameters.
    class Camera {
    public:
        Camera(const Camera&) = default;
        Camera& operator=(const Camera&) = default;
        Camera(Camera&&) = default;
        Camera& operator=(Camera&&) = default;
        virtual ~Camera() = default;
        glm::mat4 get_view() const;
        glm::mat4 get_projection() const;
        glm::vec4 get_position() const;
        float get_near() const;
        void set_near(float n);
        // Updates the camera view and projection matrix based on user input.
        virtual void update(const Window& window, float dt) = 0;
        // Updates projection matrix in case of a viewport size change.
        void resize();
        void set_move_speed(float move_speed);
    protected:
        explicit Camera(float rotation_speed = 1.0f, float movement_speed = 1.0f,
                        glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f), float fov = 60.0f,
                        float near = 0.001f, float far = 1000.0f);
        virtual void move_camera(const Window& window, float dt) = 0;

        glm::mat4 _view{};
        glm::mat4 _projection{};
        gl::Viewport _viewport{};
        float _rotation_speed;
        float _movement_speed;
        glm::vec4 _position;
        glm::quat _orientation;
        double _x{};
        double _y{};
        float _fov;
        float _near;
        float _far;
    };

    // A Trackball camera has a center point it around which it is able to move while still
    // looking at the center. This allows for a 360 degree view around an object.
    class Trackball_camera : public Camera {
    public:
        explicit Trackball_camera(float radius = 1.0f, float rotation_speed = 1.0f, float movement_speed = 1.0f,
                                  bool fast_rotate = true, glm::vec3 center = glm::vec3(0.0f, 0.0f, 0.0f),
                                  float fov = 60.0f, float near = 0.001f, float far = 1000.0f);
        glm::vec3 get_center() const;
        glm::mat4 get_rotation() const;
        float get_radius() const;
        void set_center(glm::vec3 c);
        void set_radius(float radius);
        void rotate_to_vector(glm::vec3 n);

        // Updates both the view and the projection matrix based on user input.
        // Checks for user interaction and updates the eye position on mouse movement
        // and changes the distance from eye to center by keyboard input.
        // The projection matrix is updated in case of a viewport change.
        void update(const Window& window, float dt) override;
    private:
        void update_view();
        void move_camera(const Window& window, float dt) override;

        glm::vec3 _center;
        glm::vec3 _direction;
        float _radius;
        bool _fast_rotate;
    };

    class Iterative_trackball : public Camera {
    public:
        explicit Iterative_trackball(float rotation_speed = 1.0f, float radius = 2.0, float movement_speed = 1.0f,
                                     bool fast_rotate = true, glm::vec3 center = glm::vec3(0.0f),
                                     float fov = 60.0f, float near = 0.001f, float far = 1000.0f);
        glm::vec3 get_center() const;

        // Updates both the view and the projection matrix based on user input.
        // Checks for user interaction and updates the eye position on mouse movement
        // and changes the distance from eye to center by keyboard input.
        // The projection matrix is updated in case of a viewport change.
        void update(const Window& window, float dt) override;
    private:
        void move_camera(const Window& window, float dt) override;
        bool _fast_rotate;
        float _radius;
        glm::vec3 _center;
    };
}
