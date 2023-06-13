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

#include "remoteclienthandler.h"

RemoteClientHandler::RemoteClientHandler(QObject *parent, QTcpSocket* socket, ParticleCounterDatabase* pcDB, Loghandler *loghandler) : QObject(parent)
{
    this->socket = socket;
    m_pcDB = pcDB;
    m_loghandler = loghandler;

    m_livemode = false;

#ifdef QT_DEBUG
    QString debugStr;

    debugStr += "New connection ";

    if (socket->socketType() == QAbstractSocket::TcpSocket)
        debugStr += "tcp from ";
    else if (socket->socketType() == QAbstractSocket::UdpSocket)
        debugStr += "udp from ";
    else if (socket->socketType() == QAbstractSocket::UnknownSocketType)
        debugStr += " - unknown socket type - ";

    debugStr += socket->peerAddress().toString() + ":" + QString().setNum(socket->peerPort()) + " \r\n";

    printf("%s", debugStr.toLatin1().data());
#endif
    socket->write("Hello\r\n");

    connect(socket, SIGNAL(readyRead()), this, SLOT(slot_read_ready()));
    connect(socket, SIGNAL(disconnected()), this, SLOT(slot_disconnected()));
    connect(m_pcDB, &ParticleCounterDatabase::signal_ParticleCounterActualDataHasChanged, this, &RemoteClientHandler::slot_ParticleCounterActualDataHasChanged);
}

void RemoteClientHandler::slot_read_ready()
{
    while (socket->canReadLine())
    {
        // Data format:
        // COMMAND [--key][=value] [--key][=value]...
        QString line = QString::fromUtf8(this->socket->readLine());
        line.remove(QRegExp("[\\r\\n]"));      // Strip newlines at the beginning and at the end
        int commandLength = line.indexOf(' ');
        if (commandLength == -1) commandLength = line.length();
        QString command = line.left(commandLength);
        line.remove(0, commandLength + 1);  // Remove command and optional space

#ifdef QT_DEBUG
        printf("Received data: \r\n");
#endif

        // Parse key/value pairs now
        QStringList commandChunks = line.split(' ', QString::SkipEmptyParts);
        QMap<QString, QString> data;

        foreach(QString commandChunk, commandChunks)
        {
#ifdef QT_DEBUG
            printf("Decoding chunk: %s\r\n", commandChunk.toUtf8().data());
#endif
            QStringList key_value_pair = commandChunk.split('=');
            if ((key_value_pair.length() > 2) || (key_value_pair.length() < 1))
            {
                socket->write("ERROR: key_value_pair length invalid\r\n");
                continue;
            }
//            // key and value are base64 encoded.
//            QString key = QString::fromUtf8(QByteArray::fromBase64(key_value_pair.at(0).toUtf8()));
//            QString value = QString::fromUtf8(QByteArray::fromBase64(key_value_pair.at(1).toUtf8()));
            QString key = key_value_pair.at(0);
            if (key.startsWith("--"))
            {
                key.remove(0, 2);   // Remove leading "--"
                if (key_value_pair.length() == 2)
                {
                    QString value = key_value_pair.at(1);
                    data.insert(key, value);
                }
                else
                {
                    data.insert(key, "query");
                }
            }
        }

        // message is distributed to other clients in this way
        //emit signal_broadcast(QByteArray);

        if (command == "help")
        {
            socket->write("This is the commandset of the openFFUcontrol remote unit:\r\n"
                          "\r\n"
                          "<COMMAND> [--key[=value]]\r\n"
                          "\r\n"
                          "COMMANDS:\r\n"
                          "    hostname\r\n"
                          "        Show the hostname of the controller.\r\n"
                          "    startlive\r\n"
                          "        Show data of particle counters in realtime. Can be stopped with stoplive\r\n"
                          "    stoplive\r\n"
                          "        Stop live showing of particle counter data.\r\n"
                          "    list-particlecounters\r\n"
                          "        Show the list of currently configured particlecounters from the controller database.\r\n"
                          "    log\r\n"
                          "        Show the log consisting of infos, warnings and errors.\r\n"
                          "\r\n"
                          "    buffers\r\n"
                          "        Show buffer levels.\r\n"
                          "\r\n"
                          "    add-particlecounter --bus=BUSNR --unit=ADR --id=ID\r\n"
                          "        Add a new particle counter with ID to the controller database at BUSNR with OCU at modbus address ADR.\r\n"
                          "\r\n"
                          "    delete-particlecounter --id=ID --bus=BUSNR\r\n"
                          "        Delete particle counter with ID from the controller database.\r\n"
                          "        Note that you can delete all particle counters of a certain bus by using BUSNR only.\r\n"
                          "\r\n"
                          "    set --parameter=VALUE\r\n"
                          "\r\n"
                          "    get --parameter\r\n"
                          "        parameter 'actual' lists all actual values of the selected unit id.\r\n"
                          "\r\n");
        }
        // ************************************************** hostname **************************************************
        else if (command == "hostname")
        {
            QString line;
            line = "Hostname=" + QHostInfo::localHostName() + "\n";
            socket->write(line.toUtf8());
        }
        // ************************************************** startlive **************************************************
        else if (command == "startlive")
        {
            QString line;
            line = "Liveshow=on\n";
            socket->write(line.toUtf8());
            m_livemode = true;
        }
        // ************************************************** stoplive **************************************************
        else if (command == "stoplive")
        {
            QString line;
            line = "Liveshow=off\n";
            socket->write(line.toUtf8());
            m_livemode = false;
        }
        // ************************************************** list-ocufans **************************************************
        else if (command == "list-particlecounters")
        {
            QList<ParticleCounter*> pcs = m_pcDB->getParticleCounters();
            foreach(ParticleCounter* pc, pcs)
            {
                QString line;

                line.sprintf("Particle Counter id=%i busID=%i\r\n", pc->getId(), pc->getBusID());

                socket->write(line.toUtf8());
            }
        }
        // ************************************************** log **************************************************
        else if (command == "log")
        {
            socket->write(m_loghandler->toString(LogEntry::Info).toUtf8() + "\n");
            socket->write(m_loghandler->toString(LogEntry::Warning).toUtf8() + "\n");
            socket->write(m_loghandler->toString(LogEntry::Error).toUtf8() + "\n");
        }
        // ************************************************** buffers **************************************************
        else if (command == "buffers")
        {
            int i = 0;
            foreach(ModBus* bus, *m_pcDB->getBusList())
            {
                int telegramQueueLevel_standardPriority = bus->getSizeOfTelegramQueue(false);
                int telegramQueueLevel_highPriority = bus->getSizeOfTelegramQueue(true);
                QString line;
                line.sprintf("Particle Counter ModBus line %i: TelegramQueueLevel_standardPriority=%i TelegramQueueLevel_highPriority=%i\r\n",
                             i, telegramQueueLevel_standardPriority, telegramQueueLevel_highPriority);
                socket->write(line.toUtf8());
                i++;
            }

        }
        // ************************************************** add-particlecounter **************************************************
        else if (command == "add-particlecounter")
        {
            bool ok;

            QString busString = data.value("bus");
            int bus = busString.toInt(&ok);
            if (busString.isEmpty() || !ok)
            {
                socket->write("Error[Commandparser]: parameter \"bus\" not specified or bus cannot be parsed. Abort.\r\n");
                continue;
            }

            QString idString = data.value("id");
            int id = idString.toInt(&ok);
            if (idString.isEmpty() || !ok)
            {
                socket->write("Error[Commandparser]: parameter \"id\" not specified or id can not be parsed. Abort.\r\n");
                continue;
            }

            QString unitString = data.value("unit");
            int unit = unitString.toInt(&ok);
            if ((unitString.isEmpty() || !ok) && (command == "add-particlecounter"))
            {
                socket->write("Error[Commandparser]: parameter \"unit\" not specified or id can not be parsed. Abort.\r\n");
                continue;
            }

#ifdef DEBUG
            socket->write("add-particlecounter bus=" + QString().setNum(bus).toUtf8() + " id=" + QString().setNum(id).toUtf8() + " unit=" + QString().setNum(unit).toUtf8() + "\r\n");
#endif
            QString response;
            if (command == "add-particlecounter")
                response = m_pcDB->addParticleCounter(id, bus, unit);

            socket->write(response.toUtf8() + "\r\n");
        }
        // ************************************************** delete-particlecounter **************************************************
        else if (command == "delete-particlecounter")
        {
            bool ok;
            QString response;
            bool noID = false;
            bool noBus = false;

            QString idString = data.value("id");
            int id = idString.toInt(&ok);
            if (idString.isEmpty() || !ok)
            {
                noID = true;
            }
            else
            {
                response += m_pcDB->deleteParticleCounter(id) + "\n";
            }

            QString busString = data.value("bus");
            int bus = busString.toInt(&ok);
            if (busString.isEmpty() || !ok)
            {
                noBus = true;
            }
            else
            {
                foreach (ParticleCounter* pc, m_pcDB->getParticleCounters(bus))
                {
                    response += m_pcDB->deleteParticleCounter(pc->getId()) + "\n";
                }
            }

            if (noID && noBus)
                response = "Error[Commandparser]: Neither parameter \"id\" nor parameter \"bus\" specified. Abort.\r\n";


#ifdef DEBUG
            socket->write("delete-auxfan id=" + QString().setNum(id).toUtf8() + "\r\n");
#endif


            socket->write(response.toUtf8() + "\r\n");
        }
        // ************************************************** set **************************************************
        else if (command == "set")
        {
            bool ok;
            QString idString = data.value("id");
            int id = idString.toInt(&ok);
            if (idString.isEmpty() || !ok)
            {
                socket->write("Error[Commandparser]: parameter \"id\" not specified or id can not be parsed. Abort.\r\n");
                continue;
            }

#ifdef DEBUG
            socket->write("set id=" + QString().setNum(id).toUtf8() + "\r\n");
#endif
            QString response;
            if (m_pcDB->getParticleCounterByID(id) != nullptr)
                response = m_pcDB->setParticleCounterData(id, data);
            socket->write(response.toUtf8() + "\r\n");
        }
        // ************************************************** get **************************************************
        else if (command == "get")
        {
            bool ok;
            QString idString = data.value("id");
            int id = idString.toInt(&ok);
            if (idString.isEmpty() || !ok)
            {
                socket->write("Error[Commandparser]: parameter \"id\" not specified or id can not be parsed. Abort.\r\n");
                continue;
            }

#ifdef DEBUG
            socket->write("get id=" + id.toUtf8() + "\r\n");
#endif
            QMap<QString,QString> responseData;

            if (m_pcDB->getParticleCounterByID(id) != nullptr)
                responseData = m_pcDB->getParticleCounterData(id, data.keys("query"));
            if (responseData.value("actualData").toInt() == 1)
            {
                socket->write("ActualData from id=" + QString().setNum(id).toUtf8());
                responseData.remove("actualData");  // Remove special treatment marker
            }
            else
                socket->write("Data from id=" + QString().setNum(id).toUtf8());
            QString errors;
            foreach(QString key, responseData.keys())
            {
                QString response = responseData.value(key);
                if (!response.startsWith("Error[ParticleCounter]:"))
                    socket->write(" " + key.toUtf8() + "=" + response.toUtf8());
                else
                    errors.append(response + "\r\n");
            }
            socket->write("\r\n");
            if (!errors.isEmpty())
            {
                socket->write(errors.toUtf8());
            }
        }
        // ************************************************** UNSUPPORTED COMMAND **************************************************
        else
        {
            // If control reaches this point, we have an unsupported command
            socket->write("ERROR: Command not supported: " + command.toUtf8() + "\r\n");
        }
    }
}

void RemoteClientHandler::slot_disconnected()
{
#ifdef QT_DEBUG
    QString debugStr;

    debugStr += "Closed connection ";

    if (socket->socketType() == QAbstractSocket::TcpSocket)
        debugStr += "tcp from ";
    else if (socket->socketType() == QAbstractSocket::UdpSocket)
        debugStr += "udp from ";
    else if (socket->socketType() == QAbstractSocket::UnknownSocketType)
        debugStr += " - unknown socket type - ";

    debugStr += socket->peerAddress().toString() + ":" + QString().setNum(socket->peerPort()) + " \r\n";

    printf("%s", debugStr.toLatin1().data());
#endif
    emit signal_connectionClosed(this->socket, this);

}

void RemoteClientHandler::slot_ParticleCounterActualDataHasChanged(int id)
{
    if (m_livemode)
    {
        socket->write("ActualData from id=" + QString().setNum(id).toUtf8());
        QMap<QString,QString> responseData = m_pcDB->getParticleCounterData(id, QStringList() << "actual");
        responseData.remove("actualData");  // Remove special treatment marker
        foreach(QString key, responseData.keys())
        {
            QString response = responseData.value(key);
            if (!response.startsWith("Error[Particle Counter]:"))
                socket->write(" " + key.toUtf8() + "=" + response.toUtf8());
        }
        socket->write("\r\n");
    }
}
