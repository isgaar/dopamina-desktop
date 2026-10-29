/**
 * Location: src/core/audio/automix/CrossfadeMixer.cpp
 * Function: Mathematical implementation of equal-power audio mixing curves
 * utilizing quarter-sine and quarter-cosine interpolation across track shifts.
 * All Rights Reserved isgaar
 */
#include "CrossfadeMixer.h"
#include "../dsp/EqSpec.h"
#include <cmath>
#include <algorithm>

namespace Dopamine::Core::Audio::Dsp {

CrossfadeMixer::CrossfadeMixer() = default;

void CrossfadeMixer::computeGains(float progress,
                                  float &gainOut,
                                  float &gainIn) {
    float clamped = std::clamp(progress, 0.0f, 1.0f);
    constexpr float halfPi = 1.5707963267948966f;
    gainOut = std::cos(clamped * halfPi);
    gainIn = std::sin(clamped * halfPi);
}

void CrossfadeMixer::mix(const float *outgoing,
                         const float *incoming,
                         float *output,
                         int frameCount,
                         int channels,
                         float progressStart,
                         float progressEnd) {
    if (!output || frameCount <= 0 || channels <= 0) return;
    dopamine_dsp_mix_equal_power(outgoing, incoming, output, frameCount,
                                 channels, progressStart, progressEnd);
}

} // namespace Dopamine::Core::Audio::Dsp
