#ifndef TELEMETRYMANAGER_H
#define TELEMETRYMANAGER_H

#include <QObject>
#include <QTimer>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QDomDocument>
#include <QQueue>

class TelemetryManager : public QObject {
    Q_OBJECT
public:
    explicit TelemetryManager(QObject *parent = nullptr);
    void setConfig(const QString &uuid,
                   const QString &flightId,
                   const QString &sourceEndpoint,
                   const QString &destEndpoint);
    void start();

private slots:
    void pollLocalServer();

private:    
    QString m_uuid;
    QString m_flightId;
    QString m_sourceEndpoint;
    QString m_destEndpoint;

    bool parseXml(const QByteArray &xml, double &altitude, double &roll);
    void sendTelemetry(const QByteArray &json);
    void flushQueue();


    QNetworkAccessManager network;
    QTimer pollTimer;
    QString uuid;
    QString flightId;
    QQueue<QByteArray> buffer;   // очередь неотправленных пакетов
    bool postInProgress = false;
};

#endif // TELEMETRYMANAGER_H
