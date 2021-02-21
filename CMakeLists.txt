cmake_minimum_required(VERSION 3.10)

# set(CXX "g++")
# set the project name
project(game_engine)

# add_subdirectory(bullet)
# add the executable

include_directories(${CMAKE_SOURCE_DIR}/imgui)
# add_subdirectory(imgui)

# add_subdirectory(bullet)
# add_library(game libBulletSoftBody.a)
# add_library(game libBulletDynamics.a)
# add_library(game libBulletCollision.a)
# add_library(game libLinearMath.a)

add_compile_definitions(IMGUI_IMPL_OPENGL_LOADER_GLEW GLM_GTX_intersect)
file(GLOB SOURCES "*.cpp"  "imgui/*.cpp" "imgui/guizmo/*.cpp")
 
add_executable(game ${SOURCES})

# target_link_libraries(game imgui)
# target_link_libraries(game imgui_glfw)
# target_link_libraries(game imgui_opengl3)
# target_link_libraries(game imgui_draw)
# target_link_libraries(game imgui_widgets)
target_link_libraries(game GL)
target_link_libraries(game GLU)
target_link_libraries(game GLEW)
target_link_libraries(game glfw)
target_link_libraries(game X11)
target_link_libraries(game Xxf86dga)
target_link_libraries(game Xrandr)
target_link_libraries(game pthread)
target_link_libraries(game Xi)
target_link_libraries(game SOIL)
target_link_libraries(game assimp)
target_link_libraries(game alut)
target_link_libraries(game openal)
target_link_libraries(game tbb)
target_link_libraries(game boost_serialization)

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

add_custom_command(
    TARGET game POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/config.txt
            ${CMAKE_CURRENT_BINARY_DIR}/config.txt)

# specify the C++ standard
add_definitions(-DGLM_FORCE_SWIZZLE)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)
option(BUILD_DEBUG "build un-optimized with debugger info" ON)
if(BUILD_DEBUG)
    target_compile_options(game PUBLIC -fpermissive -Wreturn-type -g -std=c++17 -Wfatal-errors)
else()
    target_compile_options(game PUBLIC -fpermissive -Wreturn-type -g -O3 -std=c++17 -Wfatal-errors)
endif()