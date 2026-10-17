#pragma once

#include <array>

#include "vibe/fluid/Emitters.hxx"

namespace vibe::fluid {

// raw band magnitudes -> normalized 0..1 levels and onsets for an AudioFrame.
// Each band is normalized by its own decaying running peak, so quiet and loud tracks both
// drive the emitters. The peak never drops below a floor: the absolute noiseFloor, or
// relativeFloor times a slow long-term reference, so quiet input right after loud music
// (a fade-out, room hiss) is not normalized back up to full strength.
class BandTracker {
public:
    float peakDecay = 0.5f;       // running peak multiplier per second
    float noiseFloor = 1.0e-3f;   // absolute floor (about -60 dBFS in FFT magnitude units)
    float relativeFloor = 0.1f;   // floor relative to the long-term reference (-20 dB)
    float longTermDecay = 0.98f;  // long-term reference multiplier per second (~35 s half-life)
    float smoothing = 0.6f;       // level smoothing per 1/60 s (0 = none)
    float onsetThreshold = 0.6f;  // normalized level that counts as a hit
    float rearmRatio = 0.7f;      // fall below threshold * rearmRatio before the next hit
    float refractory = 0.1f;      // seconds between hits

    void update(const std::array<float, kBands>& raw, double time, float dt, AudioFrame& frame);
    void reset();

private:
    std::array<float, kBands> _peak{};
    std::array<float, kBands> _longTerm{};
    std::array<float, kBands> _level{};
    std::array<bool, kBands> _armed{true, true, true, true, true};
    std::array<double, kBands> _lastHit{-1.0e9, -1.0e9, -1.0e9, -1.0e9, -1.0e9};
};

} // namespace vibe::fluid
