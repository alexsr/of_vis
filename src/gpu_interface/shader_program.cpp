//
// Alexander Scheid-Rehder
// alexanderb@scheid-rehder.de
// https://www.alexsr.de
// https://github.com/alexsr
//

#include "shader_program.hpp"
#include "utility/logging.hpp"
#include "file/file_handling.hpp"
#include "math/glm_helper.hpp"

tostf::Shader::Shader(std::vector<Shader_define> defines,
                      const gl::introspection introspection_flag)
    : _introspection_flag(introspection_flag), _defines(std::move(defines)) {
    _program_id = glCreateProgram();
}

GLuint tostf::Shader::load_shader(const std::filesystem::path& path, const GLenum type) {
    log_info() << "Loading shader from file: " << path.string();
    auto f_source = load_file_str(path, std::ios::in | std::ios::binary);
    // Skip ahead of the version definition because it has to be at the top of the source file,
    // otherwise compilation of the shader might fail.
    const auto version_start = f_source.find("#version", 0);
    const auto start = f_source.find('\n', version_start) + 1;
    // Insert shader defines into the shader source.
    std::string define_string;
    for (auto& d : _defines) {
        define_string += "#define " + d.name + " " + d.value + "\n";
    }
    f_source.insert(start, define_string);

    // Includes are only allowed before the main function of the shader.
    const auto main_start = f_source.find("void main");
    process_includes(f_source, path, start, main_start);

    return compile_shader(f_source, type);
}

GLuint tostf::Shader::compile_shader(const std::string& source_str, const GLenum type) {
    const GLchar* source = source_str.data();
    auto size = static_cast<GLint>(source_str.size());
    const auto shader_id = glCreateShader(type);
    glShaderSource(shader_id, 1, &source, &size);
    glCompileShader(shader_id);
    GLint compile_status = 0;
    glGetShaderiv(shader_id, GL_COMPILE_STATUS, &compile_status);
    if (compile_status == GL_FALSE) {
        GLint log_size = 0;
        glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &log_size);
        std::string error_log;
        error_log.resize(static_cast<unsigned long>(log_size));
        glGetShaderInfoLog(shader_id, log_size, &log_size, error_log.data());
        glDeleteShader(shader_id);
        throw std::runtime_error{
            "Error while compiling "
            + gl::shader_type_to_string(type) + " from string.\n"
            + "Error log: \n"
            + error_log
        };
    }
    return shader_id;
}

void tostf::Shader::process_includes(std::string& file, const std::filesystem::path& directory,
                                     unsigned long long start, unsigned long long end) const {
    while ((start = file.find("#include", start)) <= end) {
        const auto path_start = file.find('\"', start);
        const auto path_end = file.find('\"', path_start + 1);
        auto path_inc(directory.parent_path() / file.substr(path_start + 1, path_end - path_start - 1));
        log_info() << "Including " << path_inc.string();
        const auto eol = file.find('\n', start);
        auto inc = load_file_str(path_inc);
        path_inc = canonical(path_inc);
        process_includes(inc, path_inc, 0, inc.size());
        // Remove line with #include and replace it with the contents of the included file.
        file.replace(start, eol - start, inc.data());
        // The end of the interval has to be pushed back because the length of the file changed
        // due to the included content.
        end += inc.size();
    }
}

tostf::Shader::~Shader() {
    glDeleteProgram(_program_id);
}

void tostf::Shader::use() const {
    glUseProgram(_program_id);
}

void tostf::Shader::link_program() const {
    glLinkProgram(_program_id);
    GLint link_status = 0;
    glGetProgramiv(_program_id, GL_LINK_STATUS, &link_status);
    if (link_status == GL_FALSE) {
        GLint length = 0;
        glGetProgramiv(_program_id, GL_INFO_LOG_LENGTH, &length);
        std::string error_log;
        error_log.resize(static_cast<unsigned long>(length));
        glGetProgramInfoLog(_program_id, length, &length, &error_log[0]);
        glDeleteProgram(_program_id);
        throw std::runtime_error{
            "Error while linking shader program "
            + gl::shader_type_to_string(_program_id) +
            ".\n"
            + "Error log: \n"
            + error_log
        };
    }
}

void tostf::Shader::init_shader_program(const std::vector<GLuint>& shader_ids) {
    attach_shaders(shader_ids);
    link_program();
    detach_shaders(shader_ids);
    if (_introspection_flag != gl::introspection::none) {
        inspect_uniforms();
    }
    // Shaders can be deleted when they are successfully attached to a program.
    delete_shaders(shader_ids);
}

void tostf::Shader::attach_shaders(const std::vector<GLuint>& shader_ids) const {
    for (auto id : shader_ids) {
        glAttachShader(_program_id, id);
    }
}

void tostf::Shader::detach_shaders(const std::vector<GLuint>& shader_ids) const {
    for (auto id : shader_ids) {
        glDetachShader(_program_id, id);
    };
}

void tostf::Shader::delete_shaders(const std::vector<GLuint>& shader_ids) {
    for (auto id : shader_ids) {
        glDeleteShader(id);
    };
}

void tostf::Shader::inspect_uniforms() {
    GLint uniform_count = 0;
    glGetProgramInterfaceiv(_program_id, GL_UNIFORM, GL_ACTIVE_RESOURCES, &uniform_count);
    std::array<GLenum, 4> properties{GL_TYPE, GL_LOCATION, GL_NAME_LENGTH, GL_BLOCK_INDEX};
    for (int i = 0; i < uniform_count; i++) {
        std::array<GLint, 4> info{};
        glGetProgramResourceiv(_program_id, GL_UNIFORM, i, 4, properties.data(), 4, nullptr, info.data());
        if (info.at(3) != -1) {
            continue; // skip block uniforms
        }
        // get uniform name
        std::vector<char> name_data(static_cast<unsigned long>(info.at(2)));
        glGetProgramResourceName(_program_id, GL_UNIFORM, i, static_cast<GLsizei>(name_data.size()),
                                 nullptr, name_data.data());
        const std::string name(name_data.begin(), name_data.end() - 1);

        _uniforms.emplace(name, info.at(1));
    }
}

tostf::V_shader::V_shader(std::filesystem::path vertex_path, const std::vector<Shader_define>& defines,
                          const gl::introspection introspection_flag)
    : Shader(defines, introspection_flag), _vertex_path(std::move(vertex_path)) {
    V_shader::reload();

}

void tostf::V_shader::reload() {
    _uniforms.clear();
    std::vector<GLuint> shader_ids(1);
    shader_ids.at(0) = load_shader(_vertex_path, GL_VERTEX_SHADER);
    init_shader_program(shader_ids);
}

tostf::V_F_shader::V_F_shader(std::filesystem::path vertex_path, std::filesystem::path fragment_path,
                              const std::vector<Shader_define>& defines, const gl::introspection introspection_flag)
    : Shader(defines, introspection_flag), _vertex_path(std::move(vertex_path)),
      _fragment_path(std::move(fragment_path)) {
    V_F_shader::reload();
}

void tostf::V_F_shader::reload() {
    _uniforms.clear();
    std::vector<GLuint> shader_ids(2);
    shader_ids.at(0) = load_shader(_vertex_path, GL_VERTEX_SHADER);
    shader_ids.at(1) = load_shader(_fragment_path, GL_FRAGMENT_SHADER);
    init_shader_program(shader_ids);
}

tostf::V_G_F_shader::V_G_F_shader(std::filesystem::path vertex_path, std::filesystem::path geometry_path,
                                  std::filesystem::path fragment_path, const std::vector<Shader_define>& defines,
                                  const gl::introspection introspection_flag)
    : Shader(defines, introspection_flag), _vertex_path(std::move(vertex_path)),
      _geometry_path(std::move(geometry_path)), _fragment_path(std::move(fragment_path)) {
    V_G_F_shader::reload();
}

void tostf::V_G_F_shader::reload() {
    _uniforms.clear();
    std::vector<GLuint> shader_ids(3);
    shader_ids.at(0) = load_shader(_vertex_path, GL_VERTEX_SHADER);
    shader_ids.at(1) = load_shader(_geometry_path, GL_GEOMETRY_SHADER);
    shader_ids.at(2) = load_shader(_fragment_path, GL_FRAGMENT_SHADER);
    init_shader_program(shader_ids);
}

tostf::V_T_F_shader::V_T_F_shader(std::filesystem::path vertex_path, std::filesystem::path control_path,
                                  std::filesystem::path evaluation_path, std::filesystem::path fragment_path,
                                  const std::vector<Shader_define>& defines, const gl::introspection introspection_flag)
    : Shader(defines, introspection_flag), _vertex_path(std::move(vertex_path)),
      _control_path(std::move(control_path)), _evaluation_path(std::move(evaluation_path)),
      _fragment_path(std::move(fragment_path)) {
    V_T_F_shader::reload();
}

void tostf::V_T_F_shader::reload() {
    _uniforms.clear();
    std::vector<GLuint> shader_ids(4);
    shader_ids.at(0) = load_shader(_vertex_path, GL_VERTEX_SHADER);
    shader_ids.at(1) = load_shader(_control_path, GL_TESS_CONTROL_SHADER);
    shader_ids.at(2) = load_shader(_evaluation_path, GL_TESS_EVALUATION_SHADER);
    shader_ids.at(3) = load_shader(_fragment_path, GL_FRAGMENT_SHADER);
    init_shader_program(shader_ids);
}

tostf::V_T_G_F_shader::V_T_G_F_shader(std::filesystem::path vertex_path, std::filesystem::path control_path,
                                      std::filesystem::path evaluation_path, std::filesystem::path geometry_path,
                                      std::filesystem::path fragment_path, const std::vector<Shader_define>& defines,
                                      const gl::introspection introspection_flag)
    : Shader(defines, introspection_flag), _vertex_path(std::move(vertex_path)),
      _control_path(std::move(control_path)), _evaluation_path(std::move(evaluation_path)),
      _geometry_path(std::move(geometry_path)), _fragment_path(std::move(fragment_path)) {
    V_T_G_F_shader::reload();
}

void tostf::V_T_G_F_shader::reload() {
    _uniforms.clear();
    std::vector<GLuint> shader_ids(5);
    shader_ids.at(0) = load_shader(_vertex_path, GL_VERTEX_SHADER);
    shader_ids.at(1) = load_shader(_control_path, GL_TESS_CONTROL_SHADER);
    shader_ids.at(2) = load_shader(_evaluation_path, GL_TESS_EVALUATION_SHADER);
    shader_ids.at(3) = load_shader(_geometry_path, GL_GEOMETRY_SHADER);
    shader_ids.at(4) = load_shader(_fragment_path, GL_FRAGMENT_SHADER);
    init_shader_program(shader_ids);
}

tostf::Compute_shader::Compute_shader(std::filesystem::path compute_path, const std::vector<Shader_define>& defines,
                                      const gl::introspection introspection_flag)
    : Shader(defines, introspection_flag), _compute_path(std::move(compute_path)) {
    Compute_shader::reload();
}

void tostf::Compute_shader::reload() {
    _uniforms.clear();
    std::vector<GLuint> shader_ids;
    shader_ids.push_back(load_shader(_compute_path, GL_COMPUTE_SHADER));
    init_shader_program(shader_ids);
    glGetProgramiv(_program_id, GL_COMPUTE_WORK_GROUP_SIZE, _workgroup_size.data());
}

void tostf::Compute_shader::run(const unsigned x, const unsigned y, const unsigned z) const {
    use();
    glDispatchCompute(static_cast<GLuint>(glm::ceil(static_cast<float>(x) / _workgroup_size.at(0))),
                      static_cast<GLuint>(glm::ceil(static_cast<float>(y) / _workgroup_size.at(1))),
                      static_cast<GLuint>(glm::ceil(static_cast<float>(z) / _workgroup_size.at(2))));
}

void tostf::Compute_shader::run_with_barrier(const unsigned x, const unsigned y, const unsigned z,
                                             const GLbitfield barriers) const {
    use();
    glDispatchCompute(static_cast<GLuint>(glm::ceil(static_cast<float>(x) / _workgroup_size.at(0))),
                      static_cast<GLuint>(glm::ceil(static_cast<float>(y) / _workgroup_size.at(1))),
                      static_cast<GLuint>(glm::ceil(static_cast<float>(z) / _workgroup_size.at(2))));
    glMemoryBarrier(barriers);
}

void tostf::Compute_shader::run_workgroups(const unsigned x, const unsigned y, const unsigned z) const {
    use();
    glDispatchCompute(x, y, z);
}

void tostf::Compute_shader::run_workgroups_with_barrier(const unsigned x, const unsigned y, const unsigned z,
                                                        const GLbitfield barriers) const {
    use();
    glDispatchCompute(x, y, z);
    glMemoryBarrier(barriers);
}

std::array<GLint, 3> tostf::Compute_shader::get_workgroup_size() const {
    return _workgroup_size;
}

GLint tostf::Compute_shader::get_workgroup_size_x() const {
    return _workgroup_size.at(0);
}

GLint tostf::Compute_shader::get_workgroup_size_y() const {
    return _workgroup_size.at(1);
}

GLint tostf::Compute_shader::get_workgroup_size_z() const {
    return _workgroup_size.at(2);
}
