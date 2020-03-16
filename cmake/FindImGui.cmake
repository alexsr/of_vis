cmake_minimum_required(VERSION 3.8)

add_library(imgui
    ${EXTERNAL_PATH}/imgui/imgui/imgui.cpp
    ${EXTERNAL_PATH}/imgui/imgui/imgui_demo.cpp
    ${EXTERNAL_PATH}/imgui/imgui/imgui_draw.cpp
    ${EXTERNAL_PATH}/imgui/imgui/imgui_widgets.cpp
    ${EXTERNAL_PATH}/imgui/imgui/misc/cpp/imgui_stdlib.cpp
	${SRC_PATH}/display/tostf_imgui_widgets.cpp
	)
    
target_include_directories(imgui INTERFACE ${EXTERNAL_PATH}/imgui/)
target_include_directories(imgui INTERFACE ${EXTERNAL_PATH}/imgui/imgui/)
target_include_directories(imgui PRIVATE ${EXTERNAL_PATH}/imgui/imgui/)

add_library(imgui::imgui ALIAS imgui)

target_link_libraries(imgui
    PRIVATE
        OpenGL::GL glad glfw::glfw3
)


	if(MSVC)
		target_compile_options(imgui PUBLIC /D_ITERATOR_DEBUG_LEVEL=1 "$<$<CONFIG:RELEASE>:/O2>")
	endif()

target_compile_features(imgui INTERFACE cxx_std_17)
