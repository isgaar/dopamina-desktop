/**
 * Location: src/features/library/domain/LibraryTrack.h
 * Function: Data structure defining catalog track metadata, audio analysis,
 * station affinity, and relative media paths for library operations.
 * All Rights Reserved isgaar
 */
#ifndef DOPAMINE_FEATURES_LIBRARY_DOMAIN_LIBRARYTRACK_H
#define DOPAMINE_FEATURES_LIBRARY_DOMAIN_LIBRARYTRACK_H

#include <QString>
#include <QVariantMap>

namespace Dopamine::Features::Library::Domain {

struct LibraryTrack {
    int id = 0;
    QString title;
    QString artist;
    QString album;
    QString coverUrl;
    int duration = 0;
    QString genre;

    QVariantMap toVariantMap() const {
        return {
            {"id", id},
            {"title", title},
            {"artist", artist},
            {"album", album},
            {"cover", coverUrl},
            {"duration", duration},
            {"genre", genre}
        };
    }
};

} // namespace Dopamine::Features::Library::Domain

#endif // DOPAMINE_FEATURES_LIBRARY_DOMAIN_LIBRARYTRACK_H
