#pragma once

#include <algorithm>

namespace vibe::game {

// fixed-timestep accumulator: turns variable frame times into whole ticks
class FixedStepper {
public:
    static constexpr double kMaxFrameSeconds = 0.25; // anti spiral-of-death

    explicit FixedStepper(double tickHz) : _tickDt{1.0 / tickHz} {}

    // returns how many ticks to run this frame
    int advance(double frameSeconds, double speed) {
        _accumulator += std::min(frameSeconds * speed, kMaxFrameSeconds);
        int ticks = 0;
        while(_accumulator >= _tickDt) {
            _accumulator -= _tickDt;
            ++ticks;
        }
        return ticks;
    }

    // interpolation factor between the previous and current tick, [0, 1)
    double alpha() const { return _accumulator / _tickDt; }

    void reset() { _accumulator = 0.0; }

private:
    double _tickDt;
    double _accumulator = 0.0;
};

} // namespace vibe::game
