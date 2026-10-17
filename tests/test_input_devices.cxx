// display input devices form libsoundio

#include "vibe/common/Log.hxx"
#include "vibe/audio/LibSoundIo.hxx"

int main() {

	// create log
	vibe::ShLogPr lg = vibe::Log::create();

	lg->msg("Testing input devices...");

	return 0;
}
