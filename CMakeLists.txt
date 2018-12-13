cmake_minimum_required(VERSION 3.10)
project(brtr VERSION 0.0.1 DESCRIPTION "BRTR Ray Tracing Library")

find_package(glm REQUIRED)

include(CMakeSources.cmake)

set(BRTR_SRC ${BRTR_CPP} ${BRTR_HPP} ${BRTR_INC})
add_library(brtr STATIC ${BRTR_SRC})

foreach(_source IN ITEMS ${BRTR_SRC})
    get_filename_component(_source_path "${_source}" PATH)
    string(REPLACE "/" "\\" _source_path_msvc "${_source_path}")
    source_group("${_source_path_msvc}" FILES "${_source}")
endforeach()

set(OpenCL_INCLUDE_DIR, ${CMAKE_CURRENT_SOURCE_DIR}/External/opencl-headers-msvc)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
find_package( OpenCL REQUIRED)
target_include_directories(brtr PRIVATE ${OpenCL_INCLUDE_DIR})

target_include_directories(brtr PUBLIC include include/brtr)
target_include_directories(brtr PRIVATE src)
target_link_libraries(brtr PRIVATE glm ${OpenCL_LIBRARY})
target_compile_features(brtr PUBLIC cxx_std_17)
set_target_properties(brtr PROPERTIES PUBLIC_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/include/brtr.hpp)
add_library(brtr::brtr ALIAS brtr)

add_subdirectory(test_app)