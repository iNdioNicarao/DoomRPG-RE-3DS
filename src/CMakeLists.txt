
file(GLOB_RECURSE HEADER_FILES
		"*.hpp"
		"*.h")

file(GLOB_RECURSE SOURCE_FILES
		"*.cpp"
		"*.c")

file(GLOB_RECURSE RESOURCES_FILES
		"*.rc"
		"*.ico")

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "Header Files" FILES ${HEADER_FILES})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "Source Files" FILES ${SOURCE_FILES})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "Resources Files" FILES ${RESOURCES_FILES})

add_executable(${PROJECT_NAME} ${HEADER_FILES} ${SOURCE_FILES} ${RESOURCES_FILES}
)
target_include_directories(${PROJECT_NAME} PRIVATE
		${SDL_INCLUDE_DIRS}
		${SDL_MIXER_INCLUDE_DIRS}
		${ZLIB_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
		${SDL_MAIN_LIBRARY}
		${SDL_MIXER_LIBRARIES}
		mad
		vorbisidec
		ogg
		mikmod
		${SDL_LIBRARIES}
		${CITRO3D_LIB}
		${CTRU_LIB}
		${ZLIB_LIBRARIES}
		m
)
set(DEVKITPRO $ENV{DEVKITPRO})
set(TOOLS_DIR ${DEVKITPRO}/tools/bin)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
		COMMAND ${TOOLS_DIR}/3dsxtool
		${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.elf
		${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.3dsx
		COMMENT "Converting ELF to 3DSX"
)
#add_3dsx_target({PROJECT_NAME})