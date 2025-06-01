// config.cpp
#include "config.h"
#include "debugutils.h"
#include <QFile>
#include <QTextStream>
#include <QDebug>

QMap<QString, QString> loadConfig(const QString& filePath) {
    QMap<QString, QString> configMap;
    QFile file(filePath);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        bich("Failed to open config file:" + filePath);
        return configMap;
    }

    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();

        if (line.isEmpty() || line.startsWith("#"))
            continue; // Skip blank lines and comments

        QStringList parts = line.split("=", Qt::KeepEmptyParts);
        if (parts.size() == 2) {
            QString key = parts[0].trimmed();
            QString value = parts[1].trimmed();
            configMap.insert(key, value);
        }
    }
    bich("Opened config file: " + filePath);
    file.close();
    return configMap;
}
