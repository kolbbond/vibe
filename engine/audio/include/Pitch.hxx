
// Pitch.hh

#pragma once

#include <memory>
#include "vibe/common/common.hxx"
namespace vibe { namespace daw {

typedef std::shared_ptr<class Pitch> ShPitchPr;

class Pitch {

	// properties
protected:
	// methods
public:
	// constructor
	Pitch();

	// destructor
	~Pitch();


	// factory
	static ShPitchPr create();

	static double pitch2freq(int);
	static double midi2freq(int);
	static std::string midi2note(int);
	static int midi2pitch(int);
	static int pitch2midi(int);
	static int str2midi(std::string);
	static int str2pitch(std::string);
	static double str2freq(std::string);

	static bool check_note(char);
};

}} // namespace vibe::daw
