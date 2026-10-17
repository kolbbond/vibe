// storing the audiobuffer to play

#include "AudioBuffer.hxx"

namespace vibe { namespace daw {

AudioBuffer::AudioBuffer() {
	// initialize buffer
	_buffer_size = _sample_rate * 2; // 2 s buffer?
	_buffer.set_size(_num_channels, _buffer_size);
	_buffer_idx = 0;
}
AudioBuffer::~AudioBuffer() {}

// factory
ShAudioBufferPr AudioBuffer::create() {
	return std::make_shared<AudioBuffer>();
}

arma::Mat<fltp> AudioBuffer::get_buffer() {
	//
	return _buffer;
}

// add samples to audio buffer
void AudioBuffer::add2buffer(const arma::Row<fltp>& samples, const arma::uword cidx) {
	// check inputs

	assert(cidx <= _num_channels);
	assert(samples.n_elem <= _buffer_size);

	arma::uword num_ns = samples.n_elem; // num new samples
	arma::uword idxs, idxe; // start/end index

	// check idx of samples against last buffer idx
	// and move buffer samples to beginning
	if(_buffer_idx + num_ns > _buffer_size) {
		_buffer.cols(0, _buffer_size - _buffer_idx) = _buffer.cols(_buffer_idx, _buffer_size - 1);
		_buffer.cols(_buffer_size - _buffer_idx + 1, _buffer_size - 1).fill(0);
		_buffer_idx = 0;
	} else {
		//
	}

	//  add new samples to buffer
	idxs = _buffer_idx;
	idxe = idxs + num_ns;
	_buffer.submat(cidx, idxs, cidx, idxe) = 
        _buffer.submat(cidx,idxs,cidx,idxe) + samples;

}

void AudioBuffer::add2buffer(const arma::Row<fltp>& samples) {
	// fill all channels
	for(arma::uword i = 0; i < _num_channels; i++) {
		add2buffer(samples, i);
	}
}


// static
}} // namespace vibe::daw
