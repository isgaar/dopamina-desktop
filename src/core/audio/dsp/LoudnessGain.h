/**
 * Location: src/core/audio/dsp/LoudnessGain.h
 * Function: Pure calculation of EBU R128 and BS.1770 loudness normalization
 * gain with true-peak protection and linear scaling factors.
 * All Rights Reserved isgaar
 */
#ifndef DOPAMINE_CORE_AUDIO_DSP_LOUDNESSGAIN_H
#define DOPAMINE_CORE_AUDIO_DSP_LOUDNESSGAIN_H

#include <optional>

namespace Dopamine::Core::Audio::Dsp {

struct NormalizationResult {
    float gainDb = 0.0f;
    float linearGain = 1.0f;
};

class LoudnessGain {
public:
    static NormalizationResult calculate(
        float targetLufs,
        std::optional<float> refLufs,
        std::optional<float> refPeakDbtp = std::nullopt,
        float peakCapDbtp = -1.0f);
};

} // namespace Dopamine::Core::Audio::Dsp

#endif // DOPAMINE_CORE_AUDIO_DSP_LOUDNESSGAIN_H
