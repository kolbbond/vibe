
// events like mouse move

//  Events.cpp

// include header
#include "Events.hxx"

namespace vibe {
using namespace Magnum;

// constructor
Events::Events() {}

// destructor
Events::~Events() {}

// factory
ShEventsPr Events::create() {
	return std::make_shared<Events>();
}


int Events::mouse_press_event(void* data, Magnum::Platform::Sdl2Application::MouseEvent& event) {
	GuiMain* gui = reinterpret_cast<GuiMain*>(data);

	printf("in mouse press event\n");

	// check button
	if(event.button() == Magnum::Platform::Sdl2Application::MouseEvent::Button::Left) {
		printf("set new previous position\n");
		gui->_previousPosition = gui->positionOnSphere(event.position());
	}

	return 0;
}

int Events::mouse_release_event(void* data, Magnum::Platform::Sdl2Application::MouseEvent& event) {
	GuiMain* gui = reinterpret_cast<GuiMain*>(data);

	printf("in mouse release event\n");

	// check button
	if(event.button() == Magnum::Platform::Sdl2Application::MouseEvent::Button::Left) {
		printf("set new previous position\n");

		gui->_previousPosition = Vector3();
	}

	return 0;
}

int Events::mouse_move_event(void* data, Magnum::Platform::Sdl2Application::MouseMoveEvent& event) {

	using namespace Magnum::Math::Literals;

	GuiMain* gui = reinterpret_cast<GuiMain*>(data);

	// if it's not left or right click
	if(!(event.buttons() & Magnum::Platform::Sdl2Application::MouseMoveEvent::Button::Left) &&
		(!(event.buttons() & Magnum::Platform::Sdl2Application::MouseMoveEvent::Button::Right)))
		return 0;

	// @hey, we need to get SDL window size from here
	//Vector2 delta = 3.0f * Vector2{event.relativePosition()} / Vector2{ImGui::GetWindowSize()};
	Vector2 delta = 3.0f * Vector2{event.relativePosition()} / Vector2{1920, 1080};

	// if left click
	// we rotate
	if((event.buttons() & Magnum::Platform::Sdl2Application::MouseMoveEvent::Button::Left)) {

		gui->_mesher->_transformation =
			Matrix4::rotationX(Rad{delta.y()}) * gui->_mesher->_transformation * Matrix4::rotationY(Rad{delta.x()});

		event.setAccepted();
	} else if((event.buttons() & Magnum::Platform::Sdl2Application::MouseMoveEvent::Button::Right)) {

		static fltp xp = 0;
		xp += delta.x();
		// how do we convert a mouse event to translation
		gui->_mesher->_projection = Magnum::Matrix4::perspectiveProjection(
										35.0_degf, Magnum::Vector2{{1920, 1080}}.aspectRatio(), 0.01f, 10.0f) *
			//Magnum::Matrix4::translation(Magnum::Vector3{delta.x(), delta.y(), 0});
			Magnum::Matrix4::translation(Magnum::Vector3::zAxis(xp));
		//        Magnum::Matrix4::proj
	}

	//    redraw();

	return 0;

	return 0;
}

int Events::mouse_scroll_event(void* data, Magnum::Platform::Sdl2Application::MouseScrollEvent& event) {

	GuiMain* gui = reinterpret_cast<GuiMain*>(data);
	// if it's not left or right click
	//std::cout << "mouse scrolling!" << std::endl;

	// @hey, we need to get SDL window size from here
	//Vector2 delta = 3.0f * Vector2{event.relativePosition()} / Vector2{ImGui::GetWindowSize()};
	Vector2 delta = event.offset();
	//std::cout << "delta x,y: " << delta.x() << "," << delta.y() << std::endl;

	// if left click
	// we rotate

	//static fltp xp = 0;
	//static fltp offset = 10;
	static float dz;
	//xp += delta.x();

	fltp scroll_magnitude = 1;
	if(delta.y() > 0.0) {
		dz = scroll_magnitude;
	} else {
		dz = -scroll_magnitude;
	}

	gui->_mesher->_projection = gui->_mesher->_projection * Magnum::Matrix4::translation(Vector3{0, 0, dz});

	return 0;
}


int Events::key_press_event(void* data, Magnum::Platform::Sdl2Application::KeyEvent& event) {

	GuiMain* gui = reinterpret_cast<GuiMain*>(data);

	std::cout << "key: " << event.keyName().data() << std::endl;


	// movement vectors
	Vector3 dAn; // translation acceleration (L,T,N)
	Vector3 dTn; // rotation (L,T,N)


	// keyboard input checking
	//**********/
	// 6DOF shit
	//**********/
	if(event.key() == Platform::Sdl2Application::KeyEvent::Key::Up) {
		// roll down
		dTn = Vector3{0, -0.1, 0.0};
	} else if(event.key() == Platform::Sdl2Application::KeyEvent::Key::Down) {
		// roll up
		dTn = Vector3{0, 0.1, 0.0};
	} else if(event.key() == Platform::Sdl2Application::KeyEvent::Key::Right) {
		// roll right
		dTn = Vector3{0.1, 0.0, 0.0};
	} else if(event.key() == Platform::Sdl2Application::KeyEvent::Key::Left) {
		// roll left
		dTn = Vector3{-0.1, 0.0, 0.0};
	}
	// @hey, we need key releases

	// additional options
	else if(event.key() == Platform::Sdl2Application::KeyEvent::Key::W) {
		//  speedup longitudinally
		dAn = Vector3{0.1, 0.0, 0.0};
	} else if(event.key() == Platform::Sdl2Application::KeyEvent::Key::D) {
		// yaw right
		dTn = Vector3{0.0, 0.0, -0.1};
	} else if(event.key() == Platform::Sdl2Application::KeyEvent::Key::A) {
		// yaw right
		dTn = Vector3{0.0, 0.0, 0.1};
	} else if(event.key() == Platform::Sdl2Application::KeyEvent::Key::S) {
		//  slowdown longitudinally
		dAn = Vector3{-0.1, 0.0, 0.0};
	}

	else if(event.key() == Platform::Sdl2Application::KeyEvent::Key::P) {
		// display matrix
		std::cout << "show projection matrix" << std::endl;
		std::cout << gui->_mesher->_projection.data() << std::endl;
	}

	// set in objects
	for(size_t i = 0; i < gui->_obj_list.size(); i++) {
		ShObjDataPr myobj = gui->_obj_list[i];

		// check obj flag
		if(myobj->is_moveable()) {
			//			myobj->translate(dRn);
			myobj->add_acceleration(dAn);
			myobj->add_rotation(dTn);
		}
	}

	return 0;
}


// setters

// getters

// static
} // namespace vibe
