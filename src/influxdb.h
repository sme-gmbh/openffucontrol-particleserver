#ifndef INFLUXDB_H
#define INFLUXDB_H

#include <QObject>
#include <QSettings>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include "loghandler.h"

class InfluxDB : public QObject
{
    Q_OBJECT
public:
    explicit InfluxDB(QObject *parent, Loghandler* loghandler);
    ~InfluxDB();

    QString m_hostname;
    quint16 m_port;
    QString m_dbName;
    QString m_dbUser;
    QString m_dbPassword;

    void write(QByteArray payload);

private:
    Loghandler* m_loghandler;
    QNetworkAccessManager* m_networkManager;
    QNetworkRequest m_request;

private slots:
    void slot_replyFinished(QNetworkReply *reply);

signals:

};

#endif // INFLUXDB_H
