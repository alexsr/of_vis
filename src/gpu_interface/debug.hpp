//
// Alexander Scheid-Rehder
// alexanderb@scheid-rehder.de
// https://www.alexsr.de
// https://github.com/alexsr
//

#pragma once

#include <glad/glad.h>
#include <string>

namespace tostf
{
    namespace gl
    {
        namespace dbg
        {
            enum class source {
                api = GL_DEBUG_SOURCE_API,
                window = GL_DEBUG_SOURCE_WINDOW_SYSTEM,
                shader = GL_DEBUG_SOURCE_SHADER_COMPILER,
                third_party = GL_DEBUG_SOURCE_THIRD_PARTY,
                app = GL_DEBUG_SOURCE_APPLICATION,
                other = GL_DEBUG_SOURCE_OTHER,
                all = GL_DONT_CARE
            };

            enum class type {
                error = GL_DEBUG_TYPE_ERROR,
                deprecated = GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR,
                undefined = GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR,
                portability = GL_DEBUG_TYPE_PORTABILITY,
                performance = GL_DEBUG_TYPE_PERFORMANCE,
                marker = GL_DEBUG_TYPE_MARKER,
                push = GL_DEBUG_TYPE_PUSH_GROUP,
                pop = GL_DEBUG_TYPE_POP_GROUP,
                other = GL_DEBUG_TYPE_OTHER,
                all = GL_DONT_CARE
            };

            enum class severity {
                high = GL_DEBUG_SEVERITY_HIGH,
                medium = GL_DEBUG_SEVERITY_MEDIUM,
                low = GL_DEBUG_SEVERITY_LOW,
                notification = GL_DEBUG_SEVERITY_NOTIFICATION,
                all = GL_DONT_CARE
            };
        }

        class Debug_logger {
        public:
            explicit Debug_logger(dbg::source source = dbg::source::all, dbg::type type = dbg::type::all,
                                  dbg::severity severity = dbg::severity::all);
            void retrieve_log();
            void enable_source(dbg::source source);
            void enable_type(dbg::type type);
            void enable_severity(dbg::severity severity);
            void enable_messages(dbg::source source, dbg::type type, dbg::severity severity);
            void disable_source(dbg::source source);
            void disable_type(dbg::type type);
            void disable_severity(dbg::severity severity);
            void disable_messages(dbg::source source, dbg::type type, dbg::severity severity);
        private:
            void print_msg(GLenum source, GLenum type, GLuint id, GLenum severity, const std::string& message);
            unsigned int _count;
        };
    }
}
