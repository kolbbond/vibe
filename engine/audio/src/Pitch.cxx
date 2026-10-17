
// Pitch.cpp
// ###### -

// include header
#include "Pitch.hxx"
#include <cmath>
#include <string>

namespace vibe { namespace daw{
// constructor
Pitch::Pitch() {}

// destructor
Pitch::~Pitch() {}

// setters

// getters

// static
// https://en.wikipedia.org/wiki/Scientific_pitch_notation
int Pitch::pitch2midi(int n) {
	// pitch semitone above middle c to midi
	return n + 60;
}

int Pitch::midi2pitch(int m) {
	// midi number to pitch semitone above middle c
	return m - 60;
}

double Pitch::midi2freq(int m) {
	// midi number to pitch frequency
	// assume standard tuning A = 440 [Hz]

	return 440.0 * std::pow(2, (m - 69.0) / 12.0);
}

double Pitch::pitch2freq(int n) {
	// pitch to frequency
	// n is number of semitones above middle C
	// assume A = 440 [Hz]

	return 440.0 * std::pow(2, (n - 9.0) / 12.0);
}

int Pitch::str2pitch(std::string str) {
	// convert a string input to pitch number

	// parse the string input
	char note; // Note name (A->G) lower and uppercase
	//char mod; // modifier (if any) #, or b (for flat)
	//char octave[2]; // Octave (0->10)

	// check length of input str
	assert(str.size() > 2 && str.size() <= 4);

	// get note
	note = str[0];

	// check note

	if(Pitch::check_note(note)) {
		printf("note good\n");
	} else {
		printf("note bad\n");
	}

	// check 2nd char
	if(str[1] >= '0' && str[1] <= '9') {
		printf("0 through 9\n");
	} else if(str[1] == '#' || str[1] == 'b') {
		printf("sharp or flat\n");
	}

	// hardcoded return
	return 12;
}

double Pitch::str2freq(std::string str) {
	// string direct to freq
	return Pitch::pitch2freq(Pitch::str2pitch(str));
}

int Pitch::str2midi(std::string str) {
	// convert a string input to midi

	int pitch = Pitch::str2pitch(str);
	return Pitch::pitch2midi(pitch);
}

bool Pitch::check_note(char note) {
	// convert note is valid alphabetically
    (void) note;

	return true;
}

std::string Pitch::midi2note(int m) {
	// convert a midi note int to
	// string
	// add flats too?

    (void) m;


//	int offset = m % 12;
//	int octave = round(m / 12);

	// get char by offset

	// append octave number
    return "success";
}

// factory
ShPitchPr Pitch::create() {
	// factory
	return std::make_shared<Pitch>();
}
}} // namespace vibe
