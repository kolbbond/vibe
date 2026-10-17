// FFmpeg-based audio loader for various formats (MP3, FLAC, OGG, etc.)
#pragma once

#include <memory>
#include <string>
#include <armadillo>

#include "AudioData.hxx"

namespace vibe {
namespace daw {

typedef std::shared_ptr<class FFmpegAudioData> ShFFmpegAudioDataPr;

class FFmpegAudioData : public AudioData {

protected:
public:
    // constructor
    FFmpegAudioData();

    // destructor
    ~FFmpegAudioData();

    // Load audio file using FFmpeg (supports MP3, FLAC, OGG, WAV, and many more)
    bool load(const std::string& fpath);

    // factory
    static ShFFmpegAudioDataPr create();

    // Check if FFmpeg supports a given file extension
    static bool supports_format(const std::string& fpath);
};

} // namespace daw
} // namespace vibe
