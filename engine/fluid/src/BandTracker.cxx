#include "vibe/fluid/BandTracker.hxx"

#include <algorithm>
#include <cmath>

namespace vibe::fluid {

void BandTracker::update(const std::array<float, kBands>& raw, double time, float dt, AudioFrame& frame) {
    const float decay = std::pow(peakDecay, dt);
    const float longDecay = std::pow(longTermDecay, dt);
    const float keep = std::pow(smoothing, dt * 60.0f);
    for(std::size_t b = 0; b < static_cast<std::size_t>(kBands); ++b) {
        const float value = std::max(0.0f, raw[b]);
        _longTerm[b] = std::max(value, _longTerm[b] * longDecay);
        const float floor = std::max(noiseFloor, _longTerm[b] * relativeFloor);
        _peak[b] = std::max({value, _peak[b] * decay, floor});
        const float norm = std::min(1.0f, value / _peak[b]);
        _level[b] = keep * _level[b] + (1.0f - keep) * norm;
        frame.bands[b] = _level[b];
        frame.onsets[b] = false;
        if(_armed[b] && norm >= onsetThreshold && time - _lastHit[b] >= refractory) {
            frame.onsets[b] = true;
            _armed[b] = false;
            _lastHit[b] = time;
        } else if(!_armed[b] && norm < onsetThreshold * rearmRatio) {
            _armed[b] = true;
        }
    }
}

void BandTracker::reset() {
    _peak.fill(0.0f);
    _longTerm.fill(0.0f);
    _level.fill(0.0f);
    _armed.fill(true);
    _lastHit.fill(-1.0e9);
}

} // namespace vibe::fluid
