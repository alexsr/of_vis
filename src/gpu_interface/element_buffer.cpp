//
// Alexander Scheid-Rehder
// alexanderb@scheid-rehder.de
// https://www.alexsr.de
// https://github.com/alexsr
//

#include "element_buffer.hpp"

tostf::Element_buffer::Element_buffer(const gl::storage flags)
    : Buffer(GL_ELEMENT_ARRAY_BUFFER, flags) {
}

unsigned long tostf::Element_buffer::get_count() const {
    return _count;
}
