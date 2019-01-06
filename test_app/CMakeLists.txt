cmake_minimum_required(VERSION 3.10)

find_package(assimp REQUIRED)
find_package(glm REQUIRED)
find_package(GLEW REQUIRED)
find_package(SDL2 CONFIG REQUIRED)
add_executable(test_app main.cpp)
target_link_libraries(test_app PUBLIC ${ASSIMP_LIBRARIES} glm brtr::brtr GLEW::GLEW SDL2::SDL2 SDL2::SDL2main)


#target_include_directories(test_app PUBLIC ${ASSIMP_INCLUDE_DIRS})

add_custom_command(
        TARGET test_app POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${ASSIMP_LIBRARY_DIRS}
                ${CMAKE_CURRENT_BINARY_DIR})