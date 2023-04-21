# SPDX-FileCopyrightText: 2023 wzyforgit
#
# SPDX-License-Identifier: AGPL-3.0-or-later

cmake_minimum_required(VERSION 3.13)

set(BIN_NAME yolov5s)
project(${BIN_NAME})

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
find_package(PkgConfig REQUIRED)

#for qt
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(QtModule Core Gui Widgets)
find_package(Qt5 REQUIRED ${QtModule})

#for ncnn
pkg_check_modules(InferenceEngine REQUIRED IMPORTED_TARGET ncnn)
find_package(OpenMP)
if(OPENMP_FOUND)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fopenmp")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fopenmp")
endif()

#for sources
aux_source_directory(. allSource)
add_executable(${BIN_NAME} ${allSource})

target_include_directories(${BIN_NAME} PUBLIC
    Qt5::Core Qt5::Gui Qt5::Widgets
    ${PROJECT_INCLUDE}
    ${InferenceEngine_INCLUDE_DIRS}
)

target_link_libraries(${BIN_NAME} PRIVATE
    Qt5::Core Qt5::Gui Qt5::Widgets
    ${TARGET_LIBS}
    ${InferenceEngine_LIBRARIES}
)
