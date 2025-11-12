#include "TelemetryManager.h"
#include <QDateTime>
#include <QUrl>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonObject>
#include <QJsonDocument>
#include <QDomDocument>
#include <QDebug>

TelemetryManager::TelemetryManager(QObject *parent)
    : QObject(parent)
{
    //Настраиваем периодический опрос локального сервера
    connect(&pollTimer, &QTimer::timeout, this, &TelemetryManager::pollLocalServer);

    // Частота опроса — 10 Гц (100 мс)
    pollTimer.start(100);
}

void TelemetryManager::setConfig(const QString &uuid,
                                 const QString &flightId,
                                 const QString &sourceEndpoint,
                                 const QString &destEndpoint)
{
    // Сохраняем конфигурационные данные, полученные из config.ini
    m_uuid = uuid;
    m_flightId = flightId;
    m_sourceEndpoint = sourceEndpoint;
    m_destEndpoint = destEndpoint;

    // Логируем загруженные параметры
    /*
    qInfo() << "  TelemetryManager configured:";
    qInfo() << "  UUID:" << m_uuid;
    qInfo() << "  Flight ID:" << m_flightId;
    qInfo() << "  Source endpoint:" << m_sourceEndpoint;
    qInfo() << "  Destination endpoint:" << m_destEndpoint;
    */
}

void TelemetryManager::start()
{
    // Запуск основного процесса — просто выводим сообщение в лог
    qDebug() << "TelemetryManager started.";
}

void TelemetryManager::pollLocalServer()
{
    // Проверяем, что источник данных указан в конфиге
    if (m_sourceEndpoint.isEmpty()) {
        qWarning() << "Source endpoint not configured!";
        return;
    }

    // Формируем GET-запрос к локальному HTTP-серверу (источнику телеметрии)
    const QUrl url = QUrl::fromUserInput(m_sourceEndpoint);
    QNetworkRequest request{url};

    // Отправляем запрос и получаем ответ асинхронно
    QNetworkReply *reply = network.get(request);

    //️ Обрабатываем завершение запроса
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        QByteArray data = reply->readAll();
        reply->deleteLater(); // очищаем объект после завершения

        double altitude = 0, roll = 0;

        // Парсим XML с локального сервера (высота и крен)
        if (parseXml(data, altitude, roll)) {
            // Формируем JSON-пакет телеметрии
            QJsonObject json;
            QJsonObject mission{{"flight_id", m_flightId}};
            QJsonObject uas{{"altitude_abs", altitude}};
            QJsonObject attitude{{"roll", roll}};
            QJsonObject state{
                {"timestamp", QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs)},
                {"uas", uas},
                {"attitude", attitude}
            };

            json["uuid"] = m_uuid;
            json["mission"] = mission;
            json["current_state"] = state;

            // Преобразуем JSON-объект в компактную строку
            QByteArray jsonData = QJsonDocument(json).toJson(QJsonDocument::Compact);

            // Отправляем телеметрию на удалённый сервер
            sendTelemetry(jsonData);
        } else {
            qWarning() << "XML parse error";
        }
    });
}

bool TelemetryManager::parseXml(const QByteArray &xml, double &altitude, double &roll)
{

    QDomDocument doc;
    if (!doc.setContent(xml))
        return false;

    // Находим нужные элементы по тегам
    QDomElement root = doc.documentElement();
    QDomElement altElem = root.firstChildElement("cmd.pos.altitude");
    QDomElement rollElem = root.firstChildElement("est.att.roll");

    if (altElem.isNull() || rollElem.isNull())
        return false;

    // Преобразуем текстовые значения в числа
    bool ok1 = false, ok2 = false;
    altitude = altElem.text().toDouble(&ok1);
    roll = rollElem.text().toDouble(&ok2);

    // Возвращаем true, если оба значения успешно считаны
    return ok1 && ok2;
}

void TelemetryManager::sendTelemetry(const QByteArray &json)
{
    // Проверяем наличие адреса назначения
    if (m_destEndpoint.isEmpty()) {
        qWarning() << "⚠️ Destination endpoint not configured!";
        return;
    }

    // Если предыдущий POST-запрос ещё выполняется — ставим данные в очередь
    if (postInProgress) {
        buffer.enqueue(json);
        return;
    }

    // Формируем POST-запрос на удалённый сервер
    const QUrl url = QUrl::fromUserInput(m_destEndpoint);
    QNetworkRequest request{url};
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    // Отмечаем, что отправка в процессе
    postInProgress = true;

    // Отправляем JSON телеметрию (асинхронно)
    QNetworkReply *reply = network.post(request, json);

    // Обработка завершения POST-запроса
    connect(reply, &QNetworkReply::finished, this, [this, reply, json]() {
        postInProgress = false;

        if (reply->error() == QNetworkReply::NoError) {
            // Успешная отправка
            qDebug() << "[POST OK]" << reply->readAll();

            // Если очередь не пуста — отправляем следующий пакет
            flushQueue();
        } else {
            // Ошибка отправки — сохраняем пакет в очередь
            qWarning() << "[POST FAILED]" << reply->errorString();
            buffer.enqueue(json);
            qDebug() << "[QUEUE SIZE]" << buffer.size();
        }

        // Освобождаем память
        reply->deleteLater();
    });
}

void TelemetryManager::flushQueue()
{
    // Проверяем, есть ли отложенные пакеты
    if (buffer.isEmpty())
        return;

    // Берём следующий пакет из очереди
    QByteArray next = buffer.dequeue();

    qDebug() << "[QUEUE FLUSH]" << "sending queued packet, remaining =" << buffer.size();

    // Отправляем его повторно
    sendTelemetry(next);
}
