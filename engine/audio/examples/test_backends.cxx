// test each possible backend in soundio

#include "LibSoundIo.hxx"
#include "Log.hxx"

int main() {


	// log
	vibe::ShLogPr lg = vibe::Log::create();

	// start the soundio
	lg->msg("create soundio\n");
	SoundIo* soundio = soundio_create();
	if(!soundio) {
		fprintf(stderr, "out of memory\n");
		return 1;
	}

	// backends
	SoundIoBackend backends[7];
	backends[0] = SoundIoBackendNone;
	backends[1] = SoundIoBackendAlsa;
	backends[2] = SoundIoBackendPulseAudio;
	backends[3] = SoundIoBackendJack;
	backends[4] = SoundIoBackendWasapi;
	backends[5] = SoundIoBackendDummy;
	backends[6] = SoundIoBackendCoreAudio;

	// setup backend
	for(int i = 0; i < 7; i++) {
		SoundIoBackend backend = backends[i];
		//fprintf(stderr, "Connecting to backend: %s\n", soundio_backend_name(backend));
		lg->msg("Testing backend: %s:", soundio_backend_name(backend));

		int err =
			(backend == SoundIoBackendNone) ? soundio_connect(soundio) : soundio_connect_backend(soundio, backend);

		if(err) {
			//	fprintf(stderr, "Unable to connect to backend: %s\n", soundio_strerror(err));
			lg->msg(" %sBAD%s\n", KRED, KNRM);
			//return 1;
		} else {
			lg->msg(" %sGOOD%s\n", KGRN, KNRM);
		}

		fprintf(stderr, "Backend: %s\n", soundio_backend_name(soundio->current_backend));
		soundio_disconnect(soundio);
	}
}
