cmake_minimum_required(VERSION 3.10)
project(test_app)

find_package(assimp REQUIRED)
find_package(glm REQUIRED)
find_package(GLEW REQUIRED)
add_executable(test_app main.cpp)
target_link_libraries(test_app PUBLIC ${ASSIMP_LIBRARIES} glm brtr::brtr GLEW::GLEW)


#target_include_directories(test_app PUBLIC ${ASSIMP_INCLUDE_DIRS})

add_custom_command(
        TARGET test_app POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${ASSIMP_LIBRARY_DIRS}
                ${CMAKE_CURRENT_BINARY_DIR})