//
// Alexander Scheid-Rehder
// alexanderb@scheid-rehder.de
// https://www.alexsr.de
// https://github.com/alexsr
//

#pragma once

#include "vbo.hpp"
#include "element_buffer.hpp"
#include <memory>

namespace tostf
{
    struct VAO_attachment {
        VAO_attachment(std::shared_ptr<VBO> vbo_in, unsigned id_in);
        std::shared_ptr<VBO> vbo;
        unsigned id;
    };
    struct Instanced_VAO_attachment {
        Instanced_VAO_attachment(std::shared_ptr<VBO> vbo_in, unsigned id_in, unsigned divisor_in);
        std::shared_ptr<VBO> vbo;
        unsigned id;
        unsigned divisor;
    };

    class VAO {
    public:
        VAO();
        VAO(const VAO&) = default;
        VAO& operator=(const VAO&) = default;
        VAO(VAO&& other) noexcept;
        VAO& operator=(VAO&& other) noexcept;
        virtual ~VAO();
        void bind() const;
        static void unbind();
        void attach_main_vbo(const std::shared_ptr<VBO>& vbo, unsigned location_id = 0);
        void attach_main_vbo_instanced(const std::shared_ptr<VBO>& vbo, unsigned location_id = 0, unsigned divisor = 1);
        void attach_vbo(const std::shared_ptr<VBO>& vbo, unsigned location_id) const;
        void attach_vbo_instanced(const std::shared_ptr<VBO>& vbo, unsigned location_id, unsigned divisor = 1) const;
        void attach_main_vbo(const VBO& vbo, unsigned location_id);
        void attach_main_vbo_instanced(const VBO& vbo, unsigned location_id, unsigned divisor);
        void attach_vbo(const VBO& vbo, unsigned location_id) const;
        void attach_vbo_instanced(const VBO& vbo, unsigned location_id, unsigned divisor) const;
        void attach_vbos(const std::vector<VAO_attachment>& vbos, bool overwrite_count = false);
        void attach_vbos_instanced(const std::vector<Instanced_VAO_attachment>& vbos) const;
        void detach_vbo(unsigned location_id) const;
        void attach_ebo(const std::shared_ptr<Element_buffer>& ebo);
        void detach_ebo() const;
        void set_count(unsigned count);
        void draw(gl::draw mode, unsigned instances = 1, int count = -1, unsigned offset = 0,
                  unsigned base_instance = 0) const;
        void draw_elements(gl::draw mode, unsigned instances = 1, int count = -1,
                           unsigned offset = 0, unsigned base_instance = 0,
                           unsigned base_vertex = 0) const;
    private:
        void set_attrib_format(unsigned int index, GLint vertex_size, gl::v_type type) const;
        GLuint _name = 0;
        unsigned int _count = 0;
    };
}
