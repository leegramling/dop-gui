#pragma once

#include <vsg/all.h>

/**
 * @brief Window and input orchestration boundary.
 */
class InputManager
{
public:
    /**
     * @brief Default construct an input manager.
     */
    InputManager() = default;

    InputManager(const InputManager&) = delete;
    InputManager& operator=(const InputManager&) = delete;

    /**
     * @brief Configure window traits from parsed command-line arguments.
     * @param arguments Parsed command-line helper.
     */
    void configure(vsg::CommandLine& arguments);
    /**
     * @brief Create the application window.
     * @return Created window handle, or null on failure.
     */
    vsg::ref_ptr<vsg::Window> createWindow();
    /**
     * @brief Attach the default viewer event handlers.
     * @param viewer Viewer that should receive handlers.
     * @param camera Camera used by the default trackball handler.
     */
    void attachDefaultHandlers(vsg::ref_ptr<vsg::Viewer> viewer, vsg::ref_ptr<vsg::Camera> camera);

    /**
     * @brief Access the created window, if any.
     * @return Current window handle.
     */
    vsg::ref_ptr<vsg::Window> window() const;
    /**
     * @brief Return the configured or current window width.
     * @return Window width in pixels.
     */
    uint32_t configuredWidth() const;
    /**
     * @brief Return the configured or current window height.
     * @return Window height in pixels.
     */
    uint32_t configuredHeight() const;

private:
    vsg::ref_ptr<vsg::WindowTraits> _windowTraits;
    vsg::ref_ptr<vsg::Window> _window;
};
