//
// Alexander Scheid-Rehder
// alexanderb@scheid-rehder.de
// https://www.alexsr.de
// https://github.com/alexsr
//

#include "debug.hpp"
#include "utility/logging.hpp"
#include <vector>
#include "utility/utility.hpp"

tostf::gl::Debug_logger::Debug_logger(const dbg::source source, const dbg::type type, const dbg::severity severity)
    : _count(1) {
    GLint context_flags;
    glGetIntegerv(GL_CONTEXT_FLAGS, &context_flags);
    if ((context_flags & GL_CONTEXT_FLAG_DEBUG_BIT) != 0) {
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        enable_messages(source, type, severity);
    }
    else {
        log_warning() << "Debugging is not enabled.";
    }
}

void tostf::gl::Debug_logger::retrieve_log() {
    GLint msg_count = 0;
    glGetIntegerv(GL_DEBUG_LOGGED_MESSAGES, &msg_count);
    GLint max_msg_length = 0;
    glGetIntegerv(GL_MAX_DEBUG_MESSAGE_LENGTH, &max_msg_length);
    std::vector<GLchar> message_data(static_cast<unsigned long>(msg_count * max_msg_length));
    std::vector<GLenum> sources(static_cast<unsigned long>(msg_count));
    std::vector<GLenum> types(static_cast<unsigned long>(msg_count));
    std::vector<GLenum> severities(static_cast<unsigned long>(msg_count));
    std::vector<GLuint> ids(static_cast<unsigned long>(msg_count));
    std::vector<GLsizei> lengths(static_cast<unsigned long>(msg_count));
    const auto actual_msg_count = glGetDebugMessageLog(static_cast<GLuint>(msg_count),
                                                       static_cast<GLsizei>(message_data.size()),
                                                       sources.data(), types.data(), ids.data(),
                                                       severities.data(), lengths.data(),
                                                       message_data.data());
    auto data_it = message_data.begin();
    if (actual_msg_count > 0) {
        log_section("OpenGL debug messages " + std::to_string(_count) + " to "
                    + std::to_string(_count + actual_msg_count - 1),
                    cmd::bg_color::h_red, cmd::color::h_white);
        for (unsigned int i = 0; i < actual_msg_count; i++) {
            print_msg(sources.at(i), types.at(i), ids.at(i), severities.at(i),
                      std::string(data_it, data_it + lengths.at(i) - 1));
            data_it = data_it + lengths.at(i);
            _count++;
        }
    }
}

void tostf::gl::Debug_logger::enable_source(const dbg::source source) {
    glDebugMessageControl(to_underlying_type(source), GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
}

void tostf::gl::Debug_logger::enable_type(const dbg::type type) {
    glDebugMessageControl(GL_DONT_CARE, to_underlying_type(type), GL_DONT_CARE, 0, nullptr, GL_TRUE);
}

void tostf::gl::Debug_logger::enable_severity(const dbg::severity severity) {
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, to_underlying_type(severity), 0, nullptr, GL_TRUE);
}

void tostf::gl::Debug_logger::enable_messages(const dbg::source source, const dbg::type type,
                                              const dbg::severity severity) {
    glDebugMessageControl(to_underlying_type(source), to_underlying_type(type),
                          to_underlying_type(severity), 0, nullptr, GL_TRUE);
}

void tostf::gl::Debug_logger::disable_source(const dbg::source source) {
    glDebugMessageControl(to_underlying_type(source), GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_FALSE);
}

void tostf::gl::Debug_logger::disable_type(const dbg::type type) {
    glDebugMessageControl(GL_DONT_CARE, to_underlying_type(type), GL_DONT_CARE, 0, nullptr, GL_FALSE);
}
void tostf::gl::Debug_logger::disable_severity(const dbg::severity severity) {
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, to_underlying_type(severity), 0, nullptr, GL_FALSE);
}

void tostf::gl::Debug_logger::disable_messages(const dbg::source source, const dbg::type type,
                                               const dbg::severity severity) {
    glDebugMessageControl(to_underlying_type(source), to_underlying_type(type),
                          to_underlying_type(severity), 0, nullptr, GL_FALSE);
}

void tostf::gl::Debug_logger::print_msg(const GLenum source, const GLenum type, const GLuint id, const GLenum severity,
                                        const std::string& message) {
    std::string source_msg;
    std::string type_msg;
    std::string severity_msg;
    auto msg_color = cmd::color::h_black;
    switch (source) {
        case GL_DEBUG_SOURCE_API:
            source_msg = "API";
            break;
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
            source_msg = "window system";
            break;
        case GL_DEBUG_SOURCE_SHADER_COMPILER:
            source_msg = "shader compiler";
            break;
        case GL_DEBUG_SOURCE_THIRD_PARTY:
            source_msg = "third party";
            break;
        case GL_DEBUG_SOURCE_APPLICATION:
            source_msg = "application";
            break;
        default:
            source_msg = "other";
            break;
    }
    switch (type) {
        case GL_DEBUG_TYPE_ERROR:
            type_msg = "error";
            break;
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
            type_msg = "deprecated behavior";
            break;
        case GL_DEBUG_TYPE_MARKER:
            type_msg = "marker";
            break;
        case GL_DEBUG_TYPE_PERFORMANCE:
            type_msg = "performance";
            break;
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
            type_msg = "undefined behavior";
            break;
        case GL_DEBUG_TYPE_PORTABILITY:
            type_msg = "portability";
            break;
        case GL_DEBUG_TYPE_PUSH_GROUP:
            type_msg = "push group";
            break;
        case GL_DEBUG_TYPE_POP_GROUP:
            type_msg = "pop group";
            break;
        default:
            type_msg = "other";
            break;
    }
    switch (severity) {
        case GL_DEBUG_SEVERITY_LOW:
            severity_msg = "low";
            msg_color = cmd::color::h_white;
            break;
        case GL_DEBUG_SEVERITY_MEDIUM:
            severity_msg = "medium";
            msg_color = cmd::color::h_yellow;
            break;
        case GL_DEBUG_SEVERITY_HIGH:
            severity_msg = "high";
            msg_color = cmd::color::h_red;
            break;
        case GL_DEBUG_SEVERITY_NOTIFICATION:
        default:
            severity_msg = "notification";
            break;
    }
    log_tag("DBG_MSG (" + std::to_string(id) + ")", cmd::bg_color::yellow, 0) << msg_color << message;
    log_tag("SOURCE", cmd::bg_color::purple) << msg_color << source_msg;
    log_tag("TYPE", cmd::bg_color::purple) << msg_color << type_msg;
    log_tag("SEVERITY", cmd::bg_color::purple) << msg_color << severity_msg;
}
