/**
 * Location: src/features/playback/domain/LrcParser.h
 * Function: Parser and synchronization model for timestamped .lrc lyric files
 * identifying active lines based on playback positions.
 * All Rights Reserved isgaar
 */
#ifndef DOPAMINE_FEATURES_PLAYBACK_DOMAIN_LRCPARSER_H
#define DOPAMINE_FEATURES_PLAYBACK_DOMAIN_LRCPARSER_H

#include <QString>
#include <QList>
#include <QtGlobal>

namespace Dopamine::Features::Playback::Domain {

struct LrcLine {
    qint64 timeMs = 0;
    QString text;

    bool operator==(const LrcLine &other) const {
        return timeMs == other.timeMs && text == other.text;
    }
};

class LrcParser {
public:
    static QList<LrcLine> parse(const QString &content);
    static int activeLineIndex(const QList<LrcLine> &lines,
                               qint64 positionMs);
};

} // namespace Dopamine::Features::Playback::Domain

#endif // DOPAMINE_FEATURES_PLAYBACK_DOMAIN_LRCPARSER_H
