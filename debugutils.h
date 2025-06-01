#ifndef DEBUGUTILS_H
#define DEBUGUTILS_H

#include <QDebug>
#include <QString>

#define DEBUG 1  // Set to 1 for debug mode, 0 for normal mode

void bich(const QString &message, QtMsgType type = QtDebugMsg);

#endif
