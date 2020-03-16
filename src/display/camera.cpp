//
// Alexander Scheid-Rehder
// alexanderb@scheid-rehder.de
// https://www.alexsr.de
// https://github.com/alexsr
//

#include "camera.hpp"

tostf::Camera::Camera(const float rotation_speed, const float movement_speed, const glm::vec3 position,
                      const float fov, const float near, const float far)
    : _rotation_speed(rotation_speed), _movement_speed(movement_speed), _position(position, 1.0f),
      _fov(glm::radians(fov)), _near(near), _far(far) {
    _viewport = gl::get_viewport();
    _projection = glm::perspective(_fov, _viewport.width / static_cast<float>(_viewport.height), _near, _far);
}

glm::mat4 tostf::Camera::get_view() const {
    return _view;
}

glm::mat4 tostf::Camera::get_projection() const {
    return _projection;
}

glm::vec4 tostf::Camera::get_position() const {
    return _position;
}

float tostf::Camera::get_near() const {
    return _near;
}

void tostf::Camera::set_near(const float n) {
    _near = n;
    _projection = glm::perspective(_fov, static_cast<float>(_viewport.width) / _viewport.height, _near, _far);
}

void tostf::Camera::resize() {
    const auto viewport = gl::get_viewport();
    if (_viewport.width != viewport.width || _viewport.height != viewport.height) {
        _viewport = viewport;
        _projection[0][0] = _viewport.height / (glm::tan(_fov / 2.0f) * _viewport.width);
    }
}

void tostf::Camera::set_move_speed(const float move_speed) {
    _movement_speed = move_speed;
}

tostf::Trackball_camera::Trackball_camera(const float radius, const float rotation_speed, const float movement_speed,
                                          const bool fast_rotate, const glm::vec3 center,
                                          const float fov, const float near, const float far)
    : Camera(rotation_speed, movement_speed, center - glm::vec3(0, 0, radius), fov, near, far), _center(center),
      _direction(glm::vec3(0, 0, -1)), _radius(radius), _fast_rotate(fast_rotate) {
    _orientation = glm::quat();
}

glm::vec3 tostf::Trackball_camera::get_center() const {
    return _center;
}

glm::mat4 tostf::Trackball_camera::get_rotation() const {
    return mat4_cast(_orientation);
}

float tostf::Trackball_camera::get_radius() const {
    return _radius;
}

void tostf::Trackball_camera::set_center(const glm::vec3 c) {
    _center = c;
}

void tostf::Trackball_camera::set_radius(const float radius) {
    _radius = radius;
}

void tostf::Trackball_camera::rotate_to_vector(const glm::vec3 n) {
    // https://math.stackexchange.com/a/476311
    const auto dir = glm::vec3(_direction);
    const auto n_n = normalize(_orientation * n);
    glm::mat4 rot_mat;
    if (dir == -n_n) {
        if (abs(dir.z) == 1.0f) {
            rot_mat = glm::mat4(-1.0f, 0.0f, 0.0f, 0.0f,
                                0.0f, 1.0f, 0.0f, 0.0f,
                                0.0f, 0.0f, -1.0f, 0.0f,
                                0.0f, 0.0f, 0.0f, 1.0f);
        }
        else {
            const auto dir_x2_y2 = dir.x * dir.x - dir.y * dir.y;
            const auto dir_x_y = -2.0f * dir.x * dir.y;
            const auto denom_z = 1.0f - dir.z * dir.z;
            rot_mat = 1.0f / denom_z * glm::mat4(-dir_x2_y2, dir_x_y, 0.0f, 0.0f,
                                                 dir_x_y, dir_x2_y2, 0.0f, 0.0f,
                                                 0.0f, 0.0f, -denom_z, 0.0f,
                                                 0.0f, 0.0f, 0.0f, 1.0f);
        }
    }
    else {
        const auto v = cross(n_n, dir);
        const auto cos_alpha = 1.0f / (1.0f + dot(dir, n_n));
        const glm::mat3 skew_mat(0.0f, v.z, -v.y, -v.z, 0.0f, v.x, v.y, -v.x, 0.0f);
        rot_mat = glm::mat4(glm::mat3(1.0f) + skew_mat + skew_mat * skew_mat * cos_alpha);
    }
    _orientation = normalize(normalize(quat_cast(rot_mat)) * _orientation);
}

void tostf::Trackball_camera::update(const Window& window, const float dt) {
    const auto cursor = window.get_cursor_pos();
    if (window.check_mouse_action(win_mgr::mouse::left, win_mgr::mouse_action::press, win_mgr::key_mod::shift)) {
        const auto dt_sens = _movement_speed * dt * 0.1f;
        const auto change_x = -static_cast<float>(cursor.x - _x) * dt_sens;
        const auto change_y = static_cast<float>(cursor.y - _y) * dt_sens;
        const auto movement_vec = glm::vec4(change_x, change_y, 0.0f, 0.0f);
        _center += glm::vec3(movement_vec);
    }
    else if (window.check_mouse_action(win_mgr::mouse::left, win_mgr::mouse_action::press)) {
        window.set_cursor_mode(win_mgr::cursor_mode::disabled);
        auto dt_sens = _rotation_speed * dt;
        if (!_fast_rotate || window.check_key_action(win_mgr::key::left_ctrl, win_mgr::key_action::press)) {
            dt_sens /= 10.0f;
        }
        const auto change_x = static_cast<float>(cursor.x - _x) * dt_sens;
        const auto change_y = static_cast<float>(cursor.y - _y) * dt_sens;
        auto q = angleAxis(change_y, glm::vec3(1, 0, 0));
        q *= angleAxis(change_x, glm::vec3(0, 1, 0));
        _orientation = normalize(q * _orientation);
    }
    else {
        window.set_cursor_mode(win_mgr::cursor_mode::normal);
    }
    _x = cursor.x;
    _y = cursor.y;
    move_camera(window, dt);
    update_view();
    resize();
}

void tostf::Trackball_camera::update_view() {
    _view = translate(glm::mat4(1.0f), -_center) * translate(glm::mat4(1.0f), _radius * _direction) * mat4_cast(_orientation);
    //_view[3][3] = 1.0f;
}

void tostf::Trackball_camera::move_camera(const Window& window, const float dt) {
    const auto scroll_y = window.get_scroll_y();
    if (scroll_y != 0) {
        auto scroll_speed = _movement_speed;
        if (window.check_key_action(win_mgr::key::left_ctrl, win_mgr::key_action::press)) {
            scroll_speed /= 10.0f;
        }
        _radius -= static_cast<float>(glm::sign(scroll_y)) * scroll_speed * dt;
        // if (_radius < _near) {
        //     _radius = _near;
        // }
    }
    _position = glm::vec4(_center - _radius * _direction, 1.0f);
}

//
// tostf::Iterative_trackball::Iterative_trackball(const float rotation_speed, const float radius,
//                                                 const float movement_speed, const bool fast_rotate,
//                                                 const glm::vec3 center, const float fov,
//                                                 const float near, const float far)
//     : Camera(rotation_speed, movement_speed, glm::vec3(0), fov, near, far), _fast_rotate(fast_rotate),
//       _radius(radius), _center(center) {
//     _theta = glm::pi<float>() / 2.0f;
//     _phi = 0.0f;
//     const auto sin_theta = glm::sin(_theta);
//     const auto eye = _center + 1.0f * glm::vec3(sin_theta * glm::sin(_phi), glm::cos(_theta),
//                                                 sin_theta * glm::cos(_phi));
//     _view = lookAt(eye, center, glm::vec3(0.0f, 1.0f, 0.0f));
//     _view[3] = normalize(_view[3]) * _radius;
// }
//
// glm::vec3 tostf::Iterative_trackball::get_center() const {
//     return _center;
// }
//
// void tostf::Iterative_trackball::update(const Window& window, const float dt) {
//     const auto cursor = window.get_cursor_pos();
//     const auto dt_sens = _rotation_speed * dt;
//     if (window.check_mouse_action(win_mgr::mouse::left, win_mgr::mouse_action::press)) {
//         window.set_cursor_mode(win_mgr::cursor_mode::disabled);
//         const auto change_x = static_cast<float>(cursor.x - _x);
//         const auto change_y = static_cast<float>(cursor.y - _y);
//         auto angle = glm::sqrt(change_x * change_x + change_y * change_y) / window.get_diag_size();
//         if (angle != 0.0f) {
//             if (!_fast_rotate || window.check_key_action(win_mgr::key::left_ctrl, win_mgr::key_action::press)) {
//                 angle /= 10.0f;
//             }
//             const auto eye = glm::vec3(inverse(_view) * glm::vec4(change_y, change_x, 0.0f, 0.0f));
//             _view = rotate(_view, dt_sens * angle, eye);
//         }
//     }
//     else {
//         window.set_cursor_mode(win_mgr::cursor_mode::normal);
//         _x = cursor.x;
//         _y = cursor.y;
//     }
//     // _position = glm::vec4(eye, 1.0f);
//     move_camera(window, dt);
//     resize();
// }
//
// void tostf::Iterative_trackball::move_camera(const Window& window, const float dt) {
//     const auto scroll_y = window.get_scroll_y();
//     if (scroll_y != 0) {
//         auto scroll_speed = _movement_speed;
//         if (window.check_key_action(win_mgr::key::left_ctrl, win_mgr::key_action::press)) {
//             scroll_speed /= 10.0f;
//         }
//         _radius -= static_cast<float>(glm::sign(scroll_y)) * scroll_speed * dt;
//         if (_radius < 1.0f) {
//             _radius = 1.0f;
//         }
//         _view[3] = normalize(_view[3]) * _radius;
//     }
// }
