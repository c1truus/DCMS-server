#include "debugutils.h"

void bich(const QString &message, QtMsgType type) {
#ifdef DEBUG
    switch (type) {
    case QtDebugMsg:
        qDebug() << message;
        break;
    case QtWarningMsg:
        qWarning() << message;
        break;
    case QtCriticalMsg:
        qCritical() << message;
        break;
    case QtInfoMsg:
        qInfo() << message;
        break;
    case QtFatalMsg:
        qFatal("%s", message.toUtf8().constData());
        break;
    default:
        qDebug() << message;
        break;
    }
#endif
}
