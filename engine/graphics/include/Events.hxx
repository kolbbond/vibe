
// events header
// Events.hh
#pragma once

#include <memory>
#include <iostream>

#include "Magnum/Platform/Sdl2Application.h"

#include "GuiMain.hxx"

namespace vibe {

typedef std::shared_ptr<class Events> ShEventsPr;

class Events {

protected:
	// properties
public:
	// methods
	// constructor
	Events();

	// destructor
	~Events();

	// factory
	static ShEventsPr create();

	// mouse move events
	static int mouse_press_event(void* data, Magnum::Platform::Sdl2Application::MouseEvent& event);
	static int mouse_release_event(void* data, Magnum::Platform::Sdl2Application::MouseEvent& event);
	static int mouse_move_event(void* data, Magnum::Platform::Sdl2Application::MouseMoveEvent& event);
	static int mouse_scroll_event(void* data, Magnum::Platform::Sdl2Application::MouseScrollEvent& event);
	static int key_press_event(void* data, Magnum::Platform::Sdl2Application::KeyEvent& event);

	// setters

	// getters

	// static
};

} // namespace vibe
