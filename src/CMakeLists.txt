cmake_minimum_required(VERSION 3.8)

add_library(temp1734
	foam_processing/foam_loader.cpp
	foam_processing/volume_grid.cpp
	aneurysm/aneurysm_viewer.cpp
	display/window.cpp
	display/camera.cpp
	display/gui.cpp
	display/win_mgr.cpp
	display/view.cpp
	display/ui_elements.cpp
	file/file_handling.cpp
	file/stb_impl.cpp
	file/stb_image.cpp
	math/vector_math.cpp
	math/interpolation.cpp
	gpu_interface/gl.cpp
	gpu_interface/debug.cpp
	gpu_interface/buffer.cpp
	gpu_interface/element_buffer.cpp
	gpu_interface/storage_buffer.cpp
	gpu_interface/vbo.cpp
	gpu_interface/vao.cpp
	gpu_interface/texture.cpp
	gpu_interface/fbo.cpp
	gpu_interface/shader_program.cpp
	geometry/geometry.cpp
	geometry/mesh.cpp
	geometry/mesh_handling.cpp
	geometry/primitives.cpp
	mesh_processing/tri_face_mesh.cpp
	mesh_processing/halfedge_mesh.cpp
	mesh_processing/structured_mesh.cpp
	utility/logging.cpp
	data_processing/plane_fitting.cpp
	data_processing/clustering.cpp
	preprocessing/inlet_detection.cpp
	preprocessing/openfoam_exporter.cpp
    preprocessing/compact_grid.cpp
)

add_library(temp1734::temp1734 ALIAS temp1734)
target_include_directories(temp1734
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:src>

)
target_compile_features(temp1734 PUBLIC cxx_std_17)

set_target_properties(temp1734 PROPERTIES
            CXX_STANDARD 17
            CXX_EXTENSIONS OFF
			CXX_STANDARD_REQUIRED ON)

target_link_libraries(temp1734
    PUBLIC
        OpenGL::GL glad::glad glfw::glfw3 glm stb::stb Assimp::assimp imgui::imgui
        OpenMP::OpenMP_CXX tinyfd::tinyfd iconfont::iconfont Eigen3::Eigen
)
if(MSVC)
    target_compile_options(temp1734 PRIVATE /W4 /D_ITERATOR_DEBUG_LEVEL=1 "$<$<CONFIG:RELEASE>:/O2>")
	set_target_properties(temp1734 PROPERTIES LINK_FLAGS "$<$<CONFIG:DEBUG>:/NODEFAULTLIB:MSVCRT")
else()
    target_compile_options(temp1734 PRIVATE -Wall -Wextra -Wshadow -Wnon-virtual-dtor -Wold-style-cast
                            -Wcast-align -Wunused -Woverloaded-virtual -Wpedantic -Wconversion
                            -Wsign-conversion -Wmisleading-indentation -Wduplicated-cond
                            -Wduplicated-branches -Wlogical-op -Wnull-dereference -Wuseless-cast
                            -Wdouble-promotion -Wformat=2)
endif()

export(TARGETS temp1734 imgui glad tinyfd
	   FILE ${CMAKE_CURRENT_BINARY_DIR}/temp1734-exports.cmake NAMESPACE temp1734::)
