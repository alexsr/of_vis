//
// Alexander Scheid-Rehder
// alexanderb@scheid-rehder.de
// https://www.alexsr.de
// https://github.com/alexsr
//

#include "view.hpp"
#include "utility/logging.hpp"

tostf::View::View(const ui_pos pos, const ui_size size)
    : Ui_element(pos, size), _viewport({pos.x, pos.y, size.x, size.y}) {}

tostf::View::View(const ui_size size)
    : Ui_element({0, 0}, size), _viewport({0, 0, size.x, size.y}) {}

void tostf::View::activate() const {
    gl::set_viewport(_viewport.x, _viewport.y, static_cast<int>(_viewport.width * _scale.x),
                     static_cast<int>(_viewport.height * _scale.y));
    for (auto& [n, c] : _cameras) {
        c->resize();
    }
}

void tostf::View::resize(const ui_size size) {
    _size = size;
    _viewport.width = size.x;
    _viewport.height = size.y;
}

void tostf::View::set_pos(const ui_pos pos) {
    _pos = pos;
    _viewport.x = pos.x;
    _viewport.y = pos.y;}

tostf::gl::Viewport tostf::View::get_viewport() const {
    return _viewport;
}

void tostf::View::clear() {
    enable(gl::cap::scissor);
    set_scissor(_viewport);
    gl::clear_all();
    disable(gl::cap::scissor);
}

void tostf::View::attach_camera(const std::string& name, const std::shared_ptr<Camera>& cam) {
    const auto res = _cameras.insert_or_assign(name, cam);
    if (res.second) {
        log_info() << "Camera \"" << name << "\" attached to view.";
    }
    else {
        log_warning() << "Camera \"" << name << "\" was overwritten.";
    }
}

void tostf::View::detach_camera(const std::string& name) {
    const auto it = _cameras.find(name);
    if (it != _cameras.end()) {
        _cameras.erase(it);
        log_info() << "Camera \"" << name << "\" detached from view.";
    }
    else {
        log_warning() << "Camera \"" << name << "\" could not be found.";
    }
}
