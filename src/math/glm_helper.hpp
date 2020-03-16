//
// Alexander Scheid-Rehder
// alexanderb@scheid-rehder.de
// https://www.alexsr.de
// https://github.com/alexsr
//

#pragma once
#pragma warning(push)
#pragma warning(disable: 4201)
#pragma warning(disable: 4127)
#define GLM_FORCE_CXX17
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>
#pragma warning(pop)

namespace tostf
{
    template <typename>
    struct is_glm_vec : std::false_type {};

    template <glm::length_t L, typename T, glm::qualifier Q>
    struct is_glm_vec<glm::vec<L, T, Q>> : std::true_type {};

    template <typename T>
    inline constexpr bool is_glm_vec_v = is_glm_vec<T>::value;
}
