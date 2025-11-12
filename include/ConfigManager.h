#pragma once
#include <QString>

class ConfigManager {
public:
    explicit ConfigManager(const QString &filePath);
    bool load();

    QString uuid() const { return m_uuid; }
    QString flightId() const { return m_flightId; }
    QString sourceEndpoint() const { return m_sourceEndpoint; }
    QString destEndpoint() const { return m_destEndpoint; }

private:
    QString m_filePath;
    QString m_uuid;
    QString m_flightId;
    QString m_sourceEndpoint;
    QString m_destEndpoint;
};
