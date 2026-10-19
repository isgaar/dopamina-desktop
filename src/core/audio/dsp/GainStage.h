/**
 * Location: src/core/audio/dsp/GainStage.h
 * Function: Audio digital signal processing gain stage applying float32 scalar
 * amplification, soft-knee true-peak limiting, and optional dithering.
 * All Rights Reserved isgaar
 */
#ifndef DOPAMINE_CORE_AUDIO_DSP_GAINSTAGE_H
#define DOPAMINE_CORE_AUDIO_DSP_GAINSTAGE_H

#include <vector>

namespace Dopamine::Core::Audio::Dsp {

class GainStage {
public:
    GainStage();
    void setGain(float linearGain);
    float gain() const { return m_gain; }
    void setLimiterEnabled(bool enabled) { m_limiterEnabled = enabled; }
    bool isLimiterEnabled() const { return m_limiterEnabled; }

    void process(float *samples, int frameCount, int channels);

private:
    float m_gain = 1.0f;
    bool m_limiterEnabled = false;
    float m_envelope = 0.0f;
};

} // namespace Dopamine::Core::Audio::Dsp

#endif // DOPAMINE_CORE_AUDIO_DSP_GAINSTAGE_H
