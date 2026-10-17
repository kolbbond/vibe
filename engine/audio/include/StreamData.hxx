#pragma once

#include <armadillo>
#include <memory>

#include "AudioData.hxx"
#include "AudioFile.hxx"
#include <soundio/soundio.h>

namespace vibe { namespace daw {

typedef std::shared_ptr<class StreamData> ShStreamDataPr;

class StreamData: public AudioData {

protected:
public:
	// methods
	// constructor
	StreamData();

	// destructor
	~StreamData();

	//bool load(SoundIo* soundio);

	// factory
	static ShStreamDataPr create();
};

}} // namespace vibe::daw
