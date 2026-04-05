cmake_minimum_required(VERSION 3.20)

project(dop_gui
    VERSION 0.1.0
    DESCRIPTION "Data oriented VulkanSceneGraph application prototype"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Prefer the local VSG install tree described in the repo docs.
list(APPEND CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../vsg_deps/install")

find_package(vsg 1.1.13 REQUIRED)
find_program(GLSLANG_VALIDATOR NAMES glslangValidator REQUIRED)

add_subdirectory(external/vsgImGui)

set(DOP_GUI_SHADER_DIR "${CMAKE_CURRENT_BINARY_DIR}/shaders")
set(DOP_GUI_SHADER_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/shaders/triangle.vert"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/shaders/triangle.frag"
)
set(DOP_GUI_SHADER_OUTPUTS)

foreach(shader_source IN LISTS DOP_GUI_SHADER_SOURCES)
    get_filename_component(shader_name "${shader_source}" NAME)
    set(shader_output "${DOP_GUI_SHADER_DIR}/${shader_name}.spv")
    list(APPEND DOP_GUI_SHADER_OUTPUTS "${shader_output}")

    add_custom_command(
        OUTPUT "${shader_output}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${DOP_GUI_SHADER_DIR}"
        COMMAND "${GLSLANG_VALIDATOR}" -V "${shader_source}" -o "${shader_output}"
        DEPENDS "${shader_source}"
        COMMENT "Compiling shader ${shader_name}"
        VERBATIM
    )
endforeach()

add_custom_target(dop-gui-shaders DEPENDS ${DOP_GUI_SHADER_OUTPUTS})

add_executable(dop-gui
    src/main.cpp
)

add_dependencies(dop-gui dop-gui-shaders)

target_link_libraries(dop-gui
    PRIVATE
        vsg::vsg
        vsgImGui
)

target_compile_features(dop-gui PRIVATE cxx_std_20)
target_compile_definitions(dop-gui PRIVATE DOP_GUI_SHADER_DIR="${DOP_GUI_SHADER_DIR}")
