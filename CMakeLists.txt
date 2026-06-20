cmake_minimum_required(VERSION 3.20)

project(dop_gui
    VERSION 0.1.0
    DESCRIPTION "Data oriented VulkanSceneGraph application prototype"
    LANGUAGES CXX
)

include(CTest)

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
    src/App.cpp
    src/AppState.cpp
    src/Command.cpp
    src/InputManager.cpp
    src/NewShapePanel.cpp
    src/PanelWindow.cpp
    src/PropertiesPanel.cpp
    src/Query.cpp
    src/SceneInfoPanel.cpp
    src/ScriptRunner.cpp
    src/Theme.cpp
    src/UiManager.cpp
    src/VsgVisualizer.cpp
    src/WindowManager.cpp
    src/Widgets.cpp
    src/main.cpp
)

add_dependencies(dop-gui dop-gui-shaders)

target_link_libraries(dop-gui
    PRIVATE
        vsg::vsg
        vsgImGui
)

target_compile_features(dop-gui PRIVATE cxx_std_20)
target_compile_definitions(dop-gui PRIVATE
    DOP_GUI_SHADER_DIR="${DOP_GUI_SHADER_DIR}"
    DOP_GUI_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)

if(BUILD_TESTING)
    add_executable(dop-gui-json-result-test tests/JsonResultTest.cpp)
    target_compile_features(dop-gui-json-result-test PRIVATE cxx_std_20)
    add_test(
        NAME dop_gui_cpp_json_result
        COMMAND dop-gui-json-result-test $<TARGET_FILE:dop-gui>
    )

    add_test(
        NAME dop_gui_query_window_size
        COMMAND dop-gui --query view.window.size
    )
    set_tests_properties(dop_gui_query_window_size PROPERTIES
        PASS_REGULAR_EXPRESSION "\"ok\":true.*\"query\":\"view.window.size\".*\"width\":1600.*\"height\":900"
    )

    add_test(
        NAME dop_gui_command_scene_load
        COMMAND dop-gui --command scene.load.cubes
    )
    set_tests_properties(dop_gui_command_scene_load PROPERTIES
        PASS_REGULAR_EXPRESSION "\"ok\":true.*\"command\":\"scene.load.cubes\".*\"scene\":\"cubes\".*cube_left.*cube_center.*cube_right"
    )

    add_test(
        NAME dop_gui_command_app_exit
        COMMAND dop-gui --command app.exit
    )
    set_tests_properties(dop_gui_command_app_exit PROPERTIES
        PASS_REGULAR_EXPRESSION "\"ok\":true.*\"command\":\"app.exit\".*\"exitRequested\":true"
    )

    add_test(
        NAME dop_gui_command_sleep
        COMMAND dop-gui --command sleep.ms=1
    )
    set_tests_properties(dop_gui_command_sleep PROPERTIES
        PASS_REGULAR_EXPRESSION "\"ok\":true.*\"command\":\"sleep.ms=1\".*\"sleptMs\":1"
    )

    add_test(
        NAME dop_gui_ui_registry
        COMMAND dop-gui --ui-test-mode --query ui.widgets
    )
    set_tests_properties(dop_gui_ui_registry PROPERTIES
        PASS_REGULAR_EXPRESSION "\"ok\":true.*menubar-main.*window-size.*display-grid.*background-color.*scene-select.*selected-object"
    )

    add_test(
        NAME dop_gui_ui_background_input
        COMMAND dop-gui --ui-test-mode --script "${CMAKE_CURRENT_SOURCE_DIR}/tests/ui_background_cli.json5"
    )
    set_tests_properties(dop_gui_ui_background_input PROPERTIES
        PASS_REGULAR_EXPRESSION "\"ok\":true.*panel-bgcolor.*#0000FF.*\"query\":\"view.background.color\".*#0000FF"
    )

    add_test(
        NAME dop_gui_ui_grid_toggle
        COMMAND dop-gui --ui-test-mode --script "${CMAKE_CURRENT_SOURCE_DIR}/tests/ui_grid_cli.json5"
    )
    set_tests_properties(dop_gui_ui_grid_toggle PROPERTIES
        PASS_REGULAR_EXPRESSION "\"ok\":true.*panel-display-grid.*\"boolValue\":false"
    )

    add_test(
        NAME dop_gui_ui_scene_click
        COMMAND dop-gui --ui-test-mode --script "${CMAKE_CURRENT_SOURCE_DIR}/tests/ui_scene_click_cli.json5"
    )
    set_tests_properties(dop_gui_ui_scene_click PROPERTIES
        PASS_REGULAR_EXPRESSION "\"ok\":true.*menuitem-scene-cubes.*cube_left.*cube_center.*cube_right"
    )

    add_test(
        NAME dop_gui_ui_scene_create
        COMMAND dop-gui --ui-test-mode --script "${CMAKE_CURRENT_SOURCE_DIR}/tests/ui_scene_create_cli.json5"
    )
    set_tests_properties(dop_gui_ui_scene_create PROPERTIES
        PASS_REGULAR_EXPRESSION
        "\"query\":\"ui.panel.panel-new-shape\".*\"open\":false.*;.*\"command\":\"ui.test.click.menuitem-scene-create\".*menuitem-scene-create.*;.*\"query\":\"ui.panel.panel-new-shape\".*\"open\":true.*;.*\"query\":\"ui.panel.panel-new-shape.widget.shape-kind\".*Sphere.*;.*\"command\":\"ui.test.panel.panel-new-shape.click.cancel\".*\"widgetId\":\"cancel\".*;.*\"query\":\"ui.panel.panel-new-shape\".*\"open\":false"
    )

    add_test(
        NAME dop_gui_ui_extended_widgets
        COMMAND dop-gui --ui-test-mode --script "${CMAKE_CURRENT_SOURCE_DIR}/tests/ui_extended_widgets_cli.json5"
    )
    set_tests_properties(dop_gui_ui_extended_widgets PROPERTIES
        PASS_REGULAR_EXPRESSION "\"ok\":true.*\"query\":\"ui.panel.panel-scene-info.widget.scene-select\".*\"widgetId\":\"scene-select\".*shapes.*\"query\":\"ui.panel.panel-scene-info.widget.theme-light\".*\"widgetId\":\"theme-light\".*\"boolValue\":true.*\"query\":\"ui.widget.panel-scene-select\".*\"widgetId\":\"scene-select\".*shape_strip"
    )

    add_test(
        NAME dop_gui_ui_popup_table
        COMMAND dop-gui --ui-test-mode --script "${CMAKE_CURRENT_SOURCE_DIR}/tests/ui_popup_table_cli.json5"
    )
    set_tests_properties(dop_gui_ui_popup_table PROPERTIES
        PASS_REGULAR_EXPRESSION "\"ok\":true.*popup-scene-summary.*\"boolValue\":true.*popup-scene-summary-name.*Scene: bootstrap.*panel-scene-table.*\"textValue\":\"4\".*table-scene-objects-row-bootstrap_triangle-kind.*triangle"
    )


    add_test(
        NAME dop_gui_ui_properties_panel
        COMMAND dop-gui --ui-test-mode --script "${CMAKE_CURRENT_SOURCE_DIR}/tests/ui_properties_cli.json5"
    )
    set_tests_properties(dop_gui_ui_properties_panel PROPERTIES
        PASS_REGULAR_EXPRESSION
        "\"query\":\"data.scene.selection\".*bootstrap_cube;\
\"query\":\"ui.panel.panel-properties.widget.selected-object\".*combo.*bootstrap_cube.*\"widgetId\":\"selected-object\";\
\"query\":\"ui.widget.panel-properties-selected-object-label\".*text.*Selected Object;\
\"query\":\"ui.panel.panel-properties.widget.position-x\".*input_double.*1.25 m;\
\"query\":\"ui.panel.panel-properties.widget.rotation-z\".*45.00 deg;\
\"query\":\"ui.widget.input-properties-scale-z\".*input_double.*0.900000;\
\"query\":\"data.scene.object.bootstrap_cube.transform\".*1.25.*0.25.*45.*0.9"
    )

    add_test(
        NAME dop_gui_ui_new_shape_panel
        COMMAND dop-gui --ui-test-mode --script "${CMAKE_CURRENT_SOURCE_DIR}/tests/ui_new_shape_cli.json5"
    )
    set_tests_properties(dop_gui_ui_new_shape_panel PROPERTIES
        PASS_REGULAR_EXPRESSION
        "\"query\":\"ui.panel.panel-new-shape\".*\"open\":false.*;.*\
\"query\":\"ui.panel.panel-new-shape\".*\"open\":true.*;.*\
\"query\":\"ui.panel.panel-new-shape\".*\"open\":false.*;.*\
\"query\":\"ui.panel.panel-new-shape\".*\"open\":false.*;.*\
\"query\":\"ui.panel.panel-new-shape\".*\"open\":false.*;.*\
\"query\":\"ui.panel.panel-new-shape.widget.shape-kind\".*Sphere.*;.*\
\"query\":\"ui.panel.panel-new-shape\".*\"open\":false.*;.*\
\"query\":\"data.scene.selection\".*pyramid_1;.*\
\"query\":\"data.scene.object.sphere_1\".*\"kind\":\"sphere\".*\"position\":\\[1.5,-0.25,0.75\\].*\"rotation\":\\[0,0,30\\].*\"scale\":\\[1.25,1.25,0.8\\].*\"colorHex\":\"#00FF00\";.*\
\"query\":\"data.scene.object.torus_1\".*\"kind\":\"torus\".*\"position\":\\[-1,0,0.2\\].*\"rotation\":\\[0,45,0\\].*\"scale\":\\[0.9,0.9,0.9\\].*\"colorHex\":\"#FF00FF\";.*\
\"query\":\"data.scene.object.pyramid_1\".*\"kind\":\"pyramid\".*\"position\":\\[0,1.2,0.3\\].*\"rotation\":\\[0,0,15\\].*\"scale\":\\[1.1,1.1,1.3\\].*\"colorHex\":\"#FFA500\""
    )

    add_test(
        NAME dop_gui_ui_property_action_order
        COMMAND dop-gui --ui-test-mode --script "${CMAKE_CURRENT_SOURCE_DIR}/tests/ui_property_order_cli.json5"
    )
    set_tests_properties(dop_gui_ui_property_action_order PROPERTIES
        PASS_REGULAR_EXPRESSION
        "\"query\":\"data.scene.object.bootstrap_triangle.transform\".*\"position\":\\[0,0,0\\].*\
\"command\":\"ui.test.panel.panel-properties.set_text.position-y=2.50 m\".*\
\"query\":\"ui.panel.panel-properties.widget.position-y\".*\"textValue\":\"2.50 m\".*\
\"query\":\"data.scene.object.bootstrap_triangle.transform\".*\"position\":\\[0,2.5,0\\].*\
\"command\":\"ui.test.panel.panel-properties.set_text.position-x=1.25 m\".*\
\"query\":\"ui.panel.panel-properties.widget.position-x\".*\"textValue\":\"1.25 m\".*\
\"query\":\"data.scene.object.bootstrap_triangle.transform\".*\"position\":\\[1.25,2.5,0\\]"
    )

    add_test(
        NAME dop_gui_regression_cli
        COMMAND dop-gui --ui-test-mode --script "${CMAKE_CURRENT_SOURCE_DIR}/tests/regression_cli.json5"
    )
    set_tests_properties(dop_gui_regression_cli PROPERTIES
        PASS_REGULAR_EXPRESSION "\"ok\":true.*\"scene\":\"cubes\".*\"exitRequested\":true.*cube_left.*panel-display-grid.*\"boolValue\":false.*panel-bgcolor.*#0000FF.*\"query\":\"view.background.color\".*#0000FF"
    )

    add_test(
        NAME dop_gui_regression_actions
        COMMAND dop-gui --ui-test-mode --script "${CMAKE_CURRENT_SOURCE_DIR}/tests/regression_actions.json5"
    )
    set_tests_properties(dop_gui_regression_actions PROPERTIES
        PASS_REGULAR_EXPRESSION "\"ok\":true.*\"actions\":\\[.*\"scene\":\"cubes\".*cube_left.*panel-display-grid.*\"boolValue\":false.*panel-bgcolor.*#0000FF.*\"query\":\"view.background.color\".*#0000FF.*\"exitRequested\":true"
    )

    add_test(
        NAME dop_gui_script_smoke
        COMMAND dop-gui --script "${CMAKE_CURRENT_SOURCE_DIR}/tests/smoke_cli.json5"
    )
    set_tests_properties(dop_gui_script_smoke PROPERTIES
        PASS_REGULAR_EXPRESSION "\"ok\":true.*bootstrap_cube.*bootstrap_rectangle.*bootstrap_strip.*\"query\":\"runtime.capabilities\""
    )

    add_test(
        NAME dop_gui_script_mutate
        COMMAND dop-gui --script "${CMAKE_CURRENT_SOURCE_DIR}/tests/mutate_cli.json5"
    )
    set_tests_properties(dop_gui_script_mutate PROPERTIES
        PASS_REGULAR_EXPRESSION "\"ok\":true.*\"position\":\\[0.5,0,0.5\\].*\"eye\":\\[0.5,-4.5,2.5\\]"
    )
endif()
