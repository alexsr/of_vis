//
// Alexander Scheid-Rehder
// alexanderb@scheid-rehder.de
// https://www.alexsr.de
// https://github.com/alexsr
//

#define IMGUI_DEFINE_MATH_OPERATORS
#include <geometry/mesh_handling.hpp>
#include <core/gpu_interface.hpp>
#include <core/display.hpp>
#include <file/file_handling.hpp>
#include <utility/logging.hpp>
#include <utility/event_profiler.hpp>
#include <gpu_interface/storage_buffer.hpp>
#include <gpu_interface/fbo.hpp>
#include <display/tostf_imgui_widgets.hpp>
#include <display/view.hpp>
#include <mesh_processing/halfedge_mesh.hpp>
#include <mesh_processing/tri_face_mesh.hpp>
#include <utility/random.hpp>
#include "imgui/imgui_internal.h"
#include "core/shader_settings.hpp"
#include "math/distribution.hpp"
#include "assimp/postprocess.h"
#include "aneurysm/aneurysm_viewer.hpp"
#include "mesh_processing/structured_mesh.hpp"
#include <preprocessing/inlet_detection.hpp>
#include <preprocessing/mesh_deformation.hpp>
#include <preprocessing/openfoam_exporter.hpp>
#include "iconfont/IconsMaterialDesignIcons_c.h"
#include "glm/gtx/component_wise.hpp"
#include "visualization/vis_utilities.hpp"
#include "gpu_interface/debug.hpp"

int main(int argc, char** argv) {
    tostf::cmd::enable_color();
    if (argc == 2) {
        std::string ref_path(argv[1]);
        auto reference_mesh = tostf::load_single_mesh(ref_path, 0, true, aiProcess_PreTransformVertices).value();
        float volume = 0.0f;
        for (int id = 0; id < static_cast<int>(reference_mesh->indices.size()); id += 3) {
            // Volume calc http://chenlab.ece.cornell.edu/Publication/Cha/icip01_Cha.pdf
            auto a = reference_mesh->vertices.at(reference_mesh->indices.at(id));
            auto b = reference_mesh->vertices.at(reference_mesh->indices.at(id + 2));
            auto c = reference_mesh->vertices.at(reference_mesh->indices.at(id + 1));
            const auto v321 = c.x * b.y * a.z;
            const auto v231 = b.x * c.y * a.z;
            const auto v312 = c.x * a.y * b.z;
            const auto v132 = a.x * c.y * b.z;
            const auto v213 = b.x * a.y * c.z;
            const auto v123 = a.x * b.y * c.z;
            volume += (1.0f / 6.0f) * (-v321 + v231 + v312 - v132 - v213 + v123);
        }

        std::stringstream ss;
        ss << abs(volume);
        tostf::save_str_to_file("volume.txt", ss.str());
    }
    return 0;
}
