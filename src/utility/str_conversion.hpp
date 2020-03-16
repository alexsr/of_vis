//
// Alexander Scheid-Rehder
// alexanderb@scheid-rehder.de
// https://www.alexsr.de
// https://github.com/alexsr
//

#pragma once

#include <sstream>

namespace tostf
{
    inline bool is_str_float(const std::string& in) {
        std::stringstream sstr(in);
        float f;
        return !((sstr >> std::noskipws >> f).rdstate() ^ std::ios_base::eofbit);
    }

    inline std::string precision_to_fmt_str(const int precision) {
        std::ostringstream output;
        output << "%." << precision << "f";
        return output.str();
    }

    inline std::string float_to_str_fixed_precision(const float input, const int precision = 3) {
        std::ostringstream output;
        output.precision(precision);
        output << std::fixed << input;
        return output.str();
    }

    inline std::string float_to_scientific_str(const float input, const int precision = 3) {
        std::ostringstream output;
        output.precision(precision);
        if (abs(input) > glm::pow(0.1f, precision)) {
            output << std::fixed << input;
        }
        else {
            output << std::scientific << input << std::fixed;
        }
        return output.str();
    }

    inline int get_float_exp(const float input) {
        // https://stackoverflow.com/questions/29582919/find-exponent-and-mantissa-of-a-double-to-the-base-10
        return input == 0.0f ? 0 : 1 + static_cast<int>(std::floor(std::log10(std::fabs(input))));
    }

    inline int get_float_exp_log(const float input) {
        const auto exponent = get_float_exp(input);
        if (exponent <= 0) {
            return static_cast<int>(glm::pow(10.0f, -static_cast<float>(exponent)));
        }
        return 1;
    }
}
