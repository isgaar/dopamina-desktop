/**
 * Location: src/core/audio/PcmConversion.h
 * Function: Converts interleaved PCM samples between Qt audio sample formats
 * and the float32 representation used by the DSP chain.
 * All Rights Reserved isgaar
 */
#ifndef DOPAMINE_CORE_AUDIO_PCMCONVERSION_H
#define DOPAMINE_CORE_AUDIO_PCMCONVERSION_H

#include <QAudioFormat>
#include <algorithm>
#include <cmath>
#include <QtGlobal>

namespace Dopamine::Core::Audio::Pcm {

inline float decodeSample(const char *data,
                          QAudioFormat::SampleFormat format, int index) {
    switch (format) {
    case QAudioFormat::UInt8:
        return (reinterpret_cast<const quint8 *>(data)[index] - 128) / 128.0f;
    case QAudioFormat::Int16:
        return reinterpret_cast<const qint16 *>(data)[index] / 32768.0f;
    case QAudioFormat::Int32:
        return reinterpret_cast<const qint32 *>(data)[index] / 2147483648.0f;
    case QAudioFormat::Float:
        return reinterpret_cast<const float *>(data)[index];
    default:
        return 0.0f;
    }
}

inline void encodeSample(char *data, QAudioFormat::SampleFormat format,
                         int index, float sample) {
    const float clamped = std::clamp(sample, -1.0f, 1.0f);
    switch (format) {
    case QAudioFormat::UInt8:
        static_cast<quint8 *>(static_cast<void *>(data))[index] =
            static_cast<quint8>(std::lround((clamped * 0.5f + 0.5f) * 255.0f));
        break;
    case QAudioFormat::Int16:
        static_cast<qint16 *>(static_cast<void *>(data))[index] =
            static_cast<qint16>(std::clamp(
                std::lround(clamped * 32768.0f), -32768L, 32767L));
        break;
    case QAudioFormat::Int32:
        static_cast<qint32 *>(static_cast<void *>(data))[index] =
            static_cast<qint32>(std::clamp(
                std::llround(static_cast<double>(clamped) * 2147483648.0),
                -2147483648LL, 2147483647LL));
        break;
    case QAudioFormat::Float:
        static_cast<float *>(static_cast<void *>(data))[index] = clamped;
        break;
    default:
        break;
    }
}

} // namespace Dopamine::Core::Audio::Pcm

#endif // DOPAMINE_CORE_AUDIO_PCMCONVERSION_H
