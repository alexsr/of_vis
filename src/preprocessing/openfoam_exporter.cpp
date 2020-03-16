//
// Alexander Scheid-Rehder
// alexanderb@scheid-rehder.de
// https://www.alexsr.de
// https://github.com/alexsr
//

#include "openfoam_exporter.hpp"
#include <utility>
#include "utility/logging.hpp"
#include "file/file_handling.hpp"
#include "geometry/mesh_handling.hpp"
#include "utility/random.hpp"
#include "utility/str_conversion.hpp"
#include "inlet_detection.hpp"
#include "mesh_processing/halfedge_mesh.hpp"
#include "glm/gtx/component_wise.hpp"

std::string to_string_with_precision(const float a_value, const int n = 10)
{
    std::ostringstream out;
    out.precision(n);
    out << std::fixed << a_value;
    return out.str();
}

std::string bool_to_str(const bool v) {
    if (v) {
        return "true";
    }
    return "false";
}

std::string gen_foam_file_str(const std::string& filename, const std::string& cls = "dictionary") {
    return "/*--------------------------------*- C++ -*----------------------------------*\\\n"
           "| =========                 |                                                 |\n"
           "| \\\\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox           |\n"
           "|  \\\\    /   O peration     | Version:  v1806                                 |\n"
           "|   \\\\  /    A nd           | Web:      www.OpenFOAM.com                      |\n"
           "|    \\\\/     M anipulation  |                                                 |\n"
           "\\*---------------------------------------------------------------------------*/\n"
           "FoamFile {\n    version    2.0;\n    format    ascii;\n"
           "    class    " + cls + ";\n    object    " + filename + ";\n}\n"
           "// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //\n\n";
}

std::string gen_file_end_str() {
    return "\n// ************************************************************************* //\n";
}

std::string tostf::foam::Algorithm_settings::get_application_name() const {
    if (steady == 1) {
        return "simpleFoam";
    }
    return "pimpleFoam";
}



std::string tostf::foam::Control_dict::to_string(bool enable_iter) const {
    const auto purge_write = static_cast<int>(glm::ceil(duration / write_interval) * cycles_to_overwrite);
	if (enable_iter) {
		return "startFrom startTime;\nstartTime 0;\nstopAt endTime;\nendTime " + std::to_string(iterations) + ";\n"
			"deltaT 1;\nwriteControl timeStep;\nwriteInterval "
			+ std::to_string(iter_write_interval) + ";\nwriteFormat " + write_format_to_string(format) + ";\n"
			"writePrecision " + std::to_string(write_precision) + ";\npurgeWrite "
			+ std::to_string(purge_write) + ";\n"
			"timeFormat fixed;\ntimePrecision 0;\n";
	}
    return "startFrom startTime;\nstartTime 0;\nstopAt endTime;\nendTime " + std::to_string(duration * cycles) + ";\n"
           "deltaT " + to_string_with_precision(dt) + ";\nwriteControl adjustableRunTime;\nwriteInterval "
           + std::to_string(write_interval) + ";\nwriteFormat " + write_format_to_string(format) + ";\n"
           "writePrecision " + std::to_string(write_precision) + ";\npurgeWrite "
           + std::to_string(purge_write) + ";\n"
           "timeFormat fixed;\ntimePrecision " + std::to_string(time_precision) + ";\n"
           "adjustTimeStep true;\nmaxCo " + to_string_with_precision(max_co) + ";\n"
           "maxDeltaT " + to_string_with_precision(max_dt) + ";\n";
}

tostf::foam::Foam_field::Foam_field(std::string in_name, const Dimensions& in_dims)
    : name(std::move(in_name)), dims(in_dims) { }

tostf::foam::Scalar_field::Scalar_field(const std::string& in_name, const Dimensions& in_dims,
                                        const float in_internal_value)
    : Foam_field(in_name, in_dims), internal_value(in_internal_value) {}

tostf::foam::Vector_field::Vector_field(const std::string& in_name, const Dimensions& in_dims,
                                        const glm::vec3& in_internal_value)
    : Foam_field(in_name, in_dims), internal_value(in_internal_value) {}

std::string tostf::foam::solver_type_to_str(const field_solver_types t) {
    switch (t) {
        case field_solver_types::pcg:
            return "PCG";
        case field_solver_types::pbicg:
            return "PBiCG";
        case field_solver_types::pbicg_stab:
            return "PBiCGStab";
        case field_solver_types::smooth_solver:
            return "smoothSolver";
        case field_solver_types::gamg:
            return "GAMG";
        case field_solver_types::diagonal:
            return "diagonal";
        default:
            return "PCG";
    }
}

std::string tostf::foam::preconditioner_to_str(field_preconditioner p) {
    switch (p) {
        case field_preconditioner::dic:
            return "DIC";
        case field_preconditioner::fdic:
            return "FDIC";
        case field_preconditioner::dilu:
            return "DILU";
        case field_preconditioner::diagonal:
            return "diagonal";
        case field_preconditioner::gamg:
            return "GAMG";
        case field_preconditioner::none:
        default:
            return "none";
    }
}

std::string tostf::foam::smoother_to_str(const field_smoother s) {
    switch (s) {
        case field_smoother::gauss_seidel:
            return "GaussSeidel";
        case field_smoother::sym_gauss_seidel:
            return "symGaussSeidel";
        case field_smoother::dic_gauss_seidel:
            return "DICGaussSeidel";
        case field_smoother::dic:
            return "DIC";
        case field_smoother::dilu:
            return "DILU";
        default:
            return "GaussSeidel";
    }
}

std::string tostf::foam::Field_solver::to_string() const {
    std::string solver_str = "{\nsolver " + solver_type_to_str(solver) + ";\n";
    if (solver == field_solver_types::smooth_solver || solver == field_solver_types::gamg) {
        solver_str += "smoother " + smoother_to_str(smoother) + ";\n";
		if (solver == field_solver_types::gamg) {
			solver_str += "nCoarsestCells 1000;\nnPreSweeps 0;\nnPostSweeps 2;\ncacheAgglomeration on;\nagglomerator faceAreaPair;\nmergeLevels 1;\n";
		}
    }
    else {
        solver_str += "preconditioner " + preconditioner_to_str(preconditioner) + ";\n";
    }
    solver_str += "tolerance " + to_string_with_precision(tolerance) + ";\nrelTol "
        + to_string_with_precision(relative_tolerance) + ";\n}\n";
    return solver_str;
}

std::string tostf::foam::Solvers::to_string() const {
    return "solvers {\np " + p.to_string() + "pFinal {\n$p;\nrelTol 0;\n}\n"
           "U " + U.to_string() + "\n}\ncache {\ngrad(U);\n}\n";
}

std::string tostf::foam::Scheme::to_string() const {
    return name + " " + setting + ";\n";
}

std::string tostf::foam::Schemes_settings::to_string() const {
    std::string schemes("ddtSchemes {\n");
    for (auto& s : ddt) {
        schemes += s.to_string();
    }
    schemes += "}\ngradSchemes {\n";
    for (auto& s : grad) {
        schemes += s.to_string();
    }
    schemes += "}\ndivSchemes {\n";
    for (auto& s : div) {
        schemes += s.to_string();
    }
    schemes += "}\nlaplacianSchemes {\n";
    for (auto& s : laplacian) {
        schemes += s.to_string();
    }
    schemes += "}\ninterpolationSchemes {\n";
    for (auto& s : interpolation) {
        schemes += s.to_string();
    }
    schemes += "}\nsnGradSchemes {\n";
    for (auto& s : sn_grad) {
        schemes += s.to_string();
    }
    schemes += "}\n";
    return schemes;
}

tostf::foam::Schemes_settings tostf::foam::schemes_commercial_setup() {
    Schemes_settings schemes;
    schemes.ddt = {{"default", "CrankNicolson 0.9"}};
    schemes.grad = {{"default", "cellLimited Gauss linear 1"}, {"grad(U)", "cellLimited Gauss linear 1"}};
    schemes.div = {
        {"default", "none"}, {"div(phi,U)", "Gauss linearUpwindV grad(U)"},
        {"div(phi,omega)", "Gauss linearUpwind default"}, {"div(phi,k)", "Gauss linearUpwind default"},
        {"div(phi,alpha)", "Gauss vanLeer"}, {"div((nuEff*dev(T(grad(U)))))", "Gauss linear"},
        {"div((nuEff*dev2(T(grad(U)))))", "Gauss linear"}
    };
    schemes.laplacian = {{"default", "Gauss linear limited 1"}};
    schemes.interpolation = {{"default", "linear"}};
    schemes.sn_grad = {{"default", "limited 1"}};
    return schemes;
}

tostf::foam::Schemes_settings tostf::foam::schemes_accurate_setup() {
    Schemes_settings schemes;
    schemes.ddt = {{"default", "backward"}};
    schemes.grad = {{"default", "Gauss leastSquares"}};
    schemes.div = {
        {"default", "none"}, {"div(phi,U)", "Gauss linearUpwindV grad(U)"},
        {"div(phi,omega)", "Gauss linearUpwind default"}, {"div(phi,k)", "Gauss linearUpwind default"},
        {"div(phi,alpha)", "Gauss vanLeer"}, {"div((nuEff*dev(T(grad(U)))))", "Gauss linear"},
        {"div((nuEff*dev2(T(grad(U)))))", "Gauss linear"}
    };
    schemes.laplacian = {{"default", "Gauss linear limited 1"}};
    schemes.interpolation = {{"default", "linear"}};
    schemes.sn_grad = {{"default", "limited 1"}};
    return schemes;
}

tostf::foam::Schemes_settings tostf::foam::schemes_stable_setup() {
    Schemes_settings schemes;
    schemes.ddt = {{"default", "Euler"}};
    schemes.grad = {{"default", "cellLimited Gauss linear 1"}, {"grad(U)", "cellLimited Gauss linear 1"}};
    schemes.div = {
        {"default", "none"}, {"div(phi,U)", "Gauss linearUpwindV grad(U)"},
        {"div(phi,omega)", "Gauss linearUpwind default"}, {"div(phi,k)", "Gauss linearUpwind default"},
        {"div(phi,alpha)", "Gauss vanLeer"}, {"div((nuEff*dev(T(grad(U)))))", "Gauss linear"},
        {"div((nuEff*dev2(T(grad(U)))))", "Gauss linear"}
    };
    schemes.laplacian = {{"default", "Gauss linear limited 0.5"}};
    schemes.interpolation = {{"default", "linear"}};
    schemes.sn_grad = {{"default", "limited 0.5"}};
    return schemes;
}

std::string tostf::foam::transport_model_to_str(const transport_model tm) {
    switch (tm) {
        case transport_model::bird_carreau:
            return "BirdCarreau";
        case transport_model::cross_power_law:
            return "CrossPowerLaw";
        case transport_model::power_law:
            return "powerLaw";
        case transport_model::herschel_bulkley:
            return "HerschelBulkley";
        case transport_model::casson:
            return "Casson";
        case transport_model::newtonian:
        default:
            return "Newtonian";
    }
}

std::vector<tostf::foam::Scalar_field> tostf::foam::fields_from_model(const transport_model model) {
    switch (model) {
        case transport_model::bird_carreau:
            return {
                {"nu0", nu_dims(), 0.001f}, {"nuInf", nu_dims(), 0.00001f},
                {"k", {0, 0, 1, 0, 0, 0, 0}, 1.0f}, {"n", no_dims(), 0.5f}
            };
        case transport_model::cross_power_law:
            return {
                {"nu0", nu_dims(), 0.001f}, {"nuInf", nu_dims(), 0.00001f},
                {"m", {0, 0, 1, 0, 0, 0, 0}, 1.0f}, {"n", no_dims(), 0.5f}
            };
        case transport_model::power_law:
            return {
                {"nuMax", nu_dims(), 0.001f}, {"nuMin", nu_dims(), 0.00001f},
                {"k", nu_dims(), 0.00001f}, {"n", no_dims(), 1.0f}
            };
        case transport_model::herschel_bulkley:
            return {
                {"nu0", nu_dims(), 0.001f}, {"tau0", {0, 2, -2, 0, 0, 0, 0}, 1.0f},
                {"k", nu_dims(), 0.00001f}, {"n", no_dims(), 1.0f}
            };
        case transport_model::casson:
            return {
                {"m", nu_dims(), 3.934986e-6f}, {"tau0", {0, 2, -2, 0, 0, 0, 0}, 2.9032e-6f},
                {"nuMax", nu_dims(), 13.3333e-6f}, {"nuMin", nu_dims(), 3.9047e-6f}
            };
        case transport_model::newtonian:
        default:
            return {{"nu", nu_dims(), 0.0001f}};
    }
}

std::string tostf::foam::Surface_feature_extract::to_string() const {
    return "    extractionMethod    extractFromSurface;\n    includedAngle " + std::to_string(included_angle)
           + ";\n    subsetFeatures {\n        nonManifoldEdges " + bool_to_str(non_manifold_edges) + ";\n"
           "        openEdges " + bool_to_str(open_edges) + ";\n    }\nwriteObj yes;\n";
}

std::string tostf::foam::Castellated_mesh_controls::to_string() const {
    const auto allow_free_standing_zone_faces_string(bool_to_str(allow_free_standing_zone_faces));
    return "    maxLocalCells " + std::to_string(max_local_cells) + ";\n"
           "    maxGlobalCells " + std::to_string(max_global_cells) + ";\n"
           "    minRefinementCells " + std::to_string(min_refinement_cells) + ";\n"
           "    nCellsBetweenLevels " + std::to_string(n_cells_between_levels) + ";\n"
           "    resolveFeatureAngle " + std::to_string(resolve_feature_angle) + ";\n"
           "    allowFreeStandingZoneFaces " + allow_free_standing_zone_faces_string + ";\n";
}

std::string tostf::foam::Snap_controls::to_string() const {
    return "snapControls {\n    nSmoothPatch " + std::to_string(n_smooth_patch) + ";\n"
           "    tolerance " + to_string_with_precision(tolerance) + ";\n"
           "    nSolveIter " + std::to_string(n_solve_iter) + ";\n"
           "    nRelaxIter " + std::to_string(n_relax_iter) + ";\n"
           "    nFeatureSnapIter " + std::to_string(n_feature_snap_iter) + ";\n"
           "    explicitFeatureSnap true;\n}\n";
}

std::string tostf::foam::Dimensions::to_string() const {
    return "[ " + std::to_string(kg) + " " + std::to_string(m) + " "
           + std::to_string(s) + " " + std::to_string(K) + " " + std::to_string(mol) + " "
           + std::to_string(A) + " " + std::to_string(cd) + " ]";
}

std::string gen_control_dict_str(const tostf::foam::Algorithm_settings& algo, const tostf::foam::Control_dict& control_dict) {
    return gen_foam_file_str("controlDict").append("application " + algo.get_application_name() + ";\n")
                                           .append(control_dict.to_string(algo.steady)).append(gen_file_end_str());
}

std::string gen_schemes_str(const tostf::foam::numerical_schemes& schemes) {
    tostf::foam::Schemes_settings s;
    if (schemes == tostf::foam::numerical_schemes::commercial) {
        s = tostf::foam::schemes_commercial_setup();
    }
    else if (schemes == tostf::foam::numerical_schemes::accurate) {
        s = tostf::foam::schemes_accurate_setup();
    }
    else {
        s = tostf::foam::schemes_stable_setup();
    }
    return gen_foam_file_str("fvSchemes").append(s.to_string()).append(gen_file_end_str());
}

std::string gen_solvers_str(const tostf::foam::Solvers& solvers, const tostf::foam::Algorithm_settings& algorithm) {
    std::string algo_str("PIMPLE {\n");
    if (algorithm.get_application_name() == "simpleFoam") {
        algo_str = "SIMPLE {\n";
    }
    else {
        algo_str += "nCorrectors " + std::to_string(algorithm.n_correctors) + ";\n"
            "nOuterCorrectors " + std::to_string(algorithm.n_outer_correctors) + ";\n";
    }
	algo_str += "nNonOrthogonalCorrectors " + std::to_string(algorithm.n_non_orthogonal_correctors) + ";\n"
		"momentumPredictor " + bool_to_str(algorithm.momentum_predictor) + ";\n"
		"pRefCell " + std::to_string(algorithm.ref_cell) + ";\npRefValue " + std::to_string(algorithm.ref_value) + ";\n";
	if (algorithm.get_application_name() == "simpleFoam") {
		algo_str += "residualControl {\np " + to_string_with_precision(algorithm.tolerance_p) + ";\n"
			"U " + to_string_with_precision(algorithm.tolerance_U) + ";\n}\n}\n";
	}
	else {
		algo_str += "residualControl {\np {\n    tolerance " + to_string_with_precision(algorithm.tolerance_p) + ";\n    relTol 0.0;\n}\n"
			"U {\n    tolerance " + to_string_with_precision(algorithm.tolerance_U) + ";\n    relTol 0.0;\n}\n}\n";
	}
    algo_str += "relaxationFactors {\np " + to_string_with_precision(solvers.p.relaxation) + ";\n"
        "U " + to_string_with_precision(solvers.U.relaxation) + ";\n}\n";
    return gen_foam_file_str("fvSolution").append(solvers.to_string()).append(algo_str).append(
        gen_file_end_str());
}

std::string gen_transport_properties_str(const tostf::foam::Blood_flow_properties& blood_flow) {
    auto properties_str = gen_foam_file_str("transportProperties").append("transportModel ")
                                                                  .append(transport_model_to_str(blood_flow.model)).
                                                                  append(";\n");
    if (blood_flow.model != tostf::foam::transport_model::newtonian) {}
    for (auto& tp : blood_flow.transport_fields) {
        properties_str.append(tp.name + " " + tp.dims.to_string() + " " + std::to_string(tp.internal_value) + ";\n");
    }
    if (blood_flow.model != tostf::foam::transport_model::newtonian) {
        properties_str.append("}\n");
    }
    properties_str.append(gen_file_end_str());
    return properties_str;
}

std::string gen_turbulence_properties_str(const tostf::foam::Blood_flow_properties& blood_flow) {
    return gen_foam_file_str("turbulenceProperties") + "simulationType laminar;\n" + gen_file_end_str();
}

std::string gen_initial_U_str(const std::string& name, const tostf::foam::Blood_flow_properties& blood_flow,
                              const std::vector<tostf::Inlet>& inlets) {
    auto U_str = gen_foam_file_str(blood_flow.U.name, "volVectorField") + "dimensions " +
                 blood_flow.U.dims.to_string() + ";\ninternalField uniform ("
                 + std::to_string(blood_flow.U.internal_value.x) + " " + std::to_string(blood_flow.U.internal_value.y)
                 + " " + std::to_string(blood_flow.U.internal_value.z) + ");\nboundaryField {\n";
    for (auto& inlet : inlets) {
        U_str += name + "_" + inlet.name + " {\n";
        if (inlet.boundary.type == tostf::boundary_type::inlet) {
            U_str += "type surfaceNormalFixedValue;\nrefValue uniform " + std::to_string(-inlet.inflow_velocity) + ";\n";
        }
        else {
            U_str += "type " + boundary_condition_to_str(inlet.boundary.U_condition) + ";\n";
            if (inlet.boundary.U_condition == tostf::boundary_condition::dirichlet) {
                const auto value = inlet.boundary.U_value;
                U_str += "value uniform (" + std::to_string(value.x) + " " + std::to_string(value.y) + " "
                    + std::to_string(value.z) + ");\n";
            }
        }
        U_str += "}\n";
    }
    U_str += name + "_" + name + " {\ntype fixedValue;\nvalue uniform (0 0 0);\n}\n}\n" + gen_file_end_str();
    return U_str;
}


std::string gen_initial_p_str(const std::string& name, tostf::foam::Blood_flow_properties blood_flow,
                              const std::vector<tostf::Inlet>& inlets) {
    auto p_str = gen_foam_file_str(blood_flow.p.name, "volScalarField") + "dimensions " +
                 blood_flow.p.dims.to_string() + ";\ninternalField uniform "
                 + std::to_string(blood_flow.p.internal_value) + ";\nboundaryField {\n";
    for (auto& inlet : inlets) {
        if (inlet.boundary.type == tostf::boundary_type::inlet) {
            p_str += name + "_" + inlet.name + " {\ntype zeroGradient;\n";
            // + boundary_condition_to_str(inlet.boundary.p_condition) + ";\n";
        }
        else {
            p_str += name + "_" + inlet.name + " {\ntype fixedValue;\n value uniform 0;\n";
            // + boundary_condition_to_str(inlet.boundary.p_condition) + ";\n";
        }
        // if (inlet.boundary.p_condition == boundary_condition::dirichlet) {
        //     p_str += "value " + std::to_string(inlet.boundary.p_value) + ";\n";
        // }
        p_str += "}\n";
    }
    p_str.append(name + "_" + name + " {\ntype fixedFluxPressure;\nvalue uniform 0;\n}\n}\n").append(gen_file_end_str());
    return p_str;
}

std::string gen_bb_vertices_str(const tostf::Bounding_box& bb, float bb_offset) {
    const auto center = (bb.min + bb.max) / 2.0f;
    const auto min_bb = bb.min + normalize(bb.min - center) * bb_offset;
    const auto max_bb = bb.max + normalize(bb.max - center) * bb_offset;
    const auto min_x = std::to_string(min_bb.x);
    const auto min_y = std::to_string(min_bb.y);
    const auto min_z = std::to_string(min_bb.z);
    const auto max_x = std::to_string(max_bb.x);
    const auto max_y = std::to_string(max_bb.y);
    const auto max_z = std::to_string(max_bb.z);
    return "vertices (\n"
           "    (" + min_x + " " + min_y + " " + max_z + ")\n"
           "    (" + min_x + " " + min_y + " " + min_z + ")\n"
           "    (" + min_x + " " + max_y + " " + min_z + ")\n"
           "    (" + min_x + " " + max_y + " " + max_z + ")\n"
           "    (" + max_x + " " + min_y + " " + max_z + ")\n"
           "    (" + max_x + " " + min_y + " " + min_z + ")\n"
           "    (" + max_x + " " + max_y + " " + min_z + ")\n"
           "    (" + max_x + " " + max_y + " " + max_z + ")\n);\n";
}

std::string gen_surface_feature_extract_dict(const std::string& name, const tostf::foam::Surface_feature_extract& feature_extract) {
    return gen_foam_file_str("surfaceFeatureExtractDict") + name + ".obj {\n"
           + feature_extract.to_string() + "}\n" + gen_file_end_str();
}

std::string gen_castellated_front(const std::string& name, const tostf::foam::Castellated_mesh_controls& castellated,
                                  const std::vector<tostf::Inlet>& inlets) {
    std::string castellated_mesh_control = "castellatedMeshControls {\n" + castellated.to_string() + "\n"
                                           "    features (\n        {\n            file \"" + name + ".eMesh\";\n            level "
                                           + std::to_string(castellated.features_level_refinement) +
                                           ";\n        }\n    );\n"
                                           "    refinementSurfaces {\n        " + name + "{\n            level ("
                                           + std::to_string(castellated.refinement_surfaces_min_level) + " "
                                           + std::to_string(castellated.refinement_surfaces_max_level) +
                                           ");\n            regions {\n";
    for (auto& inlet : inlets) {
        castellated_mesh_control += "                " + name + "_" + inlet.name + " {\n                    levels ("
            + std::to_string(castellated.refinement_surfaces_min_level) + " "
            + std::to_string(castellated.refinement_surfaces_max_level) + ");\n                }\n";
    }
    return castellated_mesh_control + "\n            }\n        }\n    }\n    refinementRegions {}\n";
}

std::string gen_castellated_back(const glm::vec4& location_in_mesh) {
    return "    locationInMesh (" + std::to_string(location_in_mesh.x) + " "
           + std::to_string(location_in_mesh.y) + " " + std::to_string(location_in_mesh.z) + ");\n}\n";
}

std::string gen_regions_str(const std::string& name, const std::vector<tostf::Inlet>& inlets) {
    std::string regions;
    for (auto& inlet : inlets) {
        regions += "            " + inlet.name + " { name " + name + "_" + inlet.name + "; }\n";
    }
    return regions;
}

std::string gen_snappy_hex_mesh_front(const std::string& name, const tostf::foam::Snappy_hex_mesh_controls& snappy,
                                      const std::vector<tostf::Inlet>& inlets) {
    return gen_foam_file_str("snappyHexMeshDict")
           .append("castellatedMesh true;\nsnap " + bool_to_str(snappy.snap_active) + ";\naddLayers ")
           .append(bool_to_str(snappy.add_layers.has_value()) + ";\n")
           .append("geometry {\n    " + name + ".obj {\n        type triSurfaceMesh;\n"
               "        name " + name + ";\n        regions {\n")
           .append(gen_regions_str(name, inlets)).append("        }\n    }\n}\n")
           .append(gen_castellated_front(name, snappy.castellated, inlets));
}

std::string gen_snappy_hex_mesh_back() {
    return std::string("addLayersControls { }\n"
        "meshQualityControls {\n#includeEtc \"caseDicts/meshQualityDict\"\nnSmoothScale 4;\nerrorReduction 0.75;\n}\n"
        "writeFlags (\n    scalarLevels\n    layerSets\n    layerFields // write volScalarField for layer coverage\n);\n"
        "mergeTolerance 1e-6;\n").append(gen_file_end_str());
}

std::string gen_mesh_data_str(const std::vector<unsigned>& indices, const std::vector<glm::vec4>& vertices,
                              const std::vector<glm::vec4>& normals, const int precision = 7) {
    std::stringstream vert_str;
    std::stringstream normal_str;
    vert_str.setf(std::ios::fixed, std::ios::floatfield);
    normal_str.setf(std::ios::fixed, std::ios::floatfield);
    vert_str.precision(precision);
    normal_str.precision(precision);
    std::thread vert_thread([&vert_str, &indices, &vertices]() {
        for (int i = 0; i < static_cast<int>(indices.size()); i++) {
            const auto& v = vertices.at(indices.at(i));
            vert_str << "v " << v.x << " " << v.y << " " << v.z << "\n";
        }
    });
    std::thread normal_thread([&normal_str, &indices, &normals]() {
        for (int i = 0; i < static_cast<int>(indices.size()); i++) {
            const auto& n = normals.at(indices.at(i));
            normal_str << "vn " << n.x << " " << n.y << " " << n.z << "\n";
        }
    });
    vert_thread.join();
    normal_thread.join();
    vert_str << normal_str.str();
    return vert_str.str();
}

std::string gen_mesh_indices_str_from_view(const tostf::Geometry_view& view, const unsigned index_offset = 0) {
    std::stringstream face_str;
    for (int i = 0; i < static_cast<int>(view.indices.size()) / 3; i++) {
        const int index_a = index_offset + i * 3 + 0 + 1;
        const int index_b = index_offset + i * 3 + 1 + 1;
        const int index_c = index_offset + i * 3 + 2 + 1;
        face_str << "f " << index_a << "/" << "/" << index_a
            << " " << index_b << "/" << "/" << index_b
            << " " << index_c << "/" << "/" << index_c << "\n";
    }
    return face_str.str();
}

std::string gen_mesh_export_str(const std::string& name, const std::vector<glm::vec4>& vertices, const std::vector<glm::vec4>& normals,
                                const std::string& wall_indices_str, const std::vector<std::string>& inlet_indices_strs,
                                const std::vector<unsigned>& wall_indices,
                                const std::vector<tostf::Geometry_view>& inlet_views,
                                const std::vector<tostf::Inlet>& inlets) {
    std::stringstream exp_str;
    exp_str << "g " + name + "\n";
    exp_str << gen_mesh_data_str(wall_indices, vertices, normals);
    exp_str << wall_indices_str;
    for (unsigned i = 0; i < inlets.size(); ++i) {
        exp_str << "g " << inlets.at(i).name << "\n";
        exp_str << gen_mesh_data_str(inlet_views.at(i).indices, vertices, normals);
        exp_str << inlet_indices_strs.at(i);
    }
    return exp_str.str();
}

std::string gen_deformed_stats_str(const tostf::Deformed_mesh& deformed, const float scale) {
    std::stringstream stats_str;
    stats_str << "min " << deformed.min_dist_to_ref * scale << "\n";
    stats_str << "max " << deformed.max_dist_to_ref * scale << "\n";
    stats_str << "avg " << deformed.avg_dist_to_ref * scale << "\n";
    stats_str << "sd " << deformed.sd_dist_to_ref * scale << "\n";
    return stats_str.str();
}

void tostf::foam::export_openfoam_cases(const std::string& name, const OpenFOAM_export_info& info, const float scale, const Bounding_box& bb,
                                        const float point_offset, const std::shared_ptr<Mesh>& mesh,
                                        const std::vector<Deformed_mesh>& deformed_meshes, const Geometry_view& wall,
                                        const std::vector<Geometry_view>& inlet_views,
                                        const std::vector<Inlet>& inlets) {
    const auto block_mesh_dict_front = gen_foam_file_str("blockMeshDict").append("scale 1.0;\n");
    auto scaled_bb_min = (bb.min - info.block_mesh.bb_offset) * scale;
    auto scaled_bb_max = (bb.max + info.block_mesh.bb_offset) * scale;
    glm::ivec3 grid_size(ceil((scaled_bb_max - scaled_bb_min) / info.block_mesh.initial_cell_size));
    const auto block_mesh_dict_back = std::string("blocks (\n    hex (0 1 2 3 4 5 6 7) "
                                                  "(" + std::to_string(grid_size.x) + " "
                                                  + std::to_string(grid_size.y) + " "
                                                  + std::to_string(grid_size.z) + ") "
                                                  "simpleGrading (1 1 1)\n);\nboundary ( );\n")
        .append(gen_file_end_str());
    auto bb_diff = glm::vec3(bb.max - bb.min);
    float max_grid_side_length = glm::compMax(bb_diff);
    auto max_side_length = abs(max_grid_side_length / info.decompose_par_dict.max_sub_domains);
    glm::ivec3 subdomain_sizes(ceil(bb_diff / max_side_length));
    auto subdomain_count = subdomain_sizes.x * subdomain_sizes.y * subdomain_sizes.z;
    auto decompose_par_str = gen_foam_file_str("decomposeParDict").append(std::string("numberOfSubdomains " + std::to_string(subdomain_count) + ";\n"
        "method simple;\nsimpleCoeffs {\n    n (" + std::to_string(subdomain_sizes.x) + " " + std::to_string(subdomain_sizes.y) + " " + std::to_string(subdomain_sizes.z) + ");\n"
        "delta 0.001;\n}\ndistributed no;\nroots ( );\n"))
        .append(gen_file_end_str());
    const auto snappy_hex_front = gen_snappy_hex_mesh_front(name, info.snappy_hex_mesh, inlets);
    std::string snappy_hex_back(info.snappy_hex_mesh.snap.to_string() + gen_snappy_hex_mesh_back());
    const auto scale_str = std::to_string(scale);
    const std::string mesh_gen_bash("#!/bin/bash\n" + std::string(source_openfoam) + "\nblockMesh >> mesh_gen_log;surfaceFeatureExtract >> mesh_gen_log;"
                                    "snappyHexMesh -overwrite;transformPoints -scale \"(" + scale_str + " " + scale_str
                                    + " " + scale_str + ")\" >> mesh_gen_log;echo \"test\"\n");
    const std::string run_bash("#!/bin/bash\n" + std::string(source_openfoam) + "\n" + info.algorithm.get_application_name() + " >> log;\n");
	const std::string run_parallel_bash("#!/bin/bash\n" + std::string(source_openfoam) + "\ndecomposePar -force >> par_log;mpirun -np " + std::to_string(subdomain_count) + " " + info.algorithm.get_application_name() + " -parallel >> log;reconstructPar >> par_log;rm -r proc*;\n");

	const std::string mesh_gen_global = "for d in */\ndo\ncd \"${d}\" && ./mesh_gen.sh\ncd ..\ndone\n";
	save_str_to_file(info.export_path / "mesh_gen_all.sh", mesh_gen_global);
    const std::string run_sim_global = "for d in */\ndo\ncd \"${d}\" && ./run_simulation.sh\ncd ..\ndone\n";
    save_str_to_file(info.export_path / "run_simulations.sh", run_sim_global);
    if (info.parallel) {
        const std::string run_sim_global_parallel = "for d in */\ndo\ncd \"${d}\" && ./run_simulation_parallel.sh\ncd ..\ndone\n";
        save_str_to_file(info.export_path / "run_simulations_parallel.sh", run_sim_global_parallel);
    }
    if (info.post_processing.lambda2 || info.post_processing.wall_shear_stress || info.post_processing.q_criterion) {
        std::string postprocess_str = std::string(source_openfoam) + "\nfor d in */\ndo\ncd \"${d}\"\n";
        if (info.post_processing.lambda2) {
            postprocess_str.append("postProcess -func Lambda2 -noZero >> postprocess_log\n");
        }
        if (info.post_processing.q_criterion) {
            postprocess_str.append("postProcess -func Q >> postprocess_log\n");
        }
        if (info.post_processing.wall_shear_stress) {
            postprocess_str.append(info.algorithm.get_application_name() + " -postProcess -func wallShearStress >> postprocess_log\n");
        }
        postprocess_str.append("cd ..\ndone\n");
        save_str_to_file(info.export_path / "post_process.sh", postprocess_str);
    }
    const auto ref_case_path = info.export_path / "reference_case";
    create_directory(ref_case_path);
	save_str_to_file(ref_case_path / "mesh_gen.sh", mesh_gen_bash);
    save_str_to_file(ref_case_path / "run_simulation.sh", run_bash);
    if (info.parallel) {
        save_str_to_file(ref_case_path / "run_simulation_parallel.sh", run_parallel_bash);
    }
    create_directory(ref_case_path / "system");
	auto control_dict_str = gen_control_dict_str(info.algorithm, info.control_dict);
    save_str_to_file(ref_case_path / "system" / "controlDict", control_dict_str);
	auto fv_solution_str = gen_solvers_str(info.solvers, info.algorithm);
    save_str_to_file(ref_case_path / "system" / "fvSolution", fv_solution_str);
	auto fv_schemes_str = gen_schemes_str(info.schemes);
    save_str_to_file(ref_case_path / "system" / "fvSchemes", fv_schemes_str);
	auto feature_dict = gen_surface_feature_extract_dict(name, info.feature_extract);
    save_str_to_file(ref_case_path / "system" / "surfaceFeatureExtractDict", feature_dict);
    create_directory(ref_case_path / "constant");
	auto transport_info = gen_transport_properties_str(info.blood_flow);
    save_str_to_file(ref_case_path / "constant" / "transportProperties", transport_info);
	auto turbulence_str = gen_turbulence_properties_str(info.blood_flow);
    save_str_to_file(ref_case_path / "constant" / "turbulenceProperties", turbulence_str);
    create_directory(ref_case_path / "constant" / "triSurface");
	std::string time_dir_name = "0";
	if (!info.algorithm.steady) {
		time_dir_name += ".";
		for (int zero = 0; zero < info.control_dict.time_precision; ++zero) {
			time_dir_name += "0";
		}
	}
    create_directory(ref_case_path / time_dir_name);
	auto initial_U_str = gen_initial_U_str(name, info.blood_flow, inlets);
    save_str_to_file(ref_case_path / time_dir_name / "U", initial_U_str);
	auto initial_p_str = gen_initial_p_str(name, info.blood_flow, inlets);
    save_str_to_file(ref_case_path / time_dir_name / "p", initial_p_str);
    save_str_to_file(ref_case_path / "system" / "blockMeshDict",
                     block_mesh_dict_front + gen_bb_vertices_str(mesh->get_bounding_box(),
                                                                 info.block_mesh.bb_offset)
                     .append(block_mesh_dict_back));
    const auto loc_in_ref = mesh->find_random_point_in_mesh(point_offset);
    save_str_to_file(ref_case_path / "system" / "snappyHexMeshDict",
                     snappy_hex_front + gen_castellated_back(loc_in_ref).append(snappy_hex_back));
    const auto wall_indices = gen_mesh_indices_str_from_view(wall);
    auto offset = static_cast<unsigned>(wall.indices.size());
    std::vector<std::string> inlet_indices_strings(inlets.size());
    for (int i = 0; i < static_cast<int>(inlet_views.size()); ++i) {
        inlet_indices_strings.at(i) = gen_mesh_indices_str_from_view(inlet_views.at(i), offset);
        offset += static_cast<unsigned>(inlet_views.at(i).indices.size());
    }
    const std::filesystem::path file_name = name + ".obj";
    const auto reference_mesh = gen_mesh_export_str(name, mesh->vertices, mesh->normals, wall_indices, inlet_indices_strings,
                                                    wall.indices, inlet_views, inlets);
    save_str_to_file(ref_case_path / "constant" / "triSurface" / file_name, reference_mesh);
    save_str_to_file(ref_case_path / "reference_case.foam", "");
    for (const auto& d : deformed_meshes) {
		std::filesystem::path exp_path = info.export_path / d.name;
		create_directory(exp_path);
		save_str_to_file(exp_path / "mesh_gen.sh", mesh_gen_bash);
		save_str_to_file(exp_path / "run_simulation.sh", run_bash);
		if (info.parallel) {
			save_str_to_file(exp_path / "run_simulation_parallel.sh", run_parallel_bash);
		}
		create_directory(exp_path / "system");
		save_str_to_file(exp_path / "system" / "controlDict", control_dict_str);
		save_str_to_file(exp_path / "system" / "fvSolution", fv_solution_str);
		save_str_to_file(exp_path / "system" / "fvSchemes", fv_schemes_str);
		save_str_to_file(exp_path / "system" / "surfaceFeatureExtractDict", feature_dict);
		create_directory(exp_path / "constant");
		save_str_to_file(exp_path / "constant" / "transportProperties", transport_info);
		save_str_to_file(exp_path / "constant" / "turbulenceProperties", turbulence_str);
		create_directory(exp_path / "constant" / "triSurface");
		create_directory(exp_path / time_dir_name);
		save_str_to_file(exp_path / time_dir_name / "U", initial_U_str);
		save_str_to_file(exp_path / time_dir_name / "p", initial_p_str);
        save_str_to_file(exp_path / "dist_to_ref.txt", gen_deformed_stats_str(d, scale));
        save_str_to_file(exp_path / "system" / "blockMeshDict",
                         block_mesh_dict_front + gen_bb_vertices_str(calculate_bounding_box(d.vertices),
                                                                     info.block_mesh.bb_offset)
                         .append(block_mesh_dict_back));
        auto deformed_mesh = std::make_shared<Mesh>();
        deformed_mesh->vertices = d.vertices;
        deformed_mesh->indices = mesh->indices;
        Halfedge_mesh he_deformed(deformed_mesh);
        deformed_mesh->normals = he_deformed.calculate_normals();
        const auto loc_in_def = deformed_mesh->find_random_point_in_mesh(point_offset);
        save_str_to_file(exp_path / "system" / "snappyHexMeshDict",
                         snappy_hex_front + gen_castellated_back(loc_in_def).append(snappy_hex_back));
        auto deformed_mesh_str = gen_mesh_export_str(name, d.vertices, deformed_mesh->normals, wall_indices,
                                                     inlet_indices_strings,
                                                     wall.indices, inlet_views, inlets);
        save_str_to_file(exp_path / "constant" / "triSurface" / file_name, deformed_mesh_str);
        save_str_to_file(exp_path / std::string(d.name + ".foam"), "");
    }
    if (info.parallel) {
        save_str_to_file(ref_case_path / "system" / "decomposeParDict", decompose_par_str);
        for (const auto& d : deformed_meshes) {
            save_str_to_file(info.export_path / d.name / "system" / "decomposeParDict", decompose_par_str);
        }
    }
}
