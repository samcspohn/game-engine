cmake_minimum_required(VERSION 3.10)

set(CXX "clang++")
# set the project name
project(game_engine)

# add_subdirectory(bullet)
# add the executable

include_directories(${CMAKE_SOURCE_DIR}/imgui)
add_subdirectory(imgui)
set(BULLET2_MULTITHREADING ON)
include_directories(${CMAKE_SOURCE_DIR}/bullet)
add_subdirectory(bullet)

# add_subdirectory(bullet)
# add_library(game libBulletSoftBody.a)
# add_library(game libBulletDynamics.a)
# add_library(game libBulletCollision.a)
# add_library(game libLinearMath.a)
file(GLOB SOURCES "*.cpp")
 
add_executable(game ${SOURCES})

target_link_libraries(game imgui)
target_link_libraries(game imgui_glfw)
target_link_libraries(game imgui_opengl3)
target_link_libraries(game imgui_draw)
target_link_libraries(game imgui_widgets)
target_link_libraries(game ${CMAKE_BINARY_DIR}/bullet/src/BulletSoftBody/libBulletSoftBody.a)
target_link_libraries(game ${CMAKE_BINARY_DIR}/bullet/src/BulletDynamics/libBulletDynamics.a)
target_link_libraries(game ${CMAKE_BINARY_DIR}/bullet/src/BulletCollision/libBulletCollision.a)
target_link_libraries(game ${CMAKE_BINARY_DIR}/bullet/src/LinearMath/libLinearMath.a)
target_link_libraries(game GL)
target_link_libraries(game GLU)
target_link_libraries(game GLEW)
target_link_libraries(game glfw)
target_link_libraries(game X11)
target_link_libraries(game Xxf86vm)
target_link_libraries(game Xrandr)
target_link_libraries(game pthread)
target_link_libraries(game Xi)
target_link_libraries(game SOIL)
target_link_libraries(game assimp)
target_link_libraries(game alut)
target_link_libraries(game openal)
target_link_libraries(game ode)

target_include_directories(game PUBLIC
                           ${CMAKE_SOURCE_DIR}
                           ${CMAKE_SOURCE_DIR}/bullet/src
                           ${CMAKE_SOURCE_DIR}/imgui
                           )
FIND_PACKAGE( OpenMP)
if(OPENMP_FOUND)
    message("OPENMP FOUND")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
endif()
include_directories(SYSTEM ${OpenMP_INCLUDE_PATH})
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
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)
option(BUILD_DEBUG "build un-optimized with debugger info" ON)
if(BUILD_DEBUG)
    target_compile_options(game PUBLIC -w -g -fpermissive -fopenmp)
else()
    target_compile_options(game PUBLIC -w -g -fpermissive -fopenmp -O3)
endif()