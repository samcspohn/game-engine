cmake_minimum_required(VERSION 3.10)

# set(CXX "g++")
# set the project name
project(game_engine)

include_directories(${CMAKE_SOURCE_DIR}/imgui ${CMAKE_SOURCE_DIR}/_rendering)

add_compile_definitions(IMGUI_IMPL_OPENGL_LOADER_GLEW
 GLM_GTX_intersect #define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
 #define GLM_FORCE_SIMD_AVX2
 )
file(GLOB SOURCES "*.cpp"
    "imgui/*.cpp"
    "imgui/guizmo/*.cpp"
    "_rendering/*.cpp" 
    "components/*.cpp"
    "physics/*.cpp"
    "lighting/*.cpp"
    # "test_project/*.cpp"
    "particles/*.cpp")
 
add_executable(game ${SOURCES})


set_target_properties(game PROPERTIES ENABLE_EXPORTS 1)
target_link_libraries(game GL GLU GLEW glfw X11 Xxf86dga Xrandr pthread Xi SOIL assimp alut openal tbb boost_serialization yaml-cpp dl)

target_include_directories(game PUBLIC
                           ${CMAKE_SOURCE_DIR}
                        #    ${CMAKE_SOURCE_DIR}/bullet/src
                           ${CMAKE_SOURCE_DIR}/imgui
                           )

add_custom_command(
    TARGET game POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/res
            ${CMAKE_CURRENT_BINARY_DIR}/res)



# specify the C++ standard
add_definitions(-DGLM_FORCE_SWIZZLE)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)
option(BUILD_DEBUG "build un-optimized with debugger info" ON)
if(BUILD_DEBUG)
    target_compile_options(game PUBLIC -fpermissive  -fPIC -Wreturn-type -g -std=c++17 -ffast-math -Werror=return-type)
else()
    target_compile_options(game PUBLIC -fpermissive -fPIC -Wreturn-type -g -O3 -std=c++17 -ffast-math -fno-inline -fno-omit-frame-pointer -Werror=return-type)
endif()