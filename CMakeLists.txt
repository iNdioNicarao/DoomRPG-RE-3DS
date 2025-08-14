cmake_minimum_required(VERSION 3.25)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/Modules")

project(DoomRPG LANGUAGES C)

if(MSVC)
	add_definitions("/D_CRT_SECURE_NO_WARNINGS")
endif()

find_package(SDL REQUIRED)
find_package(SDL_mixer REQUIRED)
find_package(ZLIB REQUIRED)
find_library(CTRU_LIB ctru PATHS /opt/devkitpro/libctru/lib)
find_library(CITRO3D_LIB citro3d PATHS /opt/devkitpro/portlibs/3ds/lib)
#find_package(FluidSynth REQUIRED)
message(STATUS "CTRU_LIB = ${CTRU_LIB}")
message(STATUS "CITRO3D_LIB = ${CITRO3D_LIB}")

add_subdirectory("${PROJECT_SOURCE_DIR}/src")