#include "influxdb.h"

InfluxDB::InfluxDB(QObject *parent, Loghandler* loghandler) : QObject(parent)
{
    m_loghandler = loghandler;

    QSettings settings("/etc/openffucontrol/particleserver/config.ini", QSettings::IniFormat);
    settings.beginGroup("influxDB");

    m_hostname = settings.value("hostname", QString("localhost")).toString();
    m_port = settings.value("port", 8086).toUInt();
    m_dbUser = settings.value("username", QString()).toString();
    m_dbPassword = settings.value("password", QString()).toString();

    m_networkManager = new QNetworkAccessManager();
    connect(m_networkManager, &QNetworkAccessManager::finished, this, &InfluxDB::slot_replyFinished);
}

InfluxDB::~InfluxDB()
{
    delete m_networkManager;
}

void InfluxDB::write(QByteArray payload)
{
    // Example
    // curl -i -XPOST "http://localhost:8086/write?db=mydb&u=myusername&p=mypassword" --data-binary 'mymeas,mytag=1 myfield=91'

    QUrl url;
    url.setScheme("http");
    url.setHost(m_hostname);
    url.setPort(m_port);
    url.setPath("/write");
    if (!m_dbUser.isEmpty() && !m_dbPassword.isEmpty())
    {
        url.setUserName(m_dbUser);
        url.setPassword(m_dbPassword);
    }
    url.setQuery("db=" + m_dbName);
    m_request.setUrl(url);

    m_networkManager->post(m_request, payload);
}

void InfluxDB::slot_replyFinished(QNetworkReply *reply)
{
    if (reply->error()) {
        m_loghandler->slot_newEntry(LogEntry::Error, "InfluxDB slot_replyFinished", reply->errorString());
        return;
    }

    // Maybe do something with the reply later?
    QString answer = reply->readAll();

    reply->deleteLater();
}
