#pragma once

#include <armadillo>
#include <memory>

#include "AudioData.hxx"
#include "AudioFile.hxx"

namespace vibe {
namespace daw {

typedef std::shared_ptr<class WavData> ShWavDataPr;

class WavData : public AudioData {

protected:
public:
  // methods
  // constructor
  WavData();

  // destructor
  ~WavData();

  //
  bool load(std::string fpath);
  // void pull_samples();

  // factory
  static ShWavDataPr create();

  // setters

  // getters
  //	AudioFile<double> get_audio();
  //	arma::Row<double> get_time_vector() override;
  //	arma::Mat<double> get_sample_matrix() override;

  // static
};

} // namespace daw
} // namespace vibe
