cmake_minimum_required(VERSION 3.8)

FUNCTION(ASSIMP_COPY_DEBUG ProjectName)
    add_custom_command(TARGET ${ProjectName} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${EXTERNAL_PATH}/assimp/lib/assimp-vc140-mtd.dll
		$<TARGET_FILE_DIR:${ProjectName}>/assimp-vc140-mtd.dll
    COMMENT "Copying Assimp binaries to '$<TARGET_FILE_DIR:${ProjectName}>'" VERBATIM)
ENDFUNCTION(ASSIMP_COPY_DEBUG)

FUNCTION(ASSIMP_COPY_RELEASE ProjectName)
    add_custom_command(TARGET ${ProjectName} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${EXTERNAL_PATH}/assimp/lib/assimp-vc140-mt.dll
		$<TARGET_FILE_DIR:${ProjectName}>/assimp-vc140-mt.dll
    COMMENT "Copying Assimp binaries to '$<TARGET_FILE_DIR:${ProjectName}>'" VERBATIM)
ENDFUNCTION(ASSIMP_COPY_RELEASE)

if (temp1734_ASSIMP_RELEASE)
	find_library(ASSIMP_LIBRARY
		NAMES assimp-vc140-mt.lib assimp.a
		PATHS ${EXTERNAL_PATH}/assimp/lib/)
else()
	find_library(ASSIMP_LIBRARY
		NAMES assimp-vc140-mtd.lib assimp.a
		PATHS ${EXTERNAL_PATH}/assimp/lib/)
endif()
if (ASSIMP_LIBRARY)
    add_library(Assimp::assimp STATIC IMPORTED)
    set_target_properties(Assimp::assimp PROPERTIES
        IMPORTED_LOCATION ${ASSIMP_LIBRARY}
        INTERFACE_INCLUDE_DIRECTORIES ${EXTERNAL_PATH}/assimp/include/)
endif()
