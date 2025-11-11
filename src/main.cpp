
#include <QCoreApplication>
#include "TelemetryManager.h"

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    TelemetryManager tm;
    return app.exec();
}
