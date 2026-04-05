#pragma once

#include <vsg/all.h>

class InputManager
{
public:
    InputManager() = default;

    InputManager(const InputManager&) = delete;
    InputManager& operator=(const InputManager&) = delete;

    void configure(vsg::CommandLine& arguments);
    vsg::ref_ptr<vsg::Window> createWindow();
    void attachDefaultHandlers(vsg::ref_ptr<vsg::Viewer> viewer, vsg::ref_ptr<vsg::Camera> camera);

    vsg::ref_ptr<vsg::Window> window() const;
    uint32_t configuredWidth() const;
    uint32_t configuredHeight() const;

private:
    vsg::ref_ptr<vsg::WindowTraits> _windowTraits;
    vsg::ref_ptr<vsg::Window> _window;
};
