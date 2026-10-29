/**
 * Location: src/core/platform/media/mpris/MprisRootAdaptor.h
 * Function: D-Bus adaptor exporting org.mpris.MediaPlayer2 interface for
 * desktop application identity, URI schemes, and window management.
 * All Rights Reserved isgaar
 */
#ifndef DOPAMINE_CORE_PLATFORM_MEDIA_MPRIS_MPRISROOTADAPTOR_H
#define DOPAMINE_CORE_PLATFORM_MEDIA_MPRIS_MPRISROOTADAPTOR_H

#include <QDBusAbstractAdaptor>
#include <QStringList>

namespace Dopamine::Core::Platform::Media::Mpris {

class MprisPlayerService;

class MprisRootAdaptor : public QDBusAbstractAdaptor {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.mpris.MediaPlayer2")
    Q_CLASSINFO("D-Bus Introspection",
        "  <interface name=\"org.mpris.MediaPlayer2\">\n"
        "    <method name=\"Raise\"/>\n"
        "    <method name=\"Quit\"/>\n"
        "    <property name=\"CanQuit\" type=\"b\" access=\"read\"/>\n"
        "    <property name=\"Fullscreen\" type=\"b\" access=\"readwrite\"/>\n"
        "    <property name=\"CanSetFullscreen\" type=\"b\" access=\"read\"/>\n"
        "    <property name=\"CanRaise\" type=\"b\" access=\"read\"/>\n"
        "    <property name=\"HasTrackList\" type=\"b\" access=\"read\"/>\n"
        "    <property name=\"Identity\" type=\"s\" access=\"read\"/>\n"
        "    <property name=\"DesktopEntry\" type=\"s\" access=\"read\"/>\n"
        "    <property name=\"SupportedUriSchemes\" type=\"as\" access=\"read\"/>\n"
        "    <property name=\"SupportedMimeTypes\" type=\"as\" access=\"read\"/>\n"
        "  </interface>\n"
    )

    Q_PROPERTY(bool CanQuit READ canQuit)
    Q_PROPERTY(bool Fullscreen READ fullscreen WRITE setFullscreen)
    Q_PROPERTY(bool CanSetFullscreen READ canSetFullscreen)
    Q_PROPERTY(bool CanRaise READ canRaise)
    Q_PROPERTY(bool HasTrackList READ hasTrackList)
    Q_PROPERTY(QString Identity READ identity)
    Q_PROPERTY(QString DesktopEntry READ desktopEntry)
    Q_PROPERTY(QStringList SupportedUriSchemes READ supportedUriSchemes)
    Q_PROPERTY(QStringList SupportedMimeTypes READ supportedMimeTypes)

public:
    explicit MprisRootAdaptor(MprisPlayerService *parent);

    bool canQuit() const;
    bool fullscreen() const;
    void setFullscreen(bool value);
    bool canSetFullscreen() const;
    bool canRaise() const;
    bool hasTrackList() const;
    QString identity() const;
    QString desktopEntry() const;
    QStringList supportedUriSchemes() const;
    QStringList supportedMimeTypes() const;

public slots:
    void Raise();
    void Quit();

private:
    MprisPlayerService *m_service = nullptr;
};

} // namespace Dopamine::Core::Platform::Media::Mpris

#endif // DOPAMINE_CORE_PLATFORM_MEDIA_MPRIS_MPRISROOTADAPTOR_H
