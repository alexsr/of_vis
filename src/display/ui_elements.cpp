//
// Alexander Scheid-Rehder
// alexanderb@scheid-rehder.de
// https://www.alexsr.de
// https://github.com/alexsr
//

#include "ui_elements.hpp"

unsigned tostf::Ui_element::_next_id = 0; // _next_id has to be initialized.

tostf::Ui_element::Ui_element(const ui_pos pos, const ui_size size, const ui_align alignment, const bool focusable)
    : _pos(pos), _size(size), _align(alignment), _focusable(focusable), _id(_next_id++) {
    
}

unsigned tostf::Ui_element::get_id() const {
    return _id;
}

tostf::ui_size tostf::Ui_element::get_size() const {
    return _size;
}

tostf::ui_pos tostf::Ui_element::get_pos() const {
    return _pos;
}

bool tostf::Ui_element::is_focusable() const {
    return _focusable;
}
