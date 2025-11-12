#include "ConfigManager.h"
#include <QSettings>
#include <QDebug>

ConfigManager::ConfigManager(const QString &filePath)
    : m_filePath(filePath)
{}

bool ConfigManager::load() {
    QSettings settings(m_filePath, QSettings::IniFormat);

    settings.beginGroup("telemetry");
    m_uuid = settings.value("uuid").toString();
    m_flightId = settings.value("flight_id").toString();
    settings.endGroup();

    settings.beginGroup("source");
    m_sourceEndpoint = settings.value("endpoint").toString();
    settings.endGroup();

    settings.beginGroup("destination");
    m_destEndpoint = settings.value("endpoint").toString();
    settings.endGroup();

    if (m_uuid.isEmpty() || m_flightId.isEmpty() || m_sourceEndpoint.isEmpty() || m_destEndpoint.isEmpty()) {
        qWarning() << "❌ Invalid or incomplete configuration in" << m_filePath;
        return false;
    }

    qInfo() << "✅ Loaded config from" << m_filePath;
    qInfo() << "  uuid:" << m_uuid;
    qInfo() << "  flight_id:" << m_flightId;
    qInfo() << "  source endpoint:" << m_sourceEndpoint;
    qInfo() << "  destination endpoint:" << m_destEndpoint;

    return true;
}
