// FFmpeg-based audio loader implementation

#include <iostream>
#include <algorithm>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
#include <libavutil/opt.h>
#include <libavutil/version.h>
#include <libavutil/channel_layout.h>
}

// FFmpeg 5.1+ uses new channel layout API
#if LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(57, 28, 100)
  #define FFMPEG_NEW_CHANNEL_API 1
#else
  #define FFMPEG_NEW_CHANNEL_API 0
#endif

#include "FFmpegAudioData.hxx"

namespace vibe {
namespace daw {

FFmpegAudioData::FFmpegAudioData() {}

FFmpegAudioData::~FFmpegAudioData() {}

ShFFmpegAudioDataPr FFmpegAudioData::create() {
    return std::make_shared<FFmpegAudioData>();
}

bool FFmpegAudioData::supports_format(const std::string& fpath) {
    // Get file extension
    size_t dot_pos = fpath.rfind('.');
    if (dot_pos == std::string::npos) return false;

    std::string ext = fpath.substr(dot_pos + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    // Common audio formats supported by FFmpeg
    return ext == "mp3" || ext == "flac" || ext == "ogg" || ext == "opus" ||
           ext == "m4a" || ext == "aac" || ext == "wma" || ext == "wav" ||
           ext == "aiff" || ext == "webm" || ext == "mp4";
}

bool FFmpegAudioData::load(const std::string& fpath) {
    std::cout << "loading (FFmpeg): " << fpath;

    AVFormatContext* format_ctx = nullptr;
    AVCodecContext* codec_ctx = nullptr;
    SwrContext* swr_ctx = nullptr;
    AVPacket* packet = nullptr;
    AVFrame* frame = nullptr;

    int ret = 0;

    // Open input file
    ret = avformat_open_input(&format_ctx, fpath.c_str(), nullptr, nullptr);
    if (ret < 0) {
        std::cerr << " ... FAILED to open file\n";
        return false;
    }

    // Find stream info
    ret = avformat_find_stream_info(format_ctx, nullptr);
    if (ret < 0) {
        std::cerr << " ... FAILED to find stream info\n";
        avformat_close_input(&format_ctx);
        return false;
    }

    // Find audio stream
    int audio_stream_idx = -1;
    for (unsigned int i = 0; i < format_ctx->nb_streams; i++) {
        if (format_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audio_stream_idx = i;
            break;
        }
    }

    if (audio_stream_idx < 0) {
        std::cerr << " ... FAILED: no audio stream found\n";
        avformat_close_input(&format_ctx);
        return false;
    }

    AVStream* audio_stream = format_ctx->streams[audio_stream_idx];
    AVCodecParameters* codecpar = audio_stream->codecpar;

    // Find decoder
    const AVCodec* codec = avcodec_find_decoder(codecpar->codec_id);
    if (!codec) {
        std::cerr << " ... FAILED: no decoder found\n";
        avformat_close_input(&format_ctx);
        return false;
    }

    // Allocate codec context
    codec_ctx = avcodec_alloc_context3(codec);
    if (!codec_ctx) {
        std::cerr << " ... FAILED: could not allocate codec context\n";
        avformat_close_input(&format_ctx);
        return false;
    }

    // Copy codec parameters
    ret = avcodec_parameters_to_context(codec_ctx, codecpar);
    if (ret < 0) {
        std::cerr << " ... FAILED: could not copy codec params\n";
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&format_ctx);
        return false;
    }

    // Open codec
    ret = avcodec_open2(codec_ctx, codec, nullptr);
    if (ret < 0) {
        std::cerr << " ... FAILED: could not open codec\n";
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&format_ctx);
        return false;
    }

    // Get audio properties
    _sample_rate = codec_ctx->sample_rate;
#if FFMPEG_NEW_CHANNEL_API
    _num_channels = codec_ctx->ch_layout.nb_channels;
#else
    if (codec_ctx->channel_layout == 0 && codec_ctx->channels > 0) {
        codec_ctx->channel_layout = av_get_default_channel_layout(codec_ctx->channels);
    }
    _num_channels = codec_ctx->channels;
#endif
    _bit_depth = av_get_bytes_per_sample(codec_ctx->sample_fmt) * 8;

    // Setup resampler to convert to float64 (double) planar format
#if FFMPEG_NEW_CHANNEL_API
    AVChannelLayout out_ch_layout;
    av_channel_layout_default(&out_ch_layout, _num_channels);
    ret = swr_alloc_set_opts2(&swr_ctx,
        &out_ch_layout,              // output channel layout
        AV_SAMPLE_FMT_DBLP,          // output format: double planar
        _sample_rate,                // output sample rate
        &codec_ctx->ch_layout,       // input channel layout
        codec_ctx->sample_fmt,       // input format
        codec_ctx->sample_rate,      // input sample rate
        0, nullptr);
#else
    int64_t in_ch_layout = codec_ctx->channel_layout;
    int64_t out_ch_layout = av_get_default_channel_layout(_num_channels);
    swr_ctx = swr_alloc_set_opts(nullptr,
        out_ch_layout,               // output channel layout
        AV_SAMPLE_FMT_DBLP,          // output format: double planar
        _sample_rate,                // output sample rate
        in_ch_layout,                // input channel layout
        codec_ctx->sample_fmt,       // input format
        codec_ctx->sample_rate,      // input sample rate
        0, nullptr);
    ret = swr_ctx ? 0 : -1;
#endif

    if (ret < 0 || !swr_ctx) {
        std::cerr << " ... FAILED: could not allocate resampler\n";
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&format_ctx);
        return false;
    }

    ret = swr_init(swr_ctx);
    if (ret < 0) {
        std::cerr << " ... FAILED: could not init resampler\n";
        swr_free(&swr_ctx);
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&format_ctx);
        return false;
    }

    // Allocate packet and frame
    packet = av_packet_alloc();
    frame = av_frame_alloc();
    if (!packet || !frame) {
        std::cerr << " ... FAILED: could not allocate packet/frame\n";
        if (packet) av_packet_free(&packet);
        if (frame) av_frame_free(&frame);
        swr_free(&swr_ctx);
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&format_ctx);
        return false;
    }

    // Estimate total samples and pre-allocate (will resize later if needed)
    int64_t duration_samples = 0;
    if (format_ctx->duration != AV_NOPTS_VALUE) {
        duration_samples = av_rescale_q(format_ctx->duration, AV_TIME_BASE_Q,
            {1, _sample_rate});
    } else {
        // Fallback: estimate from file size
        duration_samples = _sample_rate * 60 * 10; // 10 minutes max initial
    }

    // Collect samples per channel
    std::vector<std::vector<double>> channel_samples(_num_channels);
    for (int ch = 0; ch < _num_channels; ch++) {
        channel_samples[ch].reserve(duration_samples);
    }

    // Decode all packets
    while (av_read_frame(format_ctx, packet) >= 0) {
        if (packet->stream_index == audio_stream_idx) {
            ret = avcodec_send_packet(codec_ctx, packet);
            if (ret < 0) {
                av_packet_unref(packet);
                continue;
            }

            while (ret >= 0) {
                ret = avcodec_receive_frame(codec_ctx, frame);
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                    break;
                } else if (ret < 0) {
                    break;
                }

                // Allocate output buffer for resampled audio
                int out_samples = swr_get_out_samples(swr_ctx, frame->nb_samples);
                uint8_t** out_data = nullptr;
                int out_linesize;

                ret = av_samples_alloc_array_and_samples(&out_data, &out_linesize,
                    _num_channels, out_samples, AV_SAMPLE_FMT_DBLP, 0);
                if (ret < 0) {
                    continue;
                }

                // Convert samples
                int converted_samples = swr_convert(swr_ctx, out_data, out_samples,
                    (const uint8_t**)frame->data, frame->nb_samples);

                if (converted_samples > 0) {
                    // Copy converted samples to our vectors
                    for (int ch = 0; ch < _num_channels; ch++) {
                        double* samples_ptr = reinterpret_cast<double*>(out_data[ch]);
                        channel_samples[ch].insert(channel_samples[ch].end(),
                            samples_ptr, samples_ptr + converted_samples);
                    }
                }

                if (out_data) {
                    av_freep(&out_data[0]);
                    av_freep(&out_data);
                }
            }
        }
        av_packet_unref(packet);
    }

    // Flush the decoder
    avcodec_send_packet(codec_ctx, nullptr);
    while (avcodec_receive_frame(codec_ctx, frame) >= 0) {
        int out_samples = swr_get_out_samples(swr_ctx, frame->nb_samples);
        uint8_t** out_data = nullptr;
        int out_linesize;

        int ret = av_samples_alloc_array_and_samples(&out_data, &out_linesize,
            _num_channels, out_samples, AV_SAMPLE_FMT_DBLP, 0);
        if (ret >= 0) {
            int converted_samples = swr_convert(swr_ctx, out_data, out_samples,
                (const uint8_t**)frame->data, frame->nb_samples);

            if (converted_samples > 0) {
                for (int ch = 0; ch < _num_channels; ch++) {
                    double* samples_ptr = reinterpret_cast<double*>(out_data[ch]);
                    channel_samples[ch].insert(channel_samples[ch].end(),
                        samples_ptr, samples_ptr + converted_samples);
                }
            }

            if (out_data) {
                av_freep(&out_data[0]);
                av_freep(&out_data);
            }
        }
    }

    // Flush resampler
    int flush_samples = swr_get_out_samples(swr_ctx, 0);
    if (flush_samples > 0) {
        uint8_t** out_data = nullptr;
        int out_linesize;
        int ret = av_samples_alloc_array_and_samples(&out_data, &out_linesize,
            _num_channels, flush_samples, AV_SAMPLE_FMT_DBLP, 0);
        if (ret >= 0) {
            int converted = swr_convert(swr_ctx, out_data, flush_samples, nullptr, 0);
            if (converted > 0) {
                for (int ch = 0; ch < _num_channels; ch++) {
                    double* samples_ptr = reinterpret_cast<double*>(out_data[ch]);
                    channel_samples[ch].insert(channel_samples[ch].end(),
                        samples_ptr, samples_ptr + converted);
                }
            }
            if (out_data) {
                av_freep(&out_data[0]);
                av_freep(&out_data);
            }
        }
    }

    // Cleanup FFmpeg resources
    av_frame_free(&frame);
    av_packet_free(&packet);
    swr_free(&swr_ctx);
#if FFMPEG_NEW_CHANNEL_API
    av_channel_layout_uninit(&out_ch_layout);
#endif
    avcodec_free_context(&codec_ctx);
    avformat_close_input(&format_ctx);

    // Convert to armadillo matrix
    _num_samples = channel_samples[0].size();
    if (_num_samples == 0) {
        std::cerr << " ... FAILED: no samples decoded\n";
        return false;
    }

    _length_in_seconds = static_cast<double>(_num_samples) / _sample_rate;

    // Build sample matrix (num_channels x num_samples)
    _smat.set_size(_num_channels, _num_samples);
    for (int ch = 0; ch < _num_channels; ch++) {
        for (int i = 0; i < _num_samples; i++) {
            _smat(ch, i) = channel_samples[ch][i];
        }
    }

    // Build time vector
    _tvec = arma::linspace<arma::Row<double>>(0, _length_in_seconds, _num_samples);

    std::cout << " ... LOADED!\n";
    std::cout << "  Sample Rate: " << _sample_rate << " Hz\n";
    std::cout << "  Channels: " << _num_channels << "\n";
    std::cout << "  Samples: " << _num_samples << "\n";
    std::cout << "  Duration: " << _length_in_seconds << " seconds\n";

    return true;
}

} // namespace daw
} // namespace vibe
