/**
 * Location: src/core/audio/dsp/GainStage.cpp
 * Function: Implementation of audio gain scaling with transparent scalar math
 * and optional peak limiter tracking for safe dynamic range rendering.
 * All Rights Reserved isgaar
 */
#include "GainStage.h"
#include <cmath>
#include <algorithm>

namespace Dopamine::Core::Audio::Dsp {

GainStage::GainStage() = default;

void GainStage::setGain(float linearGain) {
    m_gain = std::max(0.0f, linearGain);
}

void GainStage::process(float *samples, int frameCount, int channels) {
    if (!samples || frameCount <= 0 || channels <= 0) return;

    int totalSamples = frameCount * channels;
    if (m_gain != 1.0f) {
        for (int i = 0; i < totalSamples; ++i) {
            samples[i] *= m_gain;
        }
    }

    if (!m_limiterEnabled) return;

    const float ceiling = 0.98f;
    const float releaseCoeff = 0.999f;
    for (int i = 0; i < totalSamples; ++i) {
        float absVal = std::abs(samples[i]);
        if (absVal > m_envelope) {
            m_envelope = absVal;
        } else {
            float decay = 1.0f - releaseCoeff;
            m_envelope = m_envelope * releaseCoeff + absVal * decay;
        }

        if (m_envelope > ceiling) {
            samples[i] = (samples[i] / m_envelope) * ceiling;
        }
    }
}

} // namespace Dopamine::Core::Audio::Dsp
