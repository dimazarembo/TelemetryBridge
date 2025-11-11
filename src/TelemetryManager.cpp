#include "TelemetryManager.h"
#include <QDateTime>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QFile>
#include <QJsonObject>
#include <QJsonDocument>
#include <QDebug>

TelemetryManager::TelemetryManager(QObject *parent) : QObject(parent) {
    // Загружаем config.json
    QFile cfg("config.json");
    if (cfg.open(QIODevice::ReadOnly)) {
        QByteArray content = cfg.readAll();
        qDebug() << "Raw content:" << content;
        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(content, &err);
        if (err.error == QJsonParseError::NoError && doc.isObject()) {
            QJsonObject obj = doc.object();
            uuid = obj.value("uuid").toString();
            flightId = obj.value("flight_id").toString();
            qDebug() << "Loaded uuid:" << uuid << "flight_id:" << flightId;
        } else {
            qWarning() << "config.json not found or invalid. Using defaults.";
            uuid = "0000-0000";
            flightId = "UNKNOWN";
        }
        cfg.close();
    } else {
        qWarning() << "config.json not found or invalid. Using defaults.";
        uuid = "0000-0000";
        flightId = "UNKNOWN";
    }

    connect(&pollTimer, &QTimer::timeout, this, &TelemetryManager::pollLocalServer);
    pollTimer.start(100); // 10 Гц
}

void TelemetryManager::start() {
    qDebug() << "Telemetry manager started.";
}

void TelemetryManager::pollLocalServer() {
    QNetworkRequest req(QUrl("http://10.241.113.130:9280/mandala"));
    QNetworkReply *reply = network.get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        QByteArray data = reply->readAll();
        reply->deleteLater();

    //    qDebug() << "Server response:" << data;

        double altitude = 0, roll = 0;
        if (parseXml(data, altitude, roll)) {
            QJsonObject json;
            QJsonObject mission { {"flight_id", flightId} };
            QJsonObject uas { {"altitude_abs", altitude} };
            QJsonObject attitude { {"roll", roll} };
            QJsonObject state {
                {"timestamp", QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs)},
                {"uas", uas},
                {"attitude", attitude}
            };
            json["uuid"] = uuid;
            json["mission"] = mission;
            json["current_state"] = state;

            QByteArray jsonData = QJsonDocument(json).toJson(QJsonDocument::Compact);
          //  qDebug().noquote() << jsonData;

            sendTelemetry(jsonData);
        } else {
            qWarning() << "XML parse error";
        }
    });
}

bool TelemetryManager::parseXml(const QByteArray &xml, double &altitude, double &roll) {
    QDomDocument doc;
    if (!doc.setContent(xml)) return false;

    QDomElement root = doc.documentElement();
    QDomElement altElem = root.firstChildElement("cmd.pos.altitude");
    QDomElement rollElem = root.firstChildElement("est.att.roll");

    if (altElem.isNull() || rollElem.isNull()) return false;

    //если будут числа с разделителем в виде запятой то раскоментить
    /* bool ok1=false, ok2=false;
    // QString altText = altElem.text().trimmed().replace(',', '.');
    // QString rollText = rollElem.text().trimmed().replace(',', '.');
    // altitude = altText.toDouble(&ok1);
    // roll     = rollText.toDouble(&ok2);
     return ok1 && ok2;*/

    bool ok1=false, ok2=false;
    altitude = altElem.text().toDouble(&ok1);
    roll     = rollElem.text().toDouble(&ok2);
    return ok1 && ok2;
}

void TelemetryManager::sendTelemetry(const QByteArray &json) {
    const QUrl url("http://178.172.132.41:9090/geoevent/telemetry");

    if (postInProgress) {
        buffer.enqueue(json);
        return;
    }

    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    postInProgress = true;

    QNetworkReply *reply = network.post(req, json);

    connect(reply, &QNetworkReply::finished, this, [this, reply, json]() {
        postInProgress = false;

        if (reply->error() == QNetworkReply::NoError) {
            qDebug() << "[POST OK]" << reply->readAll();
            flushQueue();  // пробуем отправить, если очередь непуста
        } else {
            qWarning() << "[POST FAILED]" << reply->errorString();
            // сохраняем именно JSON, который не отправился
            buffer.enqueue(json);
            qDebug() << "[QUEUE SIZE]" << buffer.size();
        }

        reply->deleteLater();
    });
}



void TelemetryManager::flushQueue() {
    if (buffer.isEmpty()) return;

    QByteArray next = buffer.dequeue();
    qDebug() << "[QUEUE FLUSH]" << "sending queued packet, remaining =" << buffer.size();
    sendTelemetry(next);
}

