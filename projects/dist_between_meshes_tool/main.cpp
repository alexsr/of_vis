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
    if (argc == 3) {
        std::string ref_path(argv[1]);
        std::string comp_path(argv[2]);
        auto reference_mesh = tostf::load_single_mesh(ref_path, 0, true, aiProcess_OptimizeGraph).value();
        auto comp_mesh = tostf::load_single_mesh(comp_path, 0, true, aiProcess_OptimizeGraph).value();
        std::vector<float> distances(reference_mesh->vertices.size());
#pragma omp parallel for
        for (int p_id = 0; p_id < static_cast<int>(distances.size()); ++p_id) {
			float min_dist = FLT_MAX;
			for (int id = 0; id < static_cast<int>(distances.size()); ++id) {
				const auto dist = length(reference_mesh->vertices.at(p_id) - comp_mesh->vertices.at(id));
				min_dist = glm::min(min_dist, dist);
			}
            distances.at(p_id) = min_dist;
        }

        const auto min_max = std::minmax_element(std::execution::par, distances.begin(), distances.end());
        auto min_dist = *min_max.first;
        auto max_dist = *min_max.second;
        auto avg_dist = std::reduce(std::execution::par, distances.begin(), distances.end(), 0.0f);
        avg_dist = avg_dist / static_cast<float>(distances.size());
        const auto sd_dist = glm::sqrt(tostf::math::variance_seq(distances, avg_dist));
        std::stringstream ss;
        ss << min_dist << "\n" << max_dist << "\n" << avg_dist << "\n" << sd_dist;
        tostf::save_str_to_file("dist_to_ref.txt", ss.str());
    }
    return 0;
}
