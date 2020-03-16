//
// Alexander Scheid-Rehder
// alexanderb@scheid-rehder.de
// https://www.alexsr.de
// https://github.com/alexsr
//

#pragma once

#include "shader_util.hpp"
#include "uniform.hpp"
#include "utility/logging.hpp"
#include <map>
#include <array>
#include <filesystem>

namespace tostf
{
    // Shader defines are used to provide name value pairs which are passed to a shader object
    // in order to add "#define name value" strings to the shader source.
    struct Shader_define {
        template <typename T>
        Shader_define(const std::string& define_name, T define_value) {
            name = define_name;
            value = std::to_string(define_value);
        }

        std::string name;
        std::string value;
    };

    // Base class of all Shader program types providing basic functionality such as
    // uniform update methods and basic initialization functions for loading, attaching
    // and linking shaders to the shader program.
    // It is also possible to include code into the shader source by using "#include relative-path-to-file".
    // The name shader might be a bit confusing as this actually provides the functionality of
    // an OpenGL shader program rather than the shader but the benefits of a shorter name are more important.
    class Shader {
    public:
        Shader(const Shader&) = default;
        Shader& operator=(const Shader&) = default;
        Shader(Shader&&) = default;
        Shader& operator=(Shader&&) = default;
        virtual ~Shader();
        void use() const;
        virtual void reload() = 0;
        template <typename T>
        void update_uniform(const std::string& name, const T& v) const;
    protected:
        explicit Shader(std::vector<Shader_define> defines = {},
                        gl::introspection introspection_flag = gl::introspection::basic);
        // Loads shader source from file, processes the shader includes and compiles the shader.
        // Throws an exception if the compilation fails.
        GLuint load_shader(const std::filesystem::path& path, GLenum type);
        static GLuint compile_shader(const std::string& source_str, GLenum type);
        // Recursively processes all the includes in the current shader source file.
        // The contents of the included files are inserted into the string file which is passed by reference.
        // Therefore file is both input as well as output of this method.
        // By passing the directory of the file currently processed to this function it is able
        // to process relative paths in included files.
        // start and end_search both provide the interval in which to look for includes.
        void process_includes(std::string& file, const std::filesystem::path& directory,
                              unsigned long long start, unsigned long long end) const;
        // Links the shaders of the shader program. This might throw an exception in case of a linker error.
        void link_program() const;
        void init_shader_program(const std::vector<GLuint>& shader_ids);
        void attach_shaders(const std::vector<GLuint>& shader_ids) const;
        void detach_shaders(const std::vector<GLuint>& shader_ids) const;
        static void delete_shaders(const std::vector<GLuint>& shader_ids);
        // Uses program introspection to find all uniforms of the shader program
        // excluding uniform blocks. The uniforms found by this are added to their respective
        // maps in this shader program.
        void inspect_uniforms();
        GLuint _program_id;
        gl::introspection _introspection_flag;
        std::vector<Shader_define> _defines;
        std::map<std::string, Uniform> _uniforms;
    };

    template <typename T>
    void Shader::update_uniform(const std::string& name, const T& v) const {
        if (_uniforms.find(name) != _uniforms.end()) {
            _uniforms.at(name).update(v, _program_id);
        }
        else {
            log_error() << "Uniform " << name << " not found.";
        }
    }

    // Creates a vertex shader only shader program.
    class V_shader : public Shader {
    public:
        explicit V_shader(std::filesystem::path vertex_path,
                          const std::vector<Shader_define>& defines = {},
                          gl::introspection introspection_flag = gl::introspection::basic);
        void reload() override;
    private:
        std::filesystem::path _vertex_path;
    };

    // Creates a vertex and fragment shader program.
    class V_F_shader : public Shader {
    public:
        V_F_shader(std::filesystem::path vertex_path,
                   std::filesystem::path fragment_path,
                   const std::vector<Shader_define>& defines = {},
                   gl::introspection introspection_flag = gl::introspection::basic);
        void reload() override;
    private:
        std::filesystem::path _vertex_path;
        std::filesystem::path _fragment_path;
    };

    // Creates a vertex, geometry and fragment shader program.
    class V_G_F_shader : public Shader {
    public:
        V_G_F_shader(std::filesystem::path vertex_path,
                     std::filesystem::path geometry_path,
                     std::filesystem::path fragment_path,
                     const std::vector<Shader_define>& defines = {},
                     gl::introspection introspection_flag = gl::introspection::basic);
        void reload() override;
    private:
        std::filesystem::path _vertex_path;
        std::filesystem::path _geometry_path;
        std::filesystem::path _fragment_path;
    };

    // Creates a vertex, tesselation control, tesselation evaluation and fragment shader program.
    class V_T_F_shader : public Shader {
    public:
        V_T_F_shader(std::filesystem::path vertex_path,
                     std::filesystem::path control_path,
                     std::filesystem::path evaluation_path,
                     std::filesystem::path fragment_path,
                     const std::vector<Shader_define>& defines = {},
                     gl::introspection introspection_flag = gl::introspection::basic);
        void reload() override;
    private:
        std::filesystem::path _vertex_path;
        std::filesystem::path _control_path;
        std::filesystem::path _evaluation_path;
        std::filesystem::path _fragment_path;
    };

    // Creates a vertex, tesselation control, tesselation evaluation, geometry and fragment shader program.
    class V_T_G_F_shader : public Shader {
    public:
        V_T_G_F_shader(std::filesystem::path vertex_path,
                       std::filesystem::path control_path,
                       std::filesystem::path evaluation_path,
                       std::filesystem::path geometry_path,
                       std::filesystem::path fragment_path,
                       const std::vector<Shader_define>& defines = {},
                       gl::introspection introspection_flag = gl::introspection::basic);
        void reload() override;
    private:
        std::filesystem::path _vertex_path;
        std::filesystem::path _control_path;
        std::filesystem::path _evaluation_path;
        std::filesystem::path _geometry_path;
        std::filesystem::path _fragment_path;
    };

    class Compute_shader : public Shader {
    public:
        explicit Compute_shader(std::filesystem::path compute_path,
                                const std::vector<Shader_define>& defines = {},
                                gl::introspection introspection_flag = gl::introspection::basic);
        void reload() override;
        void run(unsigned int x = 1, unsigned int y = 1, unsigned int z = 1) const;
        // Enables specified memory barrier before running compute shader.
        void run_with_barrier(unsigned int x = 1, unsigned int y = 1, unsigned int z = 1,
                              GLbitfield barriers = GL_SHADER_STORAGE_BARRIER_BIT) const;
        void run_workgroups(unsigned int x = 1, unsigned int y = 1, unsigned int z = 1) const;
        // Enables specified memory barrier before running compute shader.
        void run_workgroups_with_barrier(unsigned int x = 1, unsigned int y = 1, unsigned int z = 1,
                                         GLbitfield barriers = GL_SHADER_STORAGE_BARRIER_BIT) const;
        std::array<GLint, 3> get_workgroup_size() const;
        GLint get_workgroup_size_x() const;
        GLint get_workgroup_size_y() const;
        GLint get_workgroup_size_z() const;
    private:
        std::filesystem::path _compute_path;
        std::array<GLint, 3> _workgroup_size{};
    };
}
