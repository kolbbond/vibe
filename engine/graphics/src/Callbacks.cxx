// Callbacks
// primarily for imgui

#include <thread>
#include "vibe/audio/FFT.hxx"
#include "Plotter.hxx"
#include "Callbacks.hxx"

namespace vibe { namespace gfx {

// constructor
Callbacks::Callbacks() {}

// destructor
Callbacks::~Callbacks() {}

// factory
ShCallbacksPr Callbacks::create() { return std::make_shared<Callbacks>(); }

std::string Callbacks::window_filedialog(bool& flag) {
	(void)flag;
	/*
	// open dialog window
	IGFD::FileDialogConfig config;
	config.path = "assets/";
	ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgKey", "Choose File", ".wav,.midi", config);

	std::string fpathname;
	std::string fpath;
	// display
	if(ImGuiFileDialog::Instance()->Display("ChooseFileDlgKey")) {
		if(ImGuiFileDialog::Instance()->IsOk()) {
			fpathname = ImGuiFileDialog::Instance()->GetFilePathName();
			fpath = ImGuiFileDialog::Instance()->GetCurrentPath();
			std::cout << "file path: " << fpathname << std::endl;
			std::cout << "current path: " << fpath << std::endl;
		}

		// close
		ImGuiFileDialog::Instance()->Close();
		flag = false;
	}

	//
	return fpathname;
    */
	return "";
}

int Callbacks::callback_main(void* data) {

	// get data pointer
	GuiMain* gui = reinterpret_cast<GuiMain*>(data);

	(void)gui;

	// call other callbacks
	callback_audiofile(gui->_nm.get());
	callback_drawobject(data);


	// success?
	return 0;
}

int Callbacks::callback_drawobject(void* data) {

	GuiMain* gui = reinterpret_cast<GuiMain*>(data);
	(void)gui;

	// clear buffer
	GL::defaultFramebuffer.clear(GL::FramebufferClear::Color | GL::FramebufferClear::Depth);

	ImGui::Text("draw object\n");
	ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0 / Double(ImGui::GetIO().Framerate), Double(ImGui::GetIO().Framerate));

	// draw meshes
	//gui->_mesher->
	//Magnum::GL::Mesh mesh = gui->_mesher->;
	// walk over meshes?
	/*
	Containers::Optional<GL::Mesh> meshlist;
	for(size_t i = 0; i < gui->_meshes.size(); i++) {
		Containers::Optional<GL::Mesh>& mesh = gui->_meshes[i];
		//meshlist = gui->_meshes[i];
		//Magnum::GL::Mesh& mesh;

		gui->_mesher->_shader.draw(*mesh);
	}
    */


	// draw other meshes
	//printf("draw meshes\n");
	/*
	for(size_t i = 0; i < gui->_meshes2.size(); i++) {
		gui->_mesher->_shader.draw(gui->_meshes2[i]);
	}
	//printf("set matrix\n");
	gui->_mesher->_shader.setTransformationProjectionMatrix(gui->_mesher->_transformation * gui->_mesher->_projection);
	//printf("done\n");
    */

	// try drawable
	//printf("draw with camera\n");
	gui->_camera->draw(gui->_drawables);

	// success?
	return 0;
}

int Callbacks::callback_rad(void* data) {
	// get gui
	GuiMain* gui = reinterpret_cast<GuiMain*>(data);

	// wrap to gui
	gui->show_objects();

	// success
	return 0;
}

int Callbacks::callback_audiofile(void* data) {
	//
	// out data
    daw::NoiseMaker* nm = reinterpret_cast<daw::NoiseMaker*>(data);
	assert(nm != NULL);
	//nm->generate();

	// audiofile object
	static AudioFile<double> audio;

	static bool flag_loaded;
	static bool use_work_area = true;
	static bool p_open = true;

	// start the ImGui window
	static ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings;

	// We demonstrate using the full viewport area or the work area (without menu-bars, task-bars etc.)
	// Based on your use case you may want one or the other.
	const ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(use_work_area ? viewport->WorkPos : viewport->Pos);
	ImGui::SetNextWindowSize(use_work_area ? viewport->WorkSize : viewport->Size);

	if(ImGui::Begin("Example: Fullscreen window", &p_open, flags)) {

		// check for loading wav file
		// test sound file from assets
		static std::string fpath = "assets/test_midi.wav";

		static int bit_depth, num_samples, num_channels;
		static int sample_rate;
		static double length_in_seconds;
		//static bool ismono, isstereo;

		// make vectors for ImPlotting
		static arma::Col<double> xp;
		static arma::Mat<double> yp;

		// audio objects
		static ShAudioDataPr ad = AudioData::create();
		static daw::ShFFTPr fft = daw::FFT::create();

		// file browser
		static bool flag_browse;
		if(ImGui::Button("browse")) { flag_browse = true; }

		if(flag_browse == true) { fpath = window_filedialog(flag_browse); }

		// button to load the audio file
		if(ImGui::Button("load") || !flag_loaded) {

			// load
			std::cout << "loading: " << fpath << std::endl;
			audio.load(fpath);

			// get info
			sample_rate = audio.getSampleRate();
			bit_depth = audio.getBitDepth();

			num_samples = audio.getNumSamplesPerChannel();
			length_in_seconds = audio.getLengthInSeconds();

			num_channels = audio.getNumChannels();

			// summary helper
			audio.printSummary();

			// get the samples
			// samples[channel][sampleIndex]
			AudioFile<double>::AudioBuffer samples;
			samples = audio.samples;

			// allocate
			xp = arma::Col<double>(num_samples, arma::fill::zeros);
			yp = arma::Mat<double>(num_samples, num_channels, arma::fill::zeros);

			std::cout << "fill sample vectors\n";

			// walk the samples
			//printf("i,j...: ");
			for(int i = 0; i < num_samples; i++) {

				xp(i) = i;

				// walk over channels
				for(int j = 0; j < num_channels; j++) { yp(i, j) = samples[j][i]; }
			}

			printf("xp(%llu,%llu)\n", xp.n_rows, xp.n_cols);
			printf("yp(%llu,%llu)\n", yp.n_rows, yp.n_cols);

			flag_loaded = true;

			// set audio into soundio
			//nm->generate();


			// create the audiodata
			ad->set_data(xp.t(), yp.t(), sample_rate);

			// get fft of the selection
			fft->set_audiodata(ad);

			assert(nm != NULL);
			nm->set_audio(audio);
			nm->set_callback(daw::NoiseMaker::wav_callback);

		} // load

		ImGui::SameLine();

		// play audio
		static bool flag_play;
		static std::string play_button = "play";
		if(ImGui::Button(play_button.c_str())) {
			flag_play ^= true;

			// change text
			if(play_button == "play")
				play_button = "pause";
			else
				play_button = "play";
		}

		if(flag_loaded) {

			// summary display
			ImGui::Text("file: %s\n", fpath.c_str());
			ImGui::Text("num_samples: %i\n", num_samples);
			ImGui::Text("bit_depth: %i\n", bit_depth);
			ImGui::Text("length_in_seconds: %0.3f\n", length_in_seconds);
			ImGui::Text("num_channels: %i\n", num_channels);
			ImGui::Text("sample_rate: %i\n", sample_rate);

			// plot spectrogram? (audiospectra)
			assert(yp.n_rows == static_cast<arma::uword>(num_samples));
			assert(yp.n_cols == static_cast<arma::uword>(num_channels));
			assert(xp.n_rows == static_cast<arma::uword>(yp.n_rows));

			static bool flag_plot = true;

			// allow option to plot the audio spectra
			if(ImGui::Button("toggle plot")) flag_plot ^= true;

			// slider to select plot
			static int sample_idx = 0;
			static int sample_win = 1000;
			ImGui::SliderInt("sample idx", &sample_idx, 0, xp.n_elem);
			ImGui::SliderInt("sample window", &sample_win, 0, xp.n_elem);

			// plot spectra
			if(flag_plot) {

				// data bounds
				// @hey: add downsampling
				arma::uword num_down = 1; // [Hz]
				int minx = sample_idx;
				int maxx = sample_idx + sample_win;
				if(maxx > static_cast<int>(xp.n_elem)) maxx = static_cast<int>(xp.n_elem);

				// fft this selection
				arma::Row<fltp> freqs;
				arma::Mat<fltp> mags;
				fft->calc_frequencies(ad->get_sample_matrix(), ad->get_sample_rate(), minx, maxx, freqs, mags);

				// create downsampled indices
				arma::Col<arma::uword> didxs = arma::regspace<arma::Col<arma::uword>>(minx, num_down, maxx);

				// spectra plot
				if(ImPlot::BeginPlot("audio spectra", ImVec2(400, 200), ImPlotFlags_None)) {
					ImPlot::SetupAxes("sample index", "amplitude", ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);

					// walk each channel
					for(int j = 0; j < num_channels; j++) {
						ImPlot::SetNextLineStyle(ImPlot::SampleColormap(static_cast<fltp>(j) / (num_channels - 1), ImPlotColormap_Jet));
						arma::Col<fltp> ypuse = yp.col(j);
						Plotter::plot_line(xp.rows(didxs), ypuse(didxs));
					}

					ImPlot::EndPlot();
				}

				ImGui::SameLine();

				// fft plot
				if(ImPlot::BeginPlot("frequency", ImVec2(400, 200), ImPlotFlags_None)) {
					ImPlot::SetupAxes("sample index", "amplitude", ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);

					// walk each channel
					for(int j = 0; j < num_channels; j++) {
						ImPlot::SetNextLineStyle(ImPlot::SampleColormap(static_cast<fltp>(j) / (num_channels - 1), ImPlotColormap_Jet));

						// make sure fft worked
						if(!freqs.is_empty()) { Plotter::plot_line(freqs.t(), arma::trans(mags.row(j))); }
					}

					ImPlot::EndPlot();
				}
			} // endif flag_plot


			// can we thread this?
			static const int num_cpus = std::thread::hardware_concurrency();
			ImGui::Text("number of CPU threads available: %i\n", num_cpus);
			static std::thread thread_audio;
			static std::atomic<bool> thread_done(false);
			static std::atomic<bool> thread_fired(false);
			static bool parallel_audio = false;

			// check if thread is running
			if(parallel_audio) {

				if(!thread_fired) {

					printf("firing thread\n");
					thread_audio = std::thread([&]() {
						// try firing on a thread
						thread_fired = true;
						thread_done = false;

						// catch error?
						try {

							printf("play wav here\n");
							nm->play_wav();
							printf("FIRED\n");


						} catch(const std::runtime_error& e()) {

							printf("last last audio errored!\n");
							// catch all exceptions @hey, bad design

						} catch(...) { printf("OTHER ERROR OTHER ERROR OTHER ERROR OTHER ERRORSSS\n"); }

						thread_done = true;
					});
				}
			} else {

				nm->play_wav();

				// play or pause
				if(flag_play) {
					nm->play();
				} else {
					nm->pause();
				}
			}

			// check thread
			//printf("check if done\n");
			if(thread_done) {

				printf("join thread\n");

				// join
				if(thread_audio.joinable()) thread_audio.join();

				// set flags
				thread_done = false;
				thread_fired = false;

				// play or pause
				if(flag_play) {
					nm->play();
				} else {
					nm->pause();
				}
			}
		}
	} // imgui window
	ImGui::End();


	// success
	return 0;
}

// setters

// getters

// static
}} // namespace vibe::gfx
