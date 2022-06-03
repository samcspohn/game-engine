cmake_minimum_required(VERSION 3.10)

# set(CMAKE_CXX_COMPILER g++)
# set(CMAKE_C_COMPILER g)
# set(CXX "g++")
# set the project name
project(game_engine)

include_directories(${CMAKE_SOURCE_DIR}/imgui
                    ${CMAKE_SOURCE_DIR}/imgui/imgui-flame-graph
                    ${CMAKE_SOURCE_DIR}/_rendering )

include_directories(${CMAKE_SOURCE_DIR}/PhysX/pxshared/include ${CMAKE_SOURCE_DIR}/PhysX/physx/include/)


set( PHYSX_PATH ${CMAKE_CURRENT_SOURCE_DIR}/PhysX/physx )

set( PHYSX_ROOT_DIR ${PHYSX_PATH} ) #This is needed for $ENV{PHYSX_PATH}/compiler/public/CMakeLists.txt
set( PHYSX_INCLUDE_DIRS {PHYSX_PATH}/include/ {PHYSX_PATH}/../pxshared/include/ )
set( PHYSX_LIBRARIES
        PhysXExtensions
        PhysX
        PhysXPvdSDK
        PhysXVehicle
        PhysXCharacterKinematic
        PhysXCooking
        PhysXCommon
        PhysXFoundation
        # SnippetUtils
        )

# include_directories(${PHYSX_INCLUDE_DIRS})

add_compile_options(--no-warn -O3)
# add_compile_options(--no-warn -Os)

# add_definitions(-D_DEBUG)
add_definitions(-DNDEBUG)
set(TARGET_BUILD_PLATFORM "linux") # has to match the TARGET_BUILD_PLATFORM in $ENV{PHYSX_PATH}/physix/buildtools/cmake_generate_projects.py
set(PX_BUILDSNIPPETS OFF CACHE BOOL "Generate the snippets")
set(PX_BUILDPUBLICSAMPLES OFF CACHE BOOL "Generate the samples projects")
set(PX_GENERATE_STATIC_LIBRARIES OFF CACHE BOOL "Generate static libraries")
set(PX_FLOAT_POINT_PRECISE_MATH OFF CACHE BOOL "Float point precise math")
set(NV_USE_STATIC_WINCRT OFF CACHE BOOL "Use the statically linked windows CRT")
set(NV_USE_DEBUG_WINCRT OFF CACHE BOOL "Use the debug version of the CRT")
set(PXSHARED_PATH ${PHYSX_PATH}/../pxshared)
set(PXSHARED_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX})
set(CMAKEMODULES_VERSION "1.27")
set(CMAKEMODULES_PATH ${PHYSX_PATH}/../externals/cmakemodules)
set(PX_OUTPUT_LIB_DIR ${CMAKE_CURRENT_BINARY_DIR}/externals/physx)
set(PX_OUTPUT_BIN_DIR ${CMAKE_CURRENT_BINARY_DIR}/externals/physx)

add_subdirectory(${PHYSX_PATH}/compiler/public externals/physx)
# set(PHYSX_ROOT_DIR ${CMAKE_SOURCE_DIR}/PhysX/physx) 
# add_subdirectory(PhysX/physx/compiler/public)

add_compile_definitions(IMGUI_IMPL_OPENGL_LOADER_GLEW
 GLM_GTX_intersect #define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
 #define GLM_FORCE_SIMD_AVX2
 )
file(GLOB SOURCES "*.cpp"
    "imgui/*.cpp"
    "imgui/guizmo/*.cpp"
    "imgui/imgui-flame-graph/*.cpp"
    "_rendering/*.cpp" 
    "components/*.cpp"
    "physics/*.cpp"
    "lighting/*.cpp"
    # "test_project/*.cpp"
    "particles/*.cpp")

file(GLOB SOURCES2 ${SOURCES} "builder/game.cpp")
file(GLOB SOURCES ${SOURCES} "main/main.cpp")

add_executable(game ${SOURCES})
add_library(game_engine STATIC ${SOURCES2})
set_target_properties(game_engine PROPERTIES
    VERSION 1
    SOVERSION 1
    PUBLIC_HEADER game_engine.h)
target_include_directories(game_engine PUBLIC
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/imgui
    ${CMAKE_SOURCE_DIR}/components
	PRIVATE ${CMAKE_SOURCE_DIR}/physx/PhysX/physx/include/
	# PRIVATE ${CMAKE_SOURCE_DIR}/physx/PhysX/source/physxextensions/src
    ${CMAKE_SOURCE_DIR}/physx/PhysX/pxshared/include
)

set_target_properties(game PROPERTIES ENABLE_EXPORTS 1)
target_link_libraries(game GL GLU GLEW glfw X11 Xxf86dga Xrandr pthread Xi SOIL assimp alut openal tbb boost_serialization yaml-cpp dl)
target_link_libraries(game ${PHYSX_LIBRARIES})


target_include_directories(game PUBLIC
                           ${CMAKE_SOURCE_DIR}
                           ${CMAKE_SOURCE_DIR}/imgui
                           )

add_custom_command(
    TARGET game POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/res
            ${CMAKE_CURRENT_BINARY_DIR}/res)



# specify the C++ standard
add_definitions(-DGLM_FORCE_SWIZZLE -D_DEBUG)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)
option(BUILD_DEBUG "build un-optimized with debugger info" ON)
if(BUILD_DEBUG)
    target_compile_options(game PUBLIC -fpermissive  -fPIC -Wreturn-type -g -std=c++17 -ffast-math -Werror=return-type)
    target_compile_options(game_engine PUBLIC -fpermissive  -fPIC -Wreturn-type -g -std=c++17 -ffast-math -Werror=return-type)
else()
    add_definitions(-DRELEASE)
    target_compile_options(game PUBLIC -fpermissive -fPIC -Wreturn-type -g -O3  -std=c++17 -ffast-math -Werror=return-type)
    target_compile_options(game_engine PUBLIC -fpermissive -fPIC -Wreturn-type -g -O3 -std=c++17 -ffast-math -Werror=return-type)
endif()