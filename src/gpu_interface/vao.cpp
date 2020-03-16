//
// Alexander Scheid-Rehder
// alexanderb@scheid-rehder.de
// https://www.alexsr.de
// https://github.com/alexsr
//

#include "vao.hpp"

tostf::VAO_attachment::VAO_attachment(std::shared_ptr<VBO> vbo_in, const unsigned id_in)
    : vbo(std::move(vbo_in)), id(id_in) {}

tostf::Instanced_VAO_attachment::Instanced_VAO_attachment(std::shared_ptr<VBO> vbo_in, const unsigned id_in,
                                                          const unsigned divisor_in)
    : vbo(std::move(vbo_in)), id(id_in), divisor(divisor_in) {}


tostf::VAO::VAO() {
    glCreateVertexArrays(1, &_name);
}

tostf::VAO::VAO(VAO&& other) noexcept {
    _name = other._name;
    _count = other._count;
    other._name = 0;
}

tostf::VAO& tostf::VAO::operator=(VAO&& other) noexcept {
    _name = other._name;
    _count = other._count;
    other._name = 0;
    return *this;
}

tostf::VAO::~VAO() {
    if (glIsVertexArray(_name)) {
        unbind();
        glDeleteVertexArrays(1, &_name);
    }
}

void tostf::VAO::bind() const {
    glBindVertexArray(_name);
}

void tostf::VAO::unbind() {
    glBindVertexArray(0);
}

void tostf::VAO::attach_main_vbo(const std::shared_ptr<VBO>& vbo, const unsigned location_id) {
    attach_vbo(vbo, location_id);
    _count = vbo->get_count();
}

void tostf::VAO::attach_main_vbo_instanced(const std::shared_ptr<VBO>& vbo, const unsigned location_id,
                                           const unsigned divisor) {
    attach_main_vbo(vbo, location_id);
    glVertexArrayBindingDivisor(_name, location_id, divisor);
}

void tostf::VAO::attach_vbo(const std::shared_ptr<VBO>& vbo, const unsigned location_id) const {
    glEnableVertexArrayAttrib(_name, location_id);
    glVertexArrayVertexBuffer(_name, location_id, vbo->get_name(), 0, vbo->get_stride());
    set_attrib_format(location_id, vbo->get_vertex_size(), vbo->get_type());
    glVertexArrayAttribBinding(_name, location_id, location_id);
}

void tostf::VAO::attach_vbo_instanced(const std::shared_ptr<VBO>& vbo, const unsigned location_id,
                                      const unsigned divisor) const {
    attach_vbo(vbo, location_id);
    glVertexArrayBindingDivisor(_name, location_id, divisor);
}

void tostf::VAO::attach_main_vbo(const VBO& vbo, const unsigned location_id) {
    attach_vbo(vbo, location_id);
    _count = vbo.get_count();
}

void tostf::VAO::attach_main_vbo_instanced(const VBO& vbo, const unsigned location_id,
                                           const unsigned divisor) {
    attach_main_vbo(vbo, location_id);
    glVertexArrayBindingDivisor(_name, location_id, divisor);
}

void tostf::VAO::attach_vbo(const VBO& vbo, const unsigned location_id) const {
    glEnableVertexArrayAttrib(_name, location_id);
    glVertexArrayVertexBuffer(_name, location_id, vbo.get_name(), 0, vbo.get_stride());
    set_attrib_format(location_id, vbo.get_vertex_size(), vbo.get_type());
    glVertexArrayAttribBinding(_name, location_id, location_id);
}

void tostf::VAO::attach_vbo_instanced(const VBO& vbo, const unsigned location_id,
                                      const unsigned divisor) const {
    attach_vbo(vbo, location_id);
    glVertexArrayBindingDivisor(_name, location_id, divisor);
}

void tostf::VAO::attach_vbos(const std::vector<VAO_attachment>& vbos, const bool overwrite_count) {
    if (overwrite_count) {
        _count = vbos.at(0).vbo->get_count();
    }
    for (auto& v : vbos) {
        attach_vbo(v.vbo, v.id);
    }
}

void tostf::VAO::attach_vbos_instanced(const std::vector<Instanced_VAO_attachment>& vbos) const {
    for (auto& v : vbos) {
        attach_vbo_instanced(v.vbo, v.id, v.divisor);
    }
}

void tostf::VAO::detach_vbo(const unsigned location_id) const {
    glEnableVertexArrayAttrib(_name, location_id);
    glVertexArrayVertexBuffer(_name, location_id, 0, 0, 0);
}

void tostf::VAO::attach_ebo(const std::shared_ptr<Element_buffer>& ebo) {
    glVertexArrayElementBuffer(_name, ebo->get_name());
    _count = ebo->get_count();
}

void tostf::VAO::detach_ebo() const {
    glVertexArrayElementBuffer(_name, 0);
}

void tostf::VAO::set_count(const unsigned count) {
    _count = count;
}

void tostf::VAO::draw(const gl::draw mode, const unsigned instances, int count, const unsigned offset,
                      const unsigned base_instance) const {
    bind();
    if (count < 0) {
        count = _count;
    }
    glDrawArraysInstancedBaseInstance(to_underlying_type(mode), offset, count, instances, base_instance);
}

void tostf::VAO::draw_elements(const gl::draw mode, const unsigned instances, int count, const unsigned offset,
                               const unsigned base_instance, const unsigned base_vertex) const {
    bind();
    if (count < 0) {
        count = _count;
    }
    glDrawElementsInstancedBaseVertexBaseInstance(to_underlying_type(mode), count, GL_UNSIGNED_INT,
                                                  reinterpret_cast<const void*>(static_cast<size_t>(offset)),
                                                  instances, base_vertex, base_instance);
}

void tostf::VAO::set_attrib_format(const unsigned int index, const GLint vertex_size,
                                   const gl::v_type type) const {
    if (type == gl::v_type::dbl) {
        glVertexArrayAttribLFormat(_name, index, vertex_size, to_underlying_type(type), 0);
    }
    else if (type == gl::v_type::byte || type == gl::v_type::shrt
             || type == gl::v_type::integer || type == gl::v_type::ubyte
             || type == gl::v_type::ushrt || type == gl::v_type::uint) {
        glVertexArrayAttribIFormat(_name, index, vertex_size, to_underlying_type(type), 0);
    }
    else {
        glVertexArrayAttribFormat(_name, index, vertex_size, to_underlying_type(type), GL_FALSE, 0);
    }
}
