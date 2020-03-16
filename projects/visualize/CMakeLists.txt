cmake_minimum_required(VERSION 3.8)

get_filename_component(project_name ${CMAKE_CURRENT_SOURCE_DIR} NAME)
string(REPLACE " " "_" project_name ${project_name})
project(${project_name})

add_executable(${project_name} main.cpp)

if(temp1734_ASSIMP_RELEASE)
	ASSIMP_COPY_RELEASE(${project_name})
else()
	ASSIMP_COPY_DEBUG(${project_name})
endif()

target_link_libraries(${project_name}
    PRIVATE
        temp1734::temp1734
)

target_include_directories(${project_name}
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src>
        $<INSTALL_INTERFACE:src>
)
	if(MSVC)
		target_compile_options(temp1734 PUBLIC /D_ITERATOR_DEBUG_LEVEL=1 "$<$<CONFIG:RELEASE>:/O2>")
	endif()

if (MSVC AND CMAKE_BUILD_TYPE STREQUAL "Debug")
	set_target_properties(${project_name} PROPERTIES LINK_FLAGS "/NODEFAULTLIB:MSVCRT")
endif()
