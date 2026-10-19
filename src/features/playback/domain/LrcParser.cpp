/**
 * Location: src/features/playback/domain/LrcParser.cpp
 * Function: Implementation of LRC line tokenizer and binary search selector
 * for real-time highlighted lyric displays.
 * All Rights Reserved isgaar
 */
#include "LrcParser.h"
#include <QRegularExpression>
#include <algorithm>

namespace Dopamine::Features::Playback::Domain {

QList<LrcLine> LrcParser::parse(const QString &content) {
    QList<LrcLine> result;
    static const QRegularExpression regex(
        R"(\[(\d{1,2}):(\d{2}(?:\.\d{1,3})?)\](.*))");

    const QStringList rawLines = content.split('\n');
    for (const QString &raw : rawLines) {
        QString trimmed = raw.trimmed();
        auto match = regex.match(trimmed);
        if (match.hasMatch()) {
            qint64 mins = match.captured(1).toLongLong();
            double secs = match.captured(2).toDouble();
            qint64 timeMs = (mins * 60000) + static_cast<qint64>(secs * 1000.0);
            QString txt = match.captured(3).trimmed();
            result.append(LrcLine{timeMs, txt});
        }
    }

    std::sort(result.begin(), result.end(),
              [](const LrcLine &a, const LrcLine &b) {
                  return a.timeMs < b.timeMs;
              });

    return result;
}

int LrcParser::activeLineIndex(const QList<LrcLine> &lines,
                               qint64 positionMs) {
    if (lines.isEmpty() || positionMs < lines.first().timeMs) {
        return -1;
    }

    int active = 0;
    for (int i = 0; i < lines.size(); ++i) {
        if (lines[i].timeMs <= positionMs) {
            active = i;
        } else {
            break;
        }
    }
    return active;
}

} // namespace Dopamine::Features::Playback::Domain
