/**********************************************************************
** openffucontrol-particleserver - a daemon for data acquisition from
** cleanroom particle monitoring devices into an influx time-series database
** Copyright (C) 2023 Smart Micro Engineering GmbH
** This program is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU General Public License for more details.
** You should have received a copy of the GNU General Public License
** along with this program. If not, see <http://www.gnu.org/licenses/>.
**********************************************************************/

#include "remotecontroller.h"

RemoteController::RemoteController(QObject *parent, ParticleCounterDatabase* pcDB, Loghandler* loghandler) : QObject(parent)
{
#ifdef QT_DEBUG
    fprintf(stdout, "Server started\n");
#endif

    m_pcDB = pcDB;
    m_loghandler = loghandler;
    m_noConnection = true;

    connect(&m_timer_connectionTimeout, &QTimer::timeout, this, &RemoteController::slot_connectionTimeout);
    connect(this, &RemoteController::signal_connected, &m_timer_connectionTimeout, &QTimer::stop);
    m_timer_connectionTimeout.setSingleShot(true);
    m_timer_connectionTimeout.start(30000); // 30 Sec.

    connect(&m_server, &QTcpServer::newConnection,  this, &RemoteController::slot_new_connection);
    m_server.listen(QHostAddress::LocalHost, 16001);    // Restrict to localhost (ssh tunnel endpoint)
}

RemoteController::~RemoteController()
{

}

bool RemoteController::isConnected()
{
    return (!m_noConnection);
}

void RemoteController::slot_new_connection()
{
    QTcpSocket* newSocket = m_server.nextPendingConnection();
    this->m_socket_list.append(newSocket);

    RemoteClientHandler* remoteClientHandler = new RemoteClientHandler(this, newSocket, m_pcDB, m_loghandler);
    connect(remoteClientHandler, &RemoteClientHandler::signal_broadcast,
            this, &RemoteController::slot_broadcast);
    connect(remoteClientHandler, &RemoteClientHandler::signal_connectionClosed,
            this, &RemoteController::slot_connectionClosed);

    if (m_noConnection)
    {
        m_noConnection = false;
        m_loghandler->slot_entryGone(LogEntry::Error, "Remotecontroller", "No connection to server.");
        emit signal_connected();
    }
}

void RemoteController::slot_broadcast(QByteArray data)
{
    foreach(QTcpSocket* socket, this->m_socket_list)
    {
        socket->write(data + "\r\n");
    }
}

void RemoteController::slot_connectionClosed(QTcpSocket *socket, RemoteClientHandler *remoteClientHandler)
{
    this->m_socket_list.removeOne(socket);
    delete remoteClientHandler;
#ifdef QT_DEBUG
    fprintf (stdout, "ClientHandler deleted\r\n");
#endif

    if (m_socket_list.isEmpty())
    {
        m_noConnection = true;
        m_loghandler->slot_newEntry(LogEntry::Error, "Remotecontroller", "No connection to server.");
        emit signal_disconnected();
    }
}

void RemoteController::slot_connectionTimeout()
{
    m_loghandler->slot_newEntry(LogEntry::Error, "Remotecontroller", "No connection to server.");
}
