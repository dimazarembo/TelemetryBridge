#include <QCoreApplication>
#include "ConfigManager.h"
#include "TelemetryManager.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    ConfigManager config("config.ini");
    if (!config.load()) {
        qCritical() << "❌ Ошибка загрузки конфигурации";
        return -1;
    }

    TelemetryManager telemetry;
    telemetry.setConfig(
        config.uuid(),
        config.flightId(),
        config.sourceEndpoint(),
        config.destEndpoint()
        );

    telemetry.start();

    return app.exec();
}
