
add_compile_definitions(IMGUI_IMPL_OPENGL_LOADER_GLEW)
add_library(imgui imgui.cpp)
add_library(imgui_glfw imgui_impl_glfw.cpp)
add_library(imgui_opengl3 imgui_impl_opengl3.cpp)
add_library(imgui_draw imgui_draw.cpp)
add_library(imgui_widgets imgui_widgets.cpp)
