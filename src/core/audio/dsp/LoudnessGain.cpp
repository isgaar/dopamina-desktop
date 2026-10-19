/**
 * Location: src/core/audio/dsp/LoudnessGain.cpp
 * Function: Implementation of EBU R128 / BS.1770 target matching gain logic
 * computing raw offset, true peak ceiling clamping, and linear multipliers.
 * All Rights Reserved isgaar
 */
#include "LoudnessGain.h"
#include <cmath>
#include <algorithm>

namespace Dopamine::Core::Audio::Dsp {

NormalizationResult LoudnessGain::calculate(
    float targetLufs,
    std::optional<float> refLufs,
    std::optional<float> refPeakDbtp,
    float peakCapDbtp) {
    if (!refLufs.has_value()) {
        return {0.0f, 1.0f};
    }

    float gRaw = targetLufs - refLufs.value();
    float g = gRaw;
    if (refPeakDbtp.has_value()) {
        float gMax = peakCapDbtp - refPeakDbtp.value();
        g = std::min(gRaw, gMax);
    }

    float linear = std::pow(10.0f, g / 20.0f);
    return {g, linear};
}

} // namespace Dopamine::Core::Audio::Dsp
