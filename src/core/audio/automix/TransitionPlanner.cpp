/**
 * Location: src/core/audio/automix/TransitionPlanner.cpp
 * Function: Implementation of TransitionPlanner calculating tempo folding,
 * Camelot key compatibility, and bar-aligned frame crossfade parameters.
 * All Rights Reserved isgaar
 */
#include "TransitionPlanner.h"
#include <cmath>

namespace Dopamine::Core::Audio::Automix {

double TransitionPlanner::foldTempo(double bpmA, double bpmB) {
    if (bpmA <= 0.0 || bpmB <= 0.0) return 1.0;
    double r = bpmB / bpmA;
    while (r < 0.75) r *= 2.0;
    while (r >= 1.5) r /= 2.0;
    return r;
}

bool TransitionPlanner::isCamelotCompatible(const QString &keyA,
                                            const QString &keyB) {
    QString ka = keyA.trimmed().toUpper();
    QString kb = keyB.trimmed().toUpper();
    if (ka.size() < 2 || kb.size() < 2) return false;
    QChar modeA = ka.at(ka.size() - 1);
    QChar modeB = kb.at(kb.size() - 1);
    bool okA = false, okB = false;
    int numA = ka.left(ka.size() - 1).toInt(&okA);
    int numB = kb.left(kb.size() - 1).toInt(&okB);
    if (!okA || !okB || numA < 1 || numA > 12 || numB < 1 || numB > 12) {
        return false;
    }
    if (numA == numB) return true;
    if (modeA == modeB) {
        int diff = std::abs(numA - numB);
        return diff == 1 || diff == 11;
    }
    return false;
}

TransitionPlan TransitionPlanner::plan(
    double bpmA, double confA, const QString &keyA,
    double bpmB, double confB, const QString &keyB,
    double defaultFadeSec, int sampleRate) {
    bool canBeatmatch = false;
    int bars = 0;
    if (confA >= 0.6 && confB >= 0.6 && bpmA > 0.0 && bpmB > 0.0) {
        double r = foldTempo(bpmA, bpmB);
        if (std::abs(r - 1.0) <= 0.03 && isCamelotCompatible(keyA, keyB)) {
            canBeatmatch = true;
            bars = (std::abs(r - 1.0) <= 0.015) ? 16 : 8;
        }
    }

    if (canBeatmatch) {
        qint64 frames = std::round(bars * 4.0 * 60.0 * sampleRate / bpmA);
        double dur = static_cast<double>(frames) / sampleRate;
        return {TransitionKind::Beatmatch, dur, frames, bars};
    }

    if (isCamelotCompatible(keyA, keyB)) {
        double dur = (defaultFadeSec > 0.0) ? defaultFadeSec : 4.0;
        qint64 frames = std::round(dur * sampleRate);
        return {TransitionKind::Armonico, dur, frames, 0};
    }

    double dur = std::max(0.0, defaultFadeSec);
    qint64 frames = std::round(dur * sampleRate);
    return {TransitionKind::FundidoSuave, dur, frames, 0};
}

} // namespace Dopamine::Core::Audio::Automix
