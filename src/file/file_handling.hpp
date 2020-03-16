//
// Alexander Scheid-Rehder
// alexanderb@scheid-rehder.de
// https://www.alexsr.de
// https://github.com/alexsr
//

#pragma once

#include <filesystem>
#include <optional>

namespace tostf
{
    void check_file_validity(const std::filesystem::path& path);
    std::string load_file_str(const std::filesystem::path& path, std::ios_base::openmode mode = std::ios::in);
    const char* load_file_cstr(const std::filesystem::path& path, std::ios_base::openmode mode = std::ios::in);
    void save_str_to_file(const std::filesystem::path& path, const std::string& str,
                          std::ios_base::openmode mode = std::ios::out | std::ios::trunc | std::ios::binary);
    std::optional<std::filesystem::path> show_open_file_dialog(const std::string& title,
                                                               const std::filesystem::path& dir_path,
                                                               const std::vector<const char*>& filetypes = {});
    std::optional<std::filesystem::path> show_save_file_dialog(const std::string& title,
                                                               const std::filesystem::path& dir_path,
                                                               const std::vector<const char*>& filetypes = {});
    std::optional<std::filesystem::path> show_select_folder_dialog(const std::string& title,
                                                                   const std::filesystem::path& dir_path);
}
