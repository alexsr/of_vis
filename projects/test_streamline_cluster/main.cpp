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
#include "data_processing/clustering.hpp"
#include "utility/random.hpp"

std::vector<int> eigen_kmeans(Eigen::MatrixXf M, int k) {
	std::vector<Eigen::VectorXf> center(k);
	std::vector<int> assignment(M.rows());
	Eigen::IOFormat HeavyFmt(Eigen::FullPrecision, 0, ", ", ";\n", "[", "]", "[", "]");
	for (int i = 0; i < k; ++i) {
		center.at(i) = M.row(tostf::gen_random_int_value<int>(0, M.rows() - 1));
		tostf::log() << center.at(i).format(HeavyFmt);
	}
	int assignments = 1;
	while (assignments > 0) {
		assignments = 0;
		Eigen::VectorXf ref_center = Eigen::VectorXf::Zero(k);
		tostf::log() << ref_center.format(HeavyFmt);
		std::vector<Eigen::VectorXf> new_center(k, ref_center);
		std::vector<float> to_center_count(k, 0.0f);
		for (int j = 0; j < M.rows(); ++j) {
			Eigen::VectorXf m_row = M.row(j);
			tostf::log() << m_row.format(HeavyFmt);
			int curr_cluster = -1;
			float min_dist = FLT_MAX;
			for (int i = 0; i < k; ++i) {
				float dist = (center.at(i) - m_row).norm();
				if (dist < min_dist) {
					curr_cluster = i;
					min_dist = dist;
				}
			}
			if (assignment.at(j) != curr_cluster) {
				assignment.at(j) = curr_cluster;
				assignments++;
			}
			new_center.at(curr_cluster) += m_row;
			to_center_count.at(curr_cluster) += 1.0f;
		}
		for (int i = 0; i < k; ++i) {
			if (to_center_count.at(i) > 0) {
				center.at(i) = new_center.at(i) / to_center_count.at(i);
			}
			tostf::log() << center.at(i).format(HeavyFmt);
		}
	}
	return assignment;
}

int main() {
    tostf::cmd::enable_color();
	const tostf::window_config win_conf{
		"Aneurysm mesh rendering", 1600, 900, 0, tostf::win_mgr::default_window_flags
	};
	const tostf::gl_settings gl_config{
		4, 6, true, tostf::gl_profile::core, 4,
		{1, 1, 1, 1}, {tostf::gl::cap::depth_test}
	};
	tostf::Window window(win_conf, gl_config);
	window.set_minimum_size({ 800, 800 });
	int line_count = 100;
	int line_size = 100;
	int k = 5;
	std::vector<glm::vec4> streamlines(line_count * line_size);
#pragma omp parallel for
	for (int i = 0; i < static_cast<int>(streamlines.size()); ++i) {
		streamlines.at(i) = tostf::gen_random_vec(glm::vec4(-1.0f), glm::vec4(1.0f));
	}
	
	float sigma = 0.33;
	auto Weight = Eigen::MatrixXf(line_count, line_count);
	auto Diag = Eigen::VectorXf(line_count);
	for (int i = 0; i < line_count; ++i) {
		auto a_beg = streamlines.begin() + i * line_size;
		auto a_end = a_beg + line_size;
		for (int j = 0; j < line_count; ++j) {
			if (i == j) {
				Weight(i, j) = 0.0f;
				continue;
			}
			auto b_beg = streamlines.begin() + j * line_size;
			auto b_end = b_beg + line_size;
			auto sim = tostf::mcpd(a_beg, a_end, b_beg, b_end);
			auto weight = glm::exp(-sim * sim / 2.0f * sigma * sigma);
			Weight(i, j) = weight;
		}
	}
	for (int i = 0; i < line_count; ++i) {
		Diag(i) = 0.0f;
		for (int j = 0; j < line_count; ++j) {
			if (i == j) {
				continue;
			}
			Diag(i) += Weight(i, j);
		}
	}
	Eigen::SelfAdjointEigenSolver<Eigen::MatrixXf> sqrtinverse(Diag.asDiagonal().toDenseMatrix());
	Eigen::IOFormat HeavyFmt(Eigen::FullPrecision, 0, ", ", ";\n", "[", "]", "[", "]");
	auto inverse_sqrt_D = sqrtinverse.operatorInverseSqrt();
	auto L = inverse_sqrt_D * Weight * inverse_sqrt_D;

	Eigen::EigenSolver<Eigen::MatrixXf> es(L);
	auto eig_vals = es.eigenvalues().real();
	std::vector<std::pair<float, int>> sorted_eigenvalues(eig_vals.size());
	for (int i = 0; i < static_cast<int>(sorted_eigenvalues.size()); ++i) {
		sorted_eigenvalues.at(i) = std::make_pair<>(eig_vals(i), i);
	}
	std::sort(sorted_eigenvalues.begin(), sorted_eigenvalues.end(),
		[](const std::pair<float, int>& a, const std::pair<float, int>& b) {
		return a.first < b.first;
	});

	Eigen::MatrixXf U(line_count, k);
	for (int j = 0; j < k; ++j) {
		U.col(j) = es.eigenvectors().col(j).real();
	}
	U.rowwise().normalize();
	auto cluster = eigen_kmeans(U, k);
	std::vector<glm::vec4> cluster_colors(k);
	for (int i = 0; i < k; ++i) {
		cluster_colors.at(i) = tostf::gen_random_vec(glm::vec4(0.0f), glm::vec4(1.0f));
	}
	std::vector<glm::vec4> colors(streamlines.size());
#pragma omp parallel for
	for (int i = 0; i < line_count; ++i) {
		auto col = cluster_colors.at(cluster.at(i));
		for (int j = 0; j < line_size; ++j) {
			colors.at(i * line_count + j) = col;
		}
	}
	auto buffer = std::make_shared<tostf::VBO>(streamlines, 4);
	auto color_buffer = std::make_shared<tostf::VBO>(colors, 4);
	tostf::VAO vao;
	vao.attach_main_vbo_instanced(buffer);
	vao.attach_vbo_instanced(color_buffer, 1);

	tostf::Trackball_camera cam(1.0f, 0.2f, 10.0f, true, glm::vec3(0.0f), 60.0f);
	tostf::V_F_shader visualization_shader("../../shader/rendering/render_spheres_fixed.vert",
		"../../shader/rendering/render_spheres_fixed.frag");
	visualization_shader.use();
	float dt = 0.001f;
	while (!window.should_close()) {
		tostf::gl::clear_all();
		cam.update(window, dt);
		visualization_shader.update_uniform("view", cam.get_view());
		visualization_shader.update_uniform("proj", cam.get_projection());
		visualization_shader.use();
		vao.draw(tostf::gl::draw::triangle_strip, line_count * line_size, 4);
		window.swap_buffers();
		window.poll_events();
	}
	tostf::win_mgr::terminate();
    return 0;
}
