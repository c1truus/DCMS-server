// config.h
#ifndef CONFIG_H
#define CONFIG_H

#include <QString>
#include <QMap>

// Reads a simple key=value config file
QMap<QString, QString> loadConfig(const QString& filePath);

#endif // CONFIG_H
