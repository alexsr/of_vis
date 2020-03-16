//
// Alexander Scheid-Rehder
// alexanderb@scheid-rehder.de
// https://www.alexsr.de
// https://github.com/alexsr
//

#include "logging.hpp"
#include <utility>
#include <fstream>
#include <iostream>

tostf::Logger::Logger(const bool will_be_moved) : _to_file(false), _will_be_moved(will_be_moved) {}

tostf::Logger::Logger(std::filesystem::path out, const bool moved)
    : _to_file(true), _will_be_moved(moved), _out(std::move(out)) {}

tostf::Logger::Logger(Logger&&) noexcept : _to_file(false), _will_be_moved(false) {}

tostf::Logger& tostf::Logger::operator=(Logger&&) noexcept {
    _will_be_moved = false;
    return *this;
}

tostf::Logger::~Logger() {
    if (_to_file) {
        if (!_will_be_moved) {
            _oss << "\n";
        }
        std::ofstream file(_out, std::ios::ate);
        file << _oss.str();
        file.close();
    }
    else {
        if (!_will_be_moved) {
            _oss << "\n";
        }
        _oss << cmd::reset_style();
        std::cout << _oss.str();
    }
}

tostf::Logger tostf::log() {
    return Logger();
}

tostf::Logger tostf::log(const cmd::color c, const cmd::style s, const cmd::bg_color b) {
    return std::move(Logger(true) << c << s << b);
}

tostf::Logger tostf::log(const std::filesystem::path& p) {
    return Logger(p);
}

tostf::Logger tostf::log_tag(const std::string& tag, const cmd::bg_color bg_color, size_t max_length) {
    auto tag_length = tag.size();
    if (max_length == 0) {
        max_length = tag_length;
    }
    if (tag_length >= max_length) {
        tag_length = max_length;
    }
    return std::move(Logger(true) << bg_color << cmd::color::h_white << " " << tag.substr(0, tag_length) <<
                     std::string(max_length - tag_length + 1, ' ') << cmd::reset_style() << " ");
}

tostf::Logger tostf::log_info() {
    return log_tag("INFO", cmd::bg_color::cyan);
}

tostf::Logger tostf::log_error() {
    return log_tag("ERROR", cmd::bg_color::h_red);
}

tostf::Logger tostf::log_success() {
    return log_tag("SUCCESS", cmd::bg_color::h_green);
}

tostf::Logger tostf::log_warning() {
    return log_tag("WARNING", cmd::bg_color::yellow);
}

tostf::Logger tostf::log_time() {
    return log_tag("TIME", cmd::bg_color::h_purple);
}

void tostf::log_section(const std::string& title, const cmd::bg_color bg_color, const cmd::color color,
                        size_t max_length) {
    auto title_length = title.size();
    if (max_length == 0) {
        max_length = title_length;
    }
    if (title_length >= max_length) {
        title_length = max_length;
    }
    log() << bg_color << color << "\n                    " << title.substr(0, title_length) <<
        std::string(max_length - title_length, ' ');
}

void tostf::log_section_no_newline(const std::string& title, const cmd::bg_color bg_color, const cmd::color color,
                                   size_t max_length) {

    auto title_length = title.size();
    if (max_length == 0) {
        max_length = title_length;
    }
    if (title_length >= max_length) {
        title_length = max_length;
    }
    log() << bg_color << color << "                    " << title.substr(0, title_length) <<
        std::string(max_length - title_length, ' ');
}
