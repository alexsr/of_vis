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
    using ui_pos = glm::ivec2;
    using ui_size = glm::ivec2;
    enum class ui_align_horizontal {
        center,
        left,
        right
    };
    enum class ui_align_vertical {
        center,
        top,
        bottom
    };
    struct ui_align {
        ui_align_horizontal horizontal = ui_align_horizontal::left;
        ui_align_vertical vertical = ui_align_vertical::bottom;
    };

    class Ui_element {
    public:
        Ui_element(ui_pos pos, ui_size size, ui_align alignment = {}, bool focusable = true);
        Ui_element(const Ui_element&) = default;
        Ui_element& operator=(const Ui_element&) = default;
        Ui_element(Ui_element&&) = default;
        Ui_element& operator=(Ui_element&&) = default;
        virtual ~Ui_element() = default;
        unsigned get_id() const;
        ui_size get_size() const;
        ui_pos get_pos() const;
        bool is_focusable() const;
    protected:
        ui_pos _pos;
        ui_size _size;
        ui_align _align;
        bool _focusable;
        static unsigned _next_id;
        unsigned _id;
    };
}
