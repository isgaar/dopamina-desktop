/**
 * Location: src/core/audio/automix/CrossfadeMixer.h
 * Function: Equal-power stereo audio crossfade mixer blending outgoing and
 * incoming track streams with mathematically constant acoustic power sum.
 * All Rights Reserved isgaar
 */
#ifndef DOPAMINE_CORE_AUDIO_AUTOMIX_CROSSFADEMIXER_H
#define DOPAMINE_CORE_AUDIO_AUTOMIX_CROSSFADEMIXER_H

#include <vector>

namespace Dopamine::Core::Audio::Dsp {

class CrossfadeMixer {
public:
    CrossfadeMixer();

    static void computeGains(float progress, float &gainOut, float &gainIn);

    void mix(const float *outgoing,
             const float *incoming,
             float *output,
             int frameCount,
             int channels,
             float progressStart,
             float progressEnd);
};

} // namespace Dopamine::Core::Audio::Dsp

#endif // DOPAMINE_CORE_AUDIO_AUTOMIX_CROSSFADEMIXER_H
