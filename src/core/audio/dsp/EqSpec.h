/**
 * Location: src/core/audio/dsp/EqSpec.h
 * Function: Single source of truth for 12-band equalizer specification,
 * center frequencies, gain constraints, and biquad peaking filter engine.
 * All Rights Reserved isgaar
 */
#ifndef DOPAMINE_CORE_AUDIO_DSP_EQSPEC_H
#define DOPAMINE_CORE_AUDIO_DSP_EQSPEC_H

#include <array>
#include <cmath>
#include <vector>

namespace Dopamine::Core::Audio::Dsp {

constexpr int kEqBands = 12;
constexpr double kEqGainMinDb = -12.0;
constexpr double kEqGainMaxDb = 12.0;
constexpr double kEqStepDb = 0.5;
constexpr double kEqFlatDb = 0.0;

constexpr std::array<double, kEqBands> kEqCenterHz = {
    30.0, 50.0, 90.0, 160.0, 300.0, 500.0,
    1000.0, 1600.0, 3000.0, 5000.0, 9000.0, 16000.0
};

inline bool isFlatGain(double g) {
    return std::abs(g - kEqFlatDb) < 1e-4;
}

struct BiquadCoeffs {
    float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f;
    float a1 = 0.0f, a2 = 0.0f;
    bool active = false;

    void computePeaking(float f0, float gDb, float rate, float q = 1.414f) {
        if (std::abs(gDb) < 1e-4f || rate <= 0.0f) {
            active = false;
            return;
        }
        float w0 = 2.0f * 3.14159265358979323846f * (f0 / rate);
        float alpha = std::sin(w0) / (2.0f * q);
        float a = std::pow(10.0f, gDb / 40.0f);
        float cosW = std::cos(w0);

        float a0 = 1.0f + alpha / a;
        b0 = (1.0f + alpha * a) / a0;
        b1 = (-2.0f * cosW) / a0;
        b2 = (1.0f - alpha * a) / a0;
        a1 = (-2.0f * cosW) / a0;
        a2 = (1.0f - alpha / a) / a0;
        active = true;
    }

    float process(float in, float &s1, float &s2) const {
        if (!active) return in;
        float out = b0 * in + s1;
        s1 = b1 * in - a1 * out + s2;
        s2 = b2 * in - a2 * out;
        return out;
    }
};

class EqualizerFilter {
public:
    void setup(const std::vector<float>& gains, float preampDb, float rate) {
        m_isFlat = (std::abs(preampDb) < 1e-4f);
        m_preampLinear = std::pow(10.0f, preampDb / 20.0f);

        for (int i = 0; i < kEqBands; ++i) {
            float g = (i < static_cast<int>(gains.size())) ? gains[i] : 0.0f;
            if (std::abs(g) >= 1e-4f) m_isFlat = false;
            float freq = static_cast<float>(kEqCenterHz[i]);
            m_filters[i].computePeaking(freq, g, rate);
        }
    }

    bool isFlat() const { return m_isFlat; }

    void process(float* data, int frames, int channels) {
        if (m_isFlat || !data || frames <= 0) return;
        if (channels < 1 || channels > 2) return;

        for (int f = 0; f < frames; ++f) {
            for (int ch = 0; ch < channels; ++ch) {
                int idx = f * channels + ch;
                float sample = data[idx] * m_preampLinear;
                for (int b = 0; b < kEqBands; ++b) {
                    sample = m_filters[b].process(sample,
                                                 m_s1[b][ch],
                                                 m_s2[b][ch]);
                }
                data[idx] = sample;
            }
        }
    }

    void resetStates() {
        for (int b = 0; b < kEqBands; ++b) {
            for (int ch = 0; ch < 2; ++ch) {
                m_s1[b][ch] = 0.0f;
                m_s2[b][ch] = 0.0f;
            }
        }
    }

private:
    bool m_isFlat = true;
    float m_preampLinear = 1.0f;
    std::array<BiquadCoeffs, kEqBands> m_filters;
    float m_s1[kEqBands][2] = {{0}};
    float m_s2[kEqBands][2] = {{0}};
};

} // namespace Dopamine::Core::Audio::Dsp

#endif // DOPAMINE_CORE_AUDIO_DSP_EQSPEC_H
