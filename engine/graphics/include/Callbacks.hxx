#pragma once

#include <memory>

#include <armadillo>

//#include "smg/GuiBase.hh"
//#include "smg/ImGuiFileDialog.h"
//#include "smg/Plotter.hh"
#include "vibe/audio/AudioFile.hxx"
#include "vibe/audio/NoiseMaker.hxx"
#include "GuiMain.hxx"

namespace vibe { namespace gfx {

typedef std::shared_ptr<class Callbacks> ShCallbacksPr;

class Callbacks {

protected:
	// properties
public:
	// methods
	// constructor
	Callbacks();

	// destructor
	~Callbacks();

	// factory
	static ShCallbacksPr create();

	static std::string window_filedialog(bool&);

	static int callback_main(void* data);
	static int callback_rad(void* data);
	static int callback_audiofile(void* data);
	static int callback_drawobject(void* data);

	// setters

	// getters

	// static
};

}} // namespace vibe::gfx
