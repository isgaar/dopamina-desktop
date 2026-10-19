/**
 * Location: src/core/audio/automix/CrossfadeMixer.cpp
 * Function: Mathematical implementation of equal-power audio mixing curves
 * utilizing quarter-sine and quarter-cosine interpolation across track shifts.
 * All Rights Reserved isgaar
 */
#include "CrossfadeMixer.h"
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

    for (int frame = 0; frame < frameCount; ++frame) {
        float t = progressStart;
        if (frameCount > 1) {
            float frac = static_cast<float>(frame) / (frameCount - 1);
            t = progressStart + frac * (progressEnd - progressStart);
        }

        float gOut = 1.0f;
        float gIn = 0.0f;
        computeGains(t, gOut, gIn);

        for (int ch = 0; ch < channels; ++ch) {
            int idx = frame * channels + ch;
            float sOut = outgoing ? outgoing[idx] : 0.0f;
            float sIn = incoming ? incoming[idx] : 0.0f;
            output[idx] = sOut * gOut + sIn * gIn;
        }
    }
}

} // namespace Dopamine::Core::Audio::Dsp
