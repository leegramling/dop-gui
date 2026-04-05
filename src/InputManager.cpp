#include "InputManager.h"

void InputManager::configure(vsg::CommandLine& arguments)
{
    _windowTraits = vsg::WindowTraits::create();
    _windowTraits->windowTitle = "dop-gui";
    _windowTraits->width = 1280;
    _windowTraits->height = 720;
    _windowTraits->debugLayer = arguments.read({"--debug", "-d"});
    _windowTraits->apiDumpLayer = arguments.read({"--api", "-a"});
    _windowTraits->samples = VK_SAMPLE_COUNT_4_BIT;
}

vsg::ref_ptr<vsg::Window> InputManager::createWindow()
{
    _window = vsg::Window::create(_windowTraits);
    return _window;
}

void InputManager::attachDefaultHandlers(vsg::ref_ptr<vsg::Viewer> viewer, vsg::ref_ptr<vsg::Camera> camera)
{
    viewer->addEventHandler(vsg::CloseHandler::create(viewer));
    viewer->addEventHandler(vsg::Trackball::create(camera));
}

vsg::ref_ptr<vsg::Window> InputManager::window() const
{
    return _window;
}

uint32_t InputManager::configuredWidth() const
{
    return _window ? _window->extent2D().width : _windowTraits->width;
}

uint32_t InputManager::configuredHeight() const
{
    return _window ? _window->extent2D().height : _windowTraits->height;
}
