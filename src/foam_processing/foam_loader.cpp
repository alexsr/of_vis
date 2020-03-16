//
// Alexander Scheid-Rehder
// alexanderb@scheid-rehder.de
// https://www.alexsr.de
// https://github.com/alexsr
//

#include "foam_loader.hpp"
#include "file/file_handling.hpp"
#include <cctype>
#include <utility>
#include "preprocessing/inlet_detection.hpp"
#include "utility/vector.hpp"
#include <sstream>

int digits_to_int(const std::vector<int>& digits) {
    auto exponent = pow(10, static_cast<int>(digits.size()) - 1);
    int result = 0;
    for (auto& d : digits) {
        result += static_cast<int>(d * exponent);
        exponent /= 10;
    }
    return result;
}

std::pair<int, size_t> parse_int(const std::string& file, size_t start) {
    std::vector<int> digits;
    auto curr_digit = file.at(start);
    while (std::isdigit(curr_digit)) {
        digits.push_back(static_cast<int>(curr_digit - '0'));
        start++;
        curr_digit = file.at(start);
    }
    return {digits_to_int(digits), start};
}

std::pair<float, size_t> parse_float(const std::string& file, size_t start) {
    std::vector<int> digits;
    auto curr_digit = file.at(start);
    while (std::isdigit(curr_digit)) {
        digits.push_back(static_cast<int>(curr_digit - '0'));
        start++;
        curr_digit = file.at(start);
    }
    if (file.at(start) == '.') {
        start++;
    }
    std::vector<int> frac;
    while (std::isdigit(curr_digit)) {
        frac.push_back(static_cast<int>(curr_digit - '0'));
        start++;
        curr_digit = file.at(start);
    }
    auto res = static_cast<float>(digits_to_int(digits));
    if (!frac.empty()) {
        res += static_cast<float>(digits_to_int(frac)) / pow(10.0f, static_cast<float>(frac.size()) - 1.0f);
    }
    return {res, start};
}

struct Needle_pos {
    size_t start;
    size_t end;

    bool found() const {
        return start != std::string::npos;
    }
};

size_t skip_whitespace(const std::string& file, size_t start) {
    while (std::isspace(file.at(start))) {
        start++;
    }
    return start;
}

size_t move_to_first_alnum(const std::string& file, size_t start) {
    while (!std::isalnum(file.at(start)) || file.at(start) == '_') {
        start++;
    }
    return start;
}

std::pair<std::string, size_t> parse_alnum(const std::string& file, size_t start) {
    std::string str;
    auto curr_char = file.at(start);
    while (std::isalnum(curr_char) || curr_char == '_') {
        str.push_back(curr_char);
        start++;
        curr_char = file.at(start);
    }
    return {str, start};
}

Needle_pos find_in_str(const std::string& haystack, const std::string& needle, const size_t start) {
    const auto pos = haystack.find(needle, start);
    return {pos, pos + needle.size()};
}

Needle_pos skip_header(const std::string& file) {
    const auto pos = find_in_str(file, "FoamFile", 0);
    const auto pos_header_end = find_in_str(file, "//\n", pos.end).end;
    return {0, pos_header_end};
}

void tostf::foam::Foam_face_handler::load_faces_from_file(const std::filesystem::path& path) {
    const auto file = load_file_str(path, std::ios::in | std::ios::binary);
    const auto file_ptr = file.c_str();
    auto curr_pos = skip_whitespace(file, skip_header(file).end);
    const auto array_size = parse_int(file, curr_pos);
    std::vector<int> indices(array_size.first);
    curr_pos = find_in_str(file, "(", array_size.second).end;
    std::memcpy(indices.data(), file_ptr + curr_pos, array_size.first * sizeof(int));
    curr_pos += array_size.first * sizeof(int);
    curr_pos = find_in_str(file, ")\n", curr_pos).end;
    const auto point_refs_size = parse_int(file, curr_pos);
    curr_pos = find_in_str(file, "(", point_refs_size.second).end;
    point_refs.resize(point_refs_size.first);
    std::memcpy(point_refs.data(), file_ptr + curr_pos, point_refs_size.first * sizeof(int));
    const auto point_refs_begin = point_refs.begin();
    if (!indices.empty()) {
        face_it_refs.resize(indices.size() - 1);
#pragma omp parallel for
        for (int i = 0; i < static_cast<int>(face_it_refs.size()); i++) {
            face_it_refs.at(i).begin = point_refs_begin + indices.at(i);
            face_it_refs.at(i).end = point_refs_begin + indices.at(i + 1);
        }
    }
}

void tostf::foam::Foam_owner::load_cells_from_file(const std::filesystem::path& path) {
    const auto owner_file = load_file_str(path / "owner", std::ios::in | std::ios::binary);
    const auto owner_file_ptr = owner_file.c_str();
    const auto cell_count_file = parse_int(owner_file, find_in_str(owner_file, "nCells:", 0).end);
    cell_count = cell_count_file.first;
    internal_faces_count = parse_int(owner_file,
                                     find_in_str(owner_file, "nInternalFaces:", cell_count_file.second).end).first;

    auto curr_pos = skip_whitespace(owner_file, skip_header(owner_file).end);
    const auto array_size = parse_int(owner_file, curr_pos);
    owner.resize(array_size.first);
    curr_pos = find_in_str(owner_file, "(", array_size.second).end;
    std::memcpy(owner.data(), owner_file_ptr + curr_pos, array_size.first * sizeof(int));

    const auto neighbor_file = load_file_str(path / "neighbour", std::ios::in | std::ios::binary);
    const auto neighbor_file_ptr = neighbor_file.c_str();
    curr_pos = skip_whitespace(neighbor_file, skip_header(neighbor_file).end);
    const auto neighbor_array_size = parse_int(neighbor_file, curr_pos);
    neighbor.resize(neighbor_array_size.first);
    curr_pos = find_in_str(neighbor_file, "(", neighbor_array_size.second).end;
    std::memcpy(neighbor.data(), neighbor_file_ptr + curr_pos, neighbor_array_size.first * sizeof(int));
}

std::vector<glm::vec4> tostf::foam::load_points_from_file(const std::filesystem::path& path) {
    const auto file = load_file_str(path, std::ios::in | std::ios::binary);
    const auto file_ptr = file.c_str();
    auto curr_pos = skip_whitespace(file, skip_header(file).end);
    const auto array_size = parse_int(file, curr_pos);
    std::vector<glm::dvec3> points(array_size.first);
    curr_pos = find_in_str(file, "(", array_size.second).end;
    std::memcpy(points.data(), file_ptr + curr_pos, array_size.first * sizeof(glm::dvec3));
    std::vector<glm::vec4> result(points.size());
    std::transform(std::execution::par_unseq, points.begin(), points.end(), result.begin(),
                   [](const glm::dvec3& v) { return glm::vec4(v, 1.0f); });
    return result;
}

std::vector<tostf::foam::Foam_boundary> tostf::foam::load_boundaries_from_file(const std::filesystem::path& path) {
    const auto file = load_file_str(path);
    auto curr_pos = skip_whitespace(file, skip_header(file).end);
    const auto array_size = parse_int(file, curr_pos);
    curr_pos = array_size.second;
    std::vector<Foam_boundary> boundaries;
    while (curr_pos != std::string::npos) {
        curr_pos = move_to_first_alnum(file, curr_pos);
        const auto boundary_name = parse_alnum(file, curr_pos);
        curr_pos = skip_whitespace(file, find_in_str(file, "nFaces", boundary_name.second).end);
        const auto boundary_face_count = parse_int(file, curr_pos);
        curr_pos = skip_whitespace(file, find_in_str(file, "startFace", boundary_face_count.second).end);
        const auto boundary_start_face = parse_int(file, curr_pos);
        boundaries.push_back({boundary_name.first, boundary_start_face.first, boundary_face_count.first});
        curr_pos = find_in_str(file, "}", boundary_start_face.second).end;
        if (find_in_str(file, "}", curr_pos).start == std::string::npos) {
            break;
        }
    }
    return boundaries;
}


void tostf::foam::Poly_mesh::load(std::filesystem::path p) {
    boundaries.clear();
    cell_radii.clear();
    cell_centers.clear();
    boundaries.clear();
    case_path = std::move(p);
    const auto path = case_path / "constant" / "polyMesh";
    auto points = load_points_from_file(path / "points");
    mesh_bb = calculate_bounding_box(points);

    const auto foam_boundaries = load_boundaries_from_file(path / "boundary");
    Foam_face_handler foam_faces;
    foam_faces.load_faces_from_file(path / "faces");
    Foam_owner foam_owner;
    foam_owner.load_cells_from_file(path);
    cell_centers.resize(foam_owner.cell_count);

    std::vector<Bounding_box> cell_bounding_boxes(foam_owner.cell_count);
    for (int face_id = 0; face_id < foam_owner.internal_faces_count; ++face_id) {
        auto& face_its = foam_faces.face_it_refs.at(face_id);
        for (auto face_it = face_its.begin; face_it < face_its.end; ++face_it) {
            auto& p_o = points.at(*face_it);
            cell_centers.at(foam_owner.owner.at(face_id)) += p_o;
            cell_bounding_boxes.at(foam_owner.owner.at(face_id)).min =
                min(cell_bounding_boxes.at(foam_owner.owner.at(face_id)).min, p_o);
            cell_bounding_boxes.at(foam_owner.owner.at(face_id)).max =
                max(cell_bounding_boxes.at(foam_owner.owner.at(face_id)).max, p_o);

            cell_centers.at(foam_owner.neighbor.at(face_id)) += p_o;
            cell_bounding_boxes.at(foam_owner.neighbor.at(face_id)).min =
                min(cell_bounding_boxes.at(foam_owner.neighbor.at(face_id)).min, p_o);
            cell_bounding_boxes.at(foam_owner.neighbor.at(face_id)).max =
                max(cell_bounding_boxes.at(foam_owner.neighbor.at(face_id)).max, p_o);

        }
    }
    boundaries.resize(foam_boundaries.size());
    for (int b_id = 0; b_id < static_cast<int>(foam_boundaries.size()); ++b_id) {
        boundaries.at(b_id).name = foam_boundaries.at(b_id).name;
        boundaries.at(b_id).points.resize(foam_boundaries.at(b_id).size);
        boundaries.at(b_id).radii.resize(foam_boundaries.at(b_id).size);
        boundaries.at(b_id).cell_refs.resize(foam_boundaries.at(b_id).size);
        for (int face_id = foam_boundaries.at(b_id).start_id;
             face_id < foam_boundaries.at(b_id).start_id + foam_boundaries.at(b_id).size; ++face_id) {
            auto& face_its = foam_faces.face_it_refs.at(face_id);
            glm::vec4 face_center(0.0f);
            Bounding_box face_bb;
            for (auto face_it = face_its.begin; face_it < face_its.end; ++face_it) {
                auto& p_o = points.at(*face_it);
                face_center += p_o;
                face_bb.min = min(face_bb.min, p_o);
                face_bb.max = max(face_bb.max, p_o);
                cell_centers.at(foam_owner.owner.at(face_id)) += p_o;
                cell_bounding_boxes.at(foam_owner.owner.at(face_id)).min =
                    min(cell_bounding_boxes.at(foam_owner.owner.at(face_id)).min, p_o);
                cell_bounding_boxes.at(foam_owner.owner.at(face_id)).max =
                    max(cell_bounding_boxes.at(foam_owner.owner.at(face_id)).max, p_o);
            }
            boundaries.at(b_id).points.at(face_id - foam_boundaries.at(b_id).start_id) = face_center / face_center.w;
            boundaries.at(b_id).radii.at(face_id - foam_boundaries.at(b_id).start_id) =
                length(face_bb.max - face_bb.min) / 2.0f;
            boundaries.at(b_id).cell_refs.at(face_id - foam_boundaries.at(b_id).start_id) =
                foam_owner.owner.at(face_id);
        }
    }
    foam_faces.point_refs.clear();
    foam_faces.face_it_refs.clear();
    cell_radii.resize(cell_centers.size());
#pragma omp parallel for
    for (int c_id = 0; c_id < foam_owner.cell_count; ++c_id) {
        cell_centers.at(c_id) /= cell_centers.at(c_id).w;
        cell_radii.at(c_id) = length(cell_bounding_boxes.at(c_id).max
                                     - cell_bounding_boxes.at(c_id).min) / 2.0f;
    }
}

std::string tostf::foam::Poly_mesh::gen_datapoint_str() {
    std::stringstream internal_str;
    for (auto c : cell_centers) {
        internal_str << c.x << ", " << c.y << ", " << c.z << "\n";
    }
    for (const auto& b_s : boundaries) {
        for (auto b : b_s.points) {
            internal_str << b.x << ", " << b.y << ", " << b.z << "\n";
        }
    }
    return internal_str.str();
}

tostf::foam::Field<float> tostf::foam::Poly_mesh::load_scalar_field_from_file(const std::string& step_path,
                                                                              const std::string& field_name) const {
    const auto file = load_file_str(case_path / step_path / field_name, std::ios::in | std::ios::binary);
    const auto file_ptr = file.c_str();
    const auto field_class = parse_alnum(file, skip_whitespace(file, find_in_str(file, "class", 0).end));
    const auto internal_field = parse_alnum(
        file, skip_whitespace(file, find_in_str(file, "internalField", field_class.second).end));
    Field<float> result;
    if (internal_field.first == "uniform") {
        if (field_class.first == "volScalarField") {
            const auto internal_value = parse_float(file, skip_whitespace(file, internal_field.second));
            result.internal_data = std::vector<float>(cell_centers.size(), internal_value.first);
            result.min = internal_value.first;
            result.max = internal_value.first;
            result.avg = internal_value.first * static_cast<float>(cell_centers.size());
        }
        else {
            const auto x_value = parse_float(file, skip_whitespace(file, internal_field.second) + 1);
            const auto y_value = parse_float(file, skip_whitespace(file, x_value.second));
            const auto z_value = parse_float(file, skip_whitespace(file, y_value.second));
            const auto mag = length(glm::vec3(x_value.first, y_value.first, z_value.first));
            result.internal_data = std::vector<float>(cell_centers.size(), mag);
            result.min = mag;
            result.max = mag;
            result.avg = mag * static_cast<float>(cell_centers.size());
        }
    }
    else if (internal_field.first == "nonuniform") {
        auto curr_pos = skip_whitespace(file, find_in_str(file, "\n", internal_field.second).end);
        const auto array_size = parse_int(file, curr_pos);
        curr_pos = find_in_str(file, "(", array_size.second).end;
        std::vector<float> internal_data(array_size.first);
        if (field_class.first == "volScalarField" || field_class.first == "surfaceScalarField") {
            std::vector<double> scalar(array_size.first);
            std::memcpy(scalar.data(), file_ptr + curr_pos, array_size.first * sizeof(double));
            std::transform(std::execution::par_unseq, scalar.begin(), scalar.end(), internal_data.begin(),
                           [](const double& v) { return static_cast<float>(v); });
        }
        else {
            std::vector<glm::dvec3> scalar(array_size.first);
            std::memcpy(scalar.data(), file_ptr + curr_pos, array_size.first * sizeof(glm::dvec3));
            std::transform(std::execution::par_unseq, scalar.begin(), scalar.end(), internal_data.begin(),
                           [](const glm::dvec3& v) { return static_cast<float>(length(v)); });
        }
        const auto min_max = std::minmax_element(std::execution::par, internal_data.begin(), internal_data.end());
        result.min = *min_max.first;
        result.max = *min_max.second;
        result.avg = std::reduce(std::execution::par, internal_data.begin(), internal_data.end(), 0.0f);
        result.internal_data = std::move(internal_data);
    }
    result.boundaries_data.resize(boundaries.size());
    for (int b_id = 0; b_id < static_cast<int>(boundaries.size()); ++b_id) {
        const auto boundary_name = find_in_str(file, boundaries.at(b_id).name, internal_field.second);
        auto boundary_field =
            parse_alnum(file, skip_whitespace(file, find_in_str(file, "type", boundary_name.end).end));
        auto value_type =
            parse_alnum(file, skip_whitespace(file, find_in_str(file, "value", boundary_field.second).end));

        if (boundary_field.first == "calculated" && value_type.first == "uniform") {
            boundary_field = value_type;
        }
        if (boundary_field.first == "zeroGradient" && !result.internal_data.empty()) {
            std::vector<float> boundary_data(boundaries.at(b_id).points.size());
#pragma omp parallel for
            for (int p_id = 0; p_id < static_cast<int>(boundaries.at(b_id).points.size()); ++p_id) {
                boundary_data.at(p_id) = result.internal_data.at(boundaries.at(b_id).cell_refs.at(p_id));
            }
            const auto min_max = std::minmax_element(std::execution::par, boundary_data.begin(),
                                                     boundary_data.end());
            result.min = glm::min(result.min, *min_max.first);
            result.max = glm::max(result.max, *min_max.second);
            result.avg += std::reduce(std::execution::par, boundary_data.begin(), boundary_data.end(), 0.0f);
            result.boundaries_data.at(b_id) = std::move(boundary_data);
        }
        else if (boundary_field.first == "uniform" || boundary_field.first == "fixedValue") {
            if (field_class.first == "volScalarField") {
                const auto boundary_value = parse_float(file, skip_whitespace(file, boundary_field.second));
                result.boundaries_data.at(b_id) = std::vector<float>(boundaries.at(b_id).points.size(),
                                                                     boundary_value.first);
                result.min = glm::min(result.min, boundary_value.first);
                result.max = glm::max(result.max, boundary_value.first);
                result.avg += boundary_value.first * static_cast<float>(boundaries.at(b_id).points.size());
            }
            else {
                const auto x_value = parse_float(file, skip_whitespace(file, boundary_field.second) + 1);
                const auto y_value = parse_float(file, skip_whitespace(file, x_value.second));
                const auto z_value = parse_float(file, skip_whitespace(file, y_value.second));
                const auto mag = length(glm::vec3(x_value.first, y_value.first, z_value.first));
                result.boundaries_data.at(b_id) = std::vector<float>(boundaries.at(b_id).points.size(), mag);
                result.min = glm::min(result.min, mag);
                result.max = glm::max(result.max, mag);
                result.avg += mag * static_cast<float>(boundaries.at(b_id).points.size());
            }
        }
        else if ((boundary_field.first == "calculated" || boundary_field.first == "fixedFluxPressure") && value_type.first == "nonuniform") {
            auto curr_pos = find_in_str(file, "\n", value_type.second).end;
            const auto array_size = parse_int(file, curr_pos);
            curr_pos = find_in_str(file, "(", array_size.second).end;
            std::vector<float> boundary_data(array_size.first);
            if (field_class.first == "volScalarField" || field_class.first == "surfaceScalarField") {
                std::vector<double> scalar(array_size.first);
                std::memcpy(scalar.data(), file_ptr + curr_pos, array_size.first * sizeof(double));
                std::transform(std::execution::par_unseq, scalar.begin(), scalar.end(), boundary_data.begin(),
                               [](const double& v) { return static_cast<float>(v); });
            }
            else {
                std::vector<glm::dvec3> scalar(array_size.first);
                std::memcpy(scalar.data(), file_ptr + curr_pos, array_size.first * sizeof(glm::dvec3));
                std::transform(std::execution::par_unseq, scalar.begin(), scalar.end(), boundary_data.begin(),
                               [](const glm::dvec3& v) { return static_cast<float>(length(v)); });
            }
            const auto min_max = std::minmax_element(std::execution::par, boundary_data.begin(),
                                                     boundary_data.end());
            result.min = glm::min(result.min, *min_max.first);
            result.max = glm::max(result.max, *min_max.second);
            result.avg += std::reduce(std::execution::par, boundary_data.begin(), boundary_data.end(), 0.0f);
            result.boundaries_data.at(b_id) = std::move(boundary_data);
        }
    }
    if (!result.internal_data.empty()) {
        result.avg /= static_cast<float>(result.internal_data.size());
    }
    for (auto& b : result.boundaries_data) {
        if (!b.empty()) {
            result.avg /= static_cast<float>(b.size());
        }
    }
    return result;
}

tostf::foam::Field<glm::vec4> tostf::foam::Poly_mesh::load_vector_field_from_file(const std::string& step_path,
                                                                                  const std::string& field_name) const {
    const auto file = load_file_str(case_path / step_path / field_name, std::ios::in | std::ios::binary);
    const auto file_ptr = file.c_str();
    const auto field_class = parse_alnum(file, skip_whitespace(file, find_in_str(file, "class", 0).end));
    const auto internal_field = parse_alnum(
        file, skip_whitespace(file, find_in_str(file, "internalField", field_class.second).end));
    Field<glm::vec4> result;
    if (internal_field.first == "uniform") {
        if (field_class.first == "volVectorField") {
            const auto x_value = parse_float(file, skip_whitespace(file, internal_field.second) + 1);
            const auto y_value = parse_float(file, skip_whitespace(file, x_value.second));
            const auto z_value = parse_float(file, skip_whitespace(file, y_value.second));
            const glm::vec4 vec(x_value.first, y_value.first, z_value.first, 0.0f);
            result.internal_data = std::vector<glm::vec4>(cell_centers.size(), vec);
            result.min = length(glm::vec3(vec));
            result.max = length(glm::vec3(vec));
            result.avg = length(glm::vec3(vec)) * static_cast<float>(cell_centers.size());
        }
    }
    else if (internal_field.first == "nonuniform") {
        auto curr_pos = skip_whitespace(file, find_in_str(file, "\n", internal_field.second).end);
        const auto array_size = parse_int(file, curr_pos);
        curr_pos = find_in_str(file, "(", array_size.second).end;
        std::vector<glm::vec4> internal_data(array_size.first);
        if (field_class.first == "volVectorField") {
            std::vector<glm::dvec3> scalar(array_size.first);
            std::memcpy(scalar.data(), file_ptr + curr_pos, array_size.first * sizeof(glm::dvec3));
            std::transform(std::execution::par_unseq, scalar.begin(), scalar.end(), internal_data.begin(),
                           [](const glm::dvec3& v) { return glm::vec4(v, length(v)); });
        }
        const auto min_max = std::minmax_element(std::execution::par, internal_data.begin(), internal_data.end(),
                                                 [](const glm::vec4& a, const glm::vec4& b) {
                                                     return a.w < b.w;
                                                 });
        result.min = min_max.first->w;
        result.max = min_max.second->w;
        result.avg = std::reduce(std::execution::par, internal_data.begin(), internal_data.end(), glm::vec4(0.0f)).w;
        result.internal_data = std::move(internal_data);
    }
    result.boundaries_data.resize(boundaries.size());
    for (int b_id = 0; b_id < static_cast<int>(boundaries.size()); ++b_id) {
        const auto boundary_name = find_in_str(file, boundaries.at(b_id).name, internal_field.second);
        auto boundary_field =
            parse_alnum(file, skip_whitespace(file, find_in_str(file, "type", boundary_name.end).end));
        auto value_type =
            parse_alnum(file, skip_whitespace(file, find_in_str(file, "value", boundary_field.second).end));
        if (boundary_field.first == "calculated" && value_type.first == "uniform") {
            boundary_field = value_type;
        }
        if (boundary_field.first == "zeroGradient" && !result.internal_data.empty()) {
            std::vector<glm::vec4> boundary_data(boundaries.at(b_id).points.size());
#pragma omp parallel for
            for (int p_id = 0; p_id < static_cast<int>(boundaries.at(b_id).points.size()); ++p_id) {
                boundary_data.at(p_id) = result.internal_data.at(boundaries.at(b_id).cell_refs.at(p_id));
            }
            const auto min_max = std::minmax_element(std::execution::par, boundary_data.begin(), boundary_data.end(),
                                                 [](const glm::vec4& a, const glm::vec4& b) {
                                                     return a.w < b.w;
                                                 });
            result.min = glm::min(result.min, min_max.first->w);
            result.max = glm::max(result.max, min_max.second->w);
            result.avg += std::reduce(std::execution::par, boundary_data.begin(), boundary_data.end(), glm::vec4(0.0f)).w;
            result.boundaries_data.at(b_id) = std::move(boundary_data);
        }
        else if (boundary_field.first == "uniform") {
            if (field_class.first == "volVectorField") {
                const auto x_value = parse_float(file, skip_whitespace(file, boundary_field.second) + 1);
                const auto y_value = parse_float(file, skip_whitespace(file, x_value.second));
                const auto z_value = parse_float(file, skip_whitespace(file, y_value.second));
                const auto vec = glm::vec4(x_value.first, y_value.first, z_value.first, 0.0f);
                result.boundaries_data.at(b_id) = std::vector<glm::vec4>(boundaries.at(b_id).points.size(), vec);
                result.min = length(vec);
                result.max = length(vec);
                result.avg += length(vec) * static_cast<float>(boundaries.at(b_id).points.size());
            }
        }
        else if (boundary_field.first == "calculated" && value_type.first == "nonuniform") {
            auto curr_pos = skip_whitespace(file, find_in_str(file, "\n", value_type.second).end);
            const auto array_size = parse_int(file, curr_pos);
            curr_pos = find_in_str(file, "(", array_size.second).end;
            std::vector<glm::vec4> boundary_data(array_size.first);
            if (field_class.first == "volVectorField") {
                std::vector<glm::dvec3> vector(array_size.first);
                std::memcpy(vector.data(), file_ptr + curr_pos, array_size.first * sizeof(glm::dvec3));
                std::transform(std::execution::par_unseq, vector.begin(), vector.end(), boundary_data.begin(),
                               [](const glm::dvec3& v) { return glm::vec4(v, length(v)); });
            }
            const auto min_max = std::minmax_element(std::execution::par, boundary_data.begin(), boundary_data.end(),
                                                     [](const glm::vec4& a, const glm::vec4& b) {
                                                         return a.w < b.w;
                                                     });
            result.min = glm::min(result.min, min_max.first->w);
            result.max = glm::max(result.max, min_max.second->w);
            result.avg += std::reduce(std::execution::par, boundary_data.begin(), boundary_data.end(), glm::vec4(0.0f)).w;
            result.boundaries_data.at(b_id) = std::move(boundary_data);
        }
    }
    if (!result.internal_data.empty()) {
        result.avg /= static_cast<float>(result.internal_data.size());
    }
    for (auto& b : result.boundaries_data) {
        if (!b.empty()) {
            result.avg /= static_cast<float>(b.size());
        }
    }
    return result;
}

bool tostf::foam::is_vector_field_file(const std::filesystem::path& file_path) {
    const auto file = load_file_str(file_path);
    return parse_alnum(file, skip_whitespace(file, find_in_str(file, "class", 0).end)).first == "volVectorField";
}
