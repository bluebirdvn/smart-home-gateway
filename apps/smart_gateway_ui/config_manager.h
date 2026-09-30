#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H
#include <QObject>
#include <QString>
#include <QDebug>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMutexLocker>

struct DBusConfig {
    QString serviceName;
    QString objectPath;
    QString interfaceName;
};

class ConfigManager {
public:
    static ConfigManager& getInstance() {
        static ConfigManager instance;
        return instance;
    }

    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;

    bool loadConfig(const QString& path) {
        QMutexLocker locker(&lock);
        if (loaded) {
            qInfo() << "config already loaded from: " << path;
            return true;
        }

        QFile file(path);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qWarning() << "can't open config file: " << path;
            return false;
        }

        QByteArray content = file.readAll();
        file.close();

        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(content, &error);

        if (error.error != QJsonParseError::NoError || !doc.isObject()) {
            qWarning() << "[ConfigManager] parse config failed:" << error.errorString();
            return false;
        }

        QJsonObject rootObj = doc.object();
        const QStringList modules = {"Mesh", "Database", "Mqtt", "UI"};

        for (const QString& mod : modules) {
            if (rootObj.contains(mod) && rootObj[mod].isObject()) {
                QJsonObject modObj = rootObj[mod].toObject();

                DBusConfig cfg;
                cfg.serviceName   = modObj["serviceName"].toString();
                cfg.objectPath    = modObj["objectPath"].toString();
                cfg.interfaceName = modObj["interfaceName"].toString();

                if (cfg.serviceName.isEmpty() || cfg.objectPath.isEmpty() || cfg.interfaceName.isEmpty()) {
                    qWarning() << "config of" << mod << "missing fields";
                }

                configs.insert(mod, cfg);
            } else {
                qWarning() << "file not contains module: " << mod;
            }
        }

        loaded = true;
        qInfo() << "load config success from path: " << path;
        return true;
    }

    DBusConfig getConfig(const QString& moduleName) {
        QMutexLocker locker(&lock);
        if (!configs.contains(moduleName)) {
            qWarning() << "module not exist: " << moduleName;
        }
        return configs.value(moduleName, DBusConfig());
    }

    bool isValid(const QString& moduleName) {
        QMutexLocker locker(&lock);
        if (!configs.contains(moduleName)) {
            return false;
        }
        const DBusConfig& cfg = configs[moduleName];
        return !cfg.serviceName.isEmpty() && !cfg.objectPath.isEmpty() && !cfg.interfaceName.isEmpty();
    }

    bool isLoaded() const {
        return loaded;
    }

private:
    ConfigManager() = default;
    ~ConfigManager() = default;

    QMap<QString, DBusConfig> configs;
    bool loaded = false;
    QMutex lock;
};


#endif 
