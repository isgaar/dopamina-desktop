/**
 * Location: src/core/audio/automix/TransitionPlanner.h
 * Function: Pure transition planner determining beatmatch, harmonic, or soft
 * crossfade parameters with bar alignment and integer frame calculations.
 * All Rights Reserved isgaar
 */
#ifndef DOPAMINE_CORE_AUDIO_AUTOMIX_TRANSITIONPLANNER_H
#define DOPAMINE_CORE_AUDIO_AUTOMIX_TRANSITIONPLANNER_H

#include <QString>
#include <QtGlobal>

namespace Dopamine::Core::Audio::Automix {

enum class TransitionKind {
    Beatmatch,
    Armonico,
    FundidoSuave
};

struct TransitionPlan {
    TransitionKind kind;
    double durationSec = 0.0;
    qint64 fadeFrames = 0;
    int bars = 0;
};

class TransitionPlanner {
public:
    static double foldTempo(double bpmA, double bpmB);

    static bool isCamelotCompatible(const QString &keyA,
                                    const QString &keyB);

    static TransitionPlan plan(
        double bpmA,
        double confA,
        const QString &keyA,
        double bpmB,
        double confB,
        const QString &keyB,
        double defaultFadeSec = 0.0,
        int sampleRate = 44100);
};

} // namespace Dopamine::Core::Audio::Automix

#endif // DOPAMINE_CORE_AUDIO_AUTOMIX_TRANSITIONPLANNER_H
