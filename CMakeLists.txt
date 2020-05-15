cmake_minimum_required(VERSION 3.10)

# set the project name
project(game_engine)

# add_subdirectory(bullet)
# add_subdirectory(imgui)
# add the executable
add_executable(game main.cpp)

target_link_libraries(game libBulletSoftBody.a)
target_link_libraries(game libBulletDynamics.a)
target_link_libraries(game libBulletCollision.a)
target_link_libraries(game libLinearMath.a)

target_include_directories(game PUBLIC
                           "${CMAKE_SOURCE_DIR}"
                           "${CMAKE_SOURCE_DIR}/bullet"
                           "${CMAKE_SOURCE_DIR}/imgui"
                           )

# specify the C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)
target_compile_options(game PUBLIC -w -fpermissive)