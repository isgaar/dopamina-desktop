/**
 * Location: src/core/audio/dsp/EqSpec.h
 * Function: Single source of truth for 12-band equalizer specification,
 * center frequencies, gain constraints, and Rust 64-bit DSP FFI bridge.
 * All Rights Reserved isgaar
 */
#ifndef DOPAMINE_CORE_AUDIO_DSP_EQSPEC_H
#define DOPAMINE_CORE_AUDIO_DSP_EQSPEC_H

#include <array>
#include <cmath>
#include <vector>

extern "C" {
void dopamine_dsp_init(float sample_rate, int channels);
void dopamine_dsp_reset();
void dopamine_dsp_set_eq_gains(const float *gains_ptr, int count,
                               float preamp_db);
void dopamine_dsp_set_effects1(float bass, float mids, float treble,
                               float virtualizer, float compressor,
                               float balance);
void dopamine_dsp_set_effects2(bool echo_on, float echo_gain, bool autowah_on,
                               float autowah_gain, bool reverb_on,
                               float reverb_gain, bool flanger_on);
void dopamine_dsp_set_limiter_enabled(bool enabled);
void dopamine_dsp_set_stabilizer_enabled(bool enabled);
bool dopamine_dsp_is_flat();
void dopamine_dsp_trigger_crossfade(int fade_frames);
void dopamine_dsp_set_crossfade_out_target(float target_gain);
void dopamine_dsp_process_f32(float *samples_ptr, int frames, int channels);
void dopamine_dsp_mix_equal_power(const float *outgoing_ptr,
                                  const float *incoming_ptr,
                                  float *output_ptr, int frames, int channels,
                                  float progress_start, float progress_end);
}

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

class EqualizerFilter {
public:
    void setup(const std::vector<float> &gains, float preampDb, float rate) {
        m_isFlat = (std::abs(preampDb) < 1e-4f);
        std::vector<float> padded(kEqBands, 0.0f);
        for (int i = 0; i < kEqBands; ++i) {
            float g = (i < static_cast<int>(gains.size())) ? gains[i] : 0.0f;
            if (std::abs(g) >= 1e-4f) m_isFlat = false;
            padded[static_cast<size_t>(i)] = g;
        }
        dopamine_dsp_init(rate > 0.0f ? rate : 44100.0f, 2);
        dopamine_dsp_set_eq_gains(padded.data(), kEqBands, preampDb);
    }

    bool isFlat() const { return m_isFlat && dopamine_dsp_is_flat(); }

    void process(float *data, int frames, int channels) {
        if (!data || frames <= 0 || channels < 1 || channels > 2) return;
        dopamine_dsp_process_f32(data, frames, channels);
    }

    void resetStates() { dopamine_dsp_reset(); }

private:
    bool m_isFlat = true;
};

} // namespace Dopamine::Core::Audio::Dsp

#endif // DOPAMINE_CORE_AUDIO_DSP_EQSPEC_H

