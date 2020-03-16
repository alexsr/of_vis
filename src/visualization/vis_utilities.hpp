//
// Alexander Scheid-Rehder
// alexanderb@scheid-rehder.de
// https://www.alexsr.de
// https://github.com/alexsr
//

#pragma once

#include "gpu_interface/texture.hpp"
#include "utility/logging.hpp"
#include <map>
#include "utility/str_conversion.hpp"

namespace tostf
{
    struct Colormap {
        std::unique_ptr<Texture2D> tex{};
    };

    constexpr std::array<const char*, 38> default_colormaps{
        "Accent", "Accent_r", "afmhot", "afmhot_r", "binary", "binary_r", "bwr", "bwr_r", "CMRmap", "CMRmap_r",
        "coolwarm", "coolwarm_r", "gist_rainbow", "gist_rainbow_r", "gnuplot", "gnuplot2", "gnuplot2_r", "gnuplot_r",
        "inferno", "inferno_r", "jet", "jet_r", "magma", "magma_r", "plasma", "plasma_r", "RdBu", "RdBu_r", "RdPu",
        "RdPu_r", "RdYlBu", "RdYlBu_r", "seismic", "seismic_r", "Spectral", "Spectral_r", "viridis", "viridis_r"
    };

    struct Colormap_handler {
        explicit Colormap_handler(const std::filesystem::path& folder) {
            for (auto& p : std::filesystem::directory_iterator(folder)) {
                auto filename = p.path().stem().string();
                if (p.is_regular_file() && std::find(default_colormaps.begin(), default_colormaps.end(),
                                                     filename) != default_colormaps.end()) {
                    add_or_overwrite_colormap(p.path());
                }
            }
            curr_map_it = maps.begin();
        }

        void add_colormap_folder(const std::filesystem::path& folder) {
            for (auto& p : std::filesystem::directory_iterator(folder)) {
                if (p.is_regular_file()) {
                    add_or_overwrite_colormap(p.path());
                }
            }
        }

        void add_or_overwrite_colormap(const std::filesystem::path& file) {
            if (file.has_filename() && (file.extension() == ".png" || file.extension() == ".jpg")) {
                const auto filename = file.stem().string();
                const auto insertion = maps.insert_or_assign(filename,
                                                             Colormap{
                                                                 std::make_unique<Texture2D>(file)
                                                             });
                max_name_length = glm::max(max_name_length, static_cast<int>(filename.size()));
                if (insertion.second) {
                    log_info() << "Colormap " << file.stem().string() << " added.";
                }
                else {
                    log_warning() << "Colormap " << file.stem().string() << " overwritten.";
                }
            }
        }

        std::map<std::string, Colormap> maps;
        int max_name_length = 0;
        std::map<std::string, Colormap>::iterator curr_map_it;
    };

    struct Legend {
        void init_precision(const float min_v, const float max_v) {
            const auto max_precision = get_float_exp(max_v);
            const auto min_precision = get_float_exp(min_v);
            if (max_precision != 0.0f && min_precision != 0.0f) {
                precision = glm::min(max_precision, min_precision);
            }
            else if (max_precision == 0.0f) {
                precision = min_precision;
            }
            else {
                precision = max_precision;
            }
        }
        void init(const float min_v, const float max_v) {
            min_value = min_v;
            max_value = max_v;
            const auto exp_10 = glm::pow(10, -precision + 1.0f);
            steps.resize(3);
            const auto offset = glm::floor(min_v);
            const auto min_moved = min_v - offset;
            const auto max_moved = max_v - offset;
            const auto step_size = (max_moved - min_moved) / static_cast<float>(step_count + 1.5f);
            steps.at(0) = min_v;
            steps.at(1) = min_v < 0 && max_v > 0 ? 0.0f : floor((max_v - min_v) / 2.0f * exp_10) / exp_10;
            steps.at(steps.size() - 1) = max_v;
            /*for (int i = 0; i < step_count; ++i) {
                const auto step_pos = min_moved + step_size + step_size * i;
                float exp_step_pos = exp_10 * step_pos;
                if (exp_step_pos < glm::ceil(exp_step_pos)) {
                    exp_step_pos = glm::floor(exp_step_pos) + 1.0f;
                }
                const auto result = exp_step_pos / static_cast<float>(exp_10) + offset;
                if (result < max_v) {
                    steps.at(i + 1) = result;
                }
                else {
                    steps.at(i + 1) = min_v;
                }
            }*/
            std::sort(steps.begin(), steps.end());
            steps.erase(std::unique(steps.begin(), steps.end()), steps.end());
        }

        void set_height(const float h) {
            height = h;
        }

        float get_precision() const {
            return glm::min(3, precision);
        }

        float value_to_height(const float step) const {
            return (step - min_value) / (max_value - min_value) * height;
        }

        float height = 123.0f;
        float line_length = 30.0f;
        float min_value = 0.0f;
        float max_value = 0.0f;
        int step_count = 2;
        int precision{};
        std::vector<float> steps;
    };

    struct Player {
        int get_max_step_id() const {
            return static_cast<int>(steps.size()) - 1;
        }

        void play() {
            _playing = true;
            if (current_step >= get_max_step_id()) {
                current_step = 0;
            }
        }

        void pause() {
            _playing = false;
            _accumulator = 0.0f;
        }

        bool is_playing() const {
            return _playing;
        }

        void stop() {
            current_step = 0;
            _playing = false;
            _accumulator = 0.0f;
        }

        void toggle_repeat() {
            _repeating = !_repeating;
        }

        void set_repeat(const bool v) {
            _repeating = v;
        }

        bool is_repeating() const {
            return _repeating;
        }

        bool skip_next() {
            if (current_step + 1 < static_cast<int>(steps.size())) {
                ++current_step;
                return true;
            }
            return false;
        }

        bool skip_previous() {
            if (current_step - 1 >= 0) {
                --current_step;
                return true;
            }
            return false;
        }

        void advance_accumulator(const float dt) {
            _accumulator += dt;
            if (_accumulator > time_per_step) {
                _accumulator = 0.0f;
                if (!skip_next()) {
                    if (_repeating) {
                        stop();
                        play();
                    }
                    else {
                        pause();
                    }
                }
            }
        }

        int get_current_step() const {
            return current_step;
        }

        float progress() const {
            if (steps.empty()) {
                return 0.0f;
            }
            return static_cast<float>(current_step) / (static_cast<int>(steps.size()) - 1);
        }

        std::string get_progress_str() const {
            if (steps.empty()) {
                return "";
            }
            std::stringstream str;
            str << steps.at(current_step) << "/" << steps.at(get_max_step_id());
            return str.str();
        }

        int current_step = 0;
        float time_per_step = 0.5f;
        std::vector<std::string> steps;
    private:
        bool _playing = false;
        float _accumulator = 0.0f;
        bool _repeating = false;
    };
}
