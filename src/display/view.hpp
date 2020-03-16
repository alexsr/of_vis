//
// Alexander Scheid-Rehder
// alexanderb@scheid-rehder.de
// https://www.alexsr.de
// https://github.com/alexsr
//

#pragma once

#include "camera.hpp"
#include "ui_elements.hpp"

namespace tostf
{
    class View : public Ui_element {
    public:
        explicit View(ui_pos pos, ui_size size);
        explicit View(ui_size size);
        View(const View&) = default;
        View& operator=(const View&) = default;
        View(View&&) = default;
        View& operator=(View&&) = default;
        virtual ~View() = default;
        void activate() const;
        void resize(ui_size size);
        void set_pos(ui_pos pos);
        gl::Viewport get_viewport() const;
        void clear();
        void attach_camera(const std::string& name, const std::shared_ptr<Camera>& cam);
        void detach_camera(const std::string& name);
    private:
        gl::Viewport _viewport{};
        glm::vec2 _scale{1, 1};
        std::map<std::string, std::shared_ptr<Camera>> _cameras{};
    };
}
