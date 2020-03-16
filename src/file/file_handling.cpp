//
// Alexander Scheid-Rehder
// alexanderb@scheid-rehder.de
// https://www.alexsr.de
// https://github.com/alexsr
//

#include "file_handling.hpp"
#include <stdexcept>
#include <tinyfd/tinyfiledialogs.h>
#include <fstream>

void tostf::check_file_validity(const std::filesystem::path& path) {
    if (!exists(path)) {
        throw std::runtime_error{"File at " + path.string() + " not found."};
    }
    if (is_empty(path)) {
        throw std::runtime_error{"File at " + path.string() + " is empty."};
    }
}

std::string tostf::load_file_str(const std::filesystem::path& path, const std::ios_base::openmode mode) {
    check_file_validity(path);
    std::ifstream in;
    in.exceptions(std::ifstream::badbit);
    in.open(path, mode);
    if (!in.is_open()) {
        throw std::runtime_error{"Error loading file from " + path.string()};
    }
    const auto size = file_size(path);
    std::string buffer(size, ' ');
    in.read(buffer.data(), size);
    in.close();
    return buffer;
}

const char* tostf::load_file_cstr(const std::filesystem::path& path, const std::ios_base::openmode mode) {
    check_file_validity(path);
    std::ifstream in;
    in.exceptions(std::ifstream::badbit);
    in.open(path, mode);
    if (!in.is_open()) {
        throw std::runtime_error{"Error loading file from " + path.string()};
    }
    const auto size = file_size(path);
    std::string buffer(size, ' ');
    in.read(buffer.data(), size);
    in.close();
    return buffer.c_str();
}

void tostf::save_str_to_file(const std::filesystem::path& path, const std::string& str,
                             const std::ios_base::openmode mode) {
    std::ofstream out;
    out.exceptions(std::ifstream::badbit);
    out.open(path, mode);
    if (!out.is_open()) {
        throw std::runtime_error{"Error saving file to " + path.string()};
    }
    out << str;
    out.close();
}

std::optional<std::filesystem::path> tostf::show_open_file_dialog(const std::string& title,
                                                                  const std::filesystem::path& dir_path,
                                                                  const std::vector<const char*>& filetypes) {
    if (!exists(dir_path)) {
        throw std::runtime_error{"Path " + dir_path.string() + " not found."};
    }
    const auto file_path = tinyfd_openFileDialog(title.c_str(), absolute(dir_path).string().c_str(),
                                                 static_cast<int>(filetypes.size()), filetypes.data(),
                                                 nullptr, 0);
    if (file_path == nullptr) {
        return {};
    }
    return file_path;
}

std::optional<std::filesystem::path> tostf::show_save_file_dialog(const std::string& title,
                                                                  const std::filesystem::path& dir_path,
                                                                  const std::vector<const char*>& filetypes) {
    if (!exists(dir_path)) {
        throw std::runtime_error{"Path " + dir_path.string() + " not found."};
    }
    const auto file_path = tinyfd_saveFileDialog(title.c_str(), absolute(dir_path).string().c_str(),
                                                 static_cast<int>(filetypes.size()), filetypes.data(), nullptr);
    if (file_path == nullptr) {
        return {};
    }
    return file_path;
}

std::optional<std::filesystem::path> tostf::show_select_folder_dialog(const std::string& title,
                                                                      const std::filesystem::path& dir_path) {
    if (!exists(dir_path)) {
        throw std::runtime_error{"Path " + dir_path.string() + " not found."};
    }
    const auto file_path = tinyfd_selectFolderDialog(title.c_str(), absolute(dir_path).string().c_str());
    if (file_path == nullptr) {
        return {};
    }
    return file_path;
}
