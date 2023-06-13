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

#include "particlecounterdatabase.h"

ParticleCounterDatabase::ParticleCounterDatabase(QObject *parent, ParticleCounterModbusSystem *pcModbusSystem, InfluxDB *influxDB, Loghandler *loghandler)
{
    m_pcModbusSystem = pcModbusSystem;

    m_pcModbusList = pcModbusSystem->pcModbuslist(); // Try to eliminate this!

    m_influxDB = influxDB;

    m_loghandler = loghandler;

    m_settings = new QSettings("/etc/openffucontrol/particleserver/config.ini", QSettings::IniFormat);
    m_settings->beginGroup("influxDB");

    // High level bus-system response connections
    connect(m_pcModbusSystem, &ParticleCounterModbusSystem::signal_receivedHoldingRegisterData, this, &ParticleCounterDatabase::slot_receivedHoldingRegisterData);
    connect(m_pcModbusSystem, &ParticleCounterModbusSystem::signal_receivedInputRegisterData, this, &ParticleCounterDatabase::slot_receivedInputRegisterData);
    connect(m_pcModbusSystem, &ParticleCounterModbusSystem::signal_transactionLost, this, &ParticleCounterDatabase::slot_transactionLost);

    // Timer for cyclic poll task to get the status of Particle Counters
    connect(&m_timer_pollStatus, &QTimer::timeout, this, &ParticleCounterDatabase::slot_timer_pollStatus_fired);
    m_timer_pollStatus.setInterval(2000);
    m_timer_pollStatus.start();

    // Timer for cyclic check of the particle counter's realtime clock settings
    connect(&m_timer_checkRealTimeClocks, &QTimer::timeout, this, &ParticleCounterDatabase::slot_timer_checkRealTimeClocks_fired);
    m_timer_checkRealTimeClocks.setInterval(3600000 * 12);  // Every 12 hours RTC of particle counters are set to Server UTC Clock.
}

void ParticleCounterDatabase::loadFromHdd()
{
    QString directory = "/var/openffucontrol/particlecounters/";
    QDirIterator iterator(directory, QStringList() << "*.csv", QDir::Files, QDirIterator::NoIteratorFlags);

    QStringList filepaths;

    while(iterator.hasNext())
    {
        filepaths.append(iterator.next());
    }

    filepaths.sort();

    foreach(QString filepath, filepaths)
    {
        ParticleCounter* newPc = new ParticleCounter(this, m_pcModbusSystem, m_loghandler);
        newPc->load(filepath);
        newPc->setFiledirectory(directory);
        connect(newPc, &ParticleCounter::signal_ParticleCounterActualDataHasChanged, this, &ParticleCounterDatabase::signal_ParticleCounterActualDataHasChanged);
        connect(newPc, &ParticleCounter::signal_ParticleCounterActualDataHasChanged, this, &ParticleCounterDatabase::slot_ParticleCounterActualDataHasChanged);
        connect(newPc, &ParticleCounter::signal_ParticleCounterArchiveDataReceived, this, &ParticleCounterDatabase::slot_ParticleCounterArchiveDataReceived);
        m_particlecounters.append(newPc);

        newPc->init();
    }
}

void ParticleCounterDatabase::saveToHdd()
{
    QString path = "/var/openffucontrol/particlecounters/";

    foreach (ParticleCounter* pc, m_particlecounters)
    {
        pc->setFiledirectory(path);
        pc->save();
    }
}

QList<ModBus *> *ParticleCounterDatabase::getBusList()
{
    return m_pcModbusList;
}

QString ParticleCounterDatabase::addParticleCounter(int id, int busID, int modbusAddress)
{
    ParticleCounter* newPc = new ParticleCounter(this, m_pcModbusSystem, m_loghandler);
    newPc->setFiledirectory("/var/openffucontrol/ocufans/");
    newPc->setAutoSave(false);
    newPc->setId(id);
    newPc->setBusID(busID);
    newPc->setModbusAddress(modbusAddress);
    newPc->setAutoSave(true);
    newPc->save();
    connect(newPc, &ParticleCounter::signal_ParticleCounterActualDataHasChanged, this, &ParticleCounterDatabase::signal_ParticleCounterActualDataHasChanged);
    connect(newPc, &ParticleCounter::signal_ParticleCounterActualDataHasChanged, this, &ParticleCounterDatabase::slot_ParticleCounterActualDataHasChanged);
    connect(newPc, &ParticleCounter::signal_ParticleCounterArchiveDataReceived, this, &ParticleCounterDatabase::slot_ParticleCounterArchiveDataReceived);
    m_particlecounters.append(newPc);

    newPc->init();

    return "OK[ParticleCounterDatabase]: Added ID " + QString().setNum(id);
}

QString ParticleCounterDatabase::deleteParticleCounter(int id)
{
    ParticleCounter* pc = getParticleCounterByID(id);
    if (pc == nullptr)
        return "Warning[ParticleCounterDatabase]: ID " + QString().setNum(id) + " not found.";

    bool ok = m_particlecounters.removeOne(pc);
    if (ok)
    {
        disconnect(pc, &ParticleCounter::signal_ParticleCounterActualDataHasChanged, this, &ParticleCounterDatabase::signal_ParticleCounterActualDataHasChanged);
        disconnect(pc, &ParticleCounter::signal_ParticleCounterActualDataHasChanged, this, &ParticleCounterDatabase::slot_ParticleCounterActualDataHasChanged);
        disconnect(pc, &ParticleCounter::signal_ParticleCounterArchiveDataReceived, this, &ParticleCounterDatabase::slot_ParticleCounterArchiveDataReceived);
        pc->deleteFromHdd();
        pc->deleteAllErrors();
        delete pc;
        return "OK[ParticleCounterDatabase]: Removed ID " + QString().setNum(id);
    }
    return "Warning[ParticleCounterDatabase]: Unable to remove ID " + QString().setNum(id) + " from db.";
}

QList<ParticleCounter *> ParticleCounterDatabase::getParticleCounters(int busNr)
{
    QList<ParticleCounter *> pcList;
    if (busNr == -1)
        return m_particlecounters;
    else
    {
        foreach (ParticleCounter* pc, m_particlecounters) {
            if (pc->getBusID() == busNr)
                pcList.append(pc);
        }
    }

    return pcList;
}

ParticleCounter *ParticleCounterDatabase::getParticleCounterByID(int id)
{
    foreach (ParticleCounter* pc, m_particlecounters)
    {
        if (pc->getId() == id)
            return pc;
    }
    return nullptr;    // Not found
}

QString ParticleCounterDatabase::getParticleCounterData(int id, QString key)
{
    ParticleCounter* pc = getParticleCounterByID(id);
    if (pc == nullptr)
        return "Warning[ParticleCounterDatabase]: ID " + QString().setNum(id) + " not found.";

    return pc->getData(key);
}

QMap<QString, QString> ParticleCounterDatabase::getParticleCounterData(int id, QStringList keys)
{
    QMap<QString,QString> response;

    ParticleCounter* pc = getParticleCounterByID(id);
    if (pc == nullptr)
    {
        return response;
    }

    if (keys.contains("actual"))
    {
        keys.clear();   // Only show actual values, drop all other requests because the answer goes into a special processing later
        keys.append(pc->getActualKeys());
        response.insert("actualData", "1"); // And show the recipient that this is actualData
    }

    foreach (QString key, keys)
    {
        response.insert(key, pc->getData(key));
    }

    return response;
}

QString ParticleCounterDatabase::setParticleCounterData(int id, QString key, QString value)
{
    ParticleCounter* pc = getParticleCounterByID(id);
    if (pc == nullptr)
        return "Warning[ParticleCounterDatabase]: ID " + QString().setNum(id) + " not found.";

    pc->setData(key, value);
    return "OK[ParticleCounterDatabase]: Setting " + key + " to " + value;
}

QString ParticleCounterDatabase::setParticleCounterData(int id, QMap<QString, QString> dataMap)
{
    ParticleCounter* pc = getParticleCounterByID(id);
    if (pc == nullptr)
        return "Warning[ParticleCounterDatabase]: ID " + QString().setNum(id) + " not found.";

    QString dataString;

    foreach(QString key, dataMap.keys())
    {
        QString value = dataMap.value(key);
        pc->setData(key, value);

        dataString.append(" " + key + ":" + value);
    }

    return "OK[ParticleCounterDatabase]: Setting data:" + dataString;
}

ParticleCounter *ParticleCounterDatabase::getParticleCounterByTelegramID(quint64 telegramID)
{
    foreach (ParticleCounter* pc, m_particlecounters) {
        bool found = pc->isThisYourTelegram(telegramID);
        if (found)
        {
            return pc;
        }
    }

    return nullptr;    // TransactionID not initiated by pc requests, so it came frome somebody else
}

void ParticleCounterDatabase::slot_transactionFinished()
{
    // Do nothing
}

void ParticleCounterDatabase::slot_transactionLost(quint64 telegramID)
{
    ParticleCounter* pc = getParticleCounterByTelegramID(telegramID);
    if (pc == nullptr)
    {
        // Somebody other than the pc requested that response, so do nothing with the response at this point
        m_loghandler->slot_newEntry(LogEntry::Error, "ParticleCounterDatabase slot_transactionLost", "Telegram id mismatch.");
        return;
    }
    pc->slot_transactionLost(telegramID);
}

void ParticleCounterDatabase::slot_receivedHoldingRegisterData(quint64 telegramID, quint16 adr, quint16 reg, QList<quint16> data)
{
    ParticleCounter* pc = getParticleCounterByTelegramID(telegramID);
    if (pc == nullptr)
    {
        // Somebody other than the pc requested that response, so do nothing with the response at this point
        m_loghandler->slot_newEntry(LogEntry::Error, "ParticleCounterDatabase slot_receivedHoldingRegisterData", "Telegram id mismatch.");
        return;
    }
    pc->slot_receivedHoldingRegisterData(telegramID, adr, reg, data);
}

void ParticleCounterDatabase::slot_receivedInputRegisterData(quint64 telegramID, quint16 adr, quint16 reg, QList<quint16> data)
{
    ParticleCounter* pc = getParticleCounterByTelegramID(telegramID);
    if (pc == nullptr)
    {
        // Somebody other than the pc requested that response, so do nothing with the response at this point
        m_loghandler->slot_newEntry(LogEntry::Error, "ParticleCounterDatabase slot_receivedInputRegisterData", "Telegram id mismatch.");
        return;
    }
    pc->slot_receivedInputRegisterData(telegramID, adr, reg, data);
}

void ParticleCounterDatabase::slot_ParticleCounterActualDataHasChanged(int id)
{
    // Example of payload:
    // 'particles,tag_id=2,tag_channel=1,tag_room=iso5-Raum id=2i,channel=1i,counts=15i 1678388136783721259'

    QMap<QString,QString> responseData = this->getParticleCounterData(id, QStringList() << "actual");
    responseData.remove("actualData");  // Remove special treatment marker

    QString measurementName = m_settings->value("measurementName", QString()).toString();

    QByteArray payload;
    payload.append(measurementName.toUtf8() + ",");
    payload.append("tag_id=" + QByteArray().setNum(id) + ",");
    payload.append("tag_channel=" + responseData.value("channel").toUtf8());
    payload.append("tag_room=" + responseData.value("room").toUtf8());
    payload.append(" ");
    payload.append("id=" + QByteArray().setNum(id) + "i,");
    payload.append("channel=" + responseData.value("channel").toUtf8() + "i,");
    payload.append("counts=" + responseData.value("counts").toUtf8() + "i ");
    payload.append(responseData.value("timestamp").toUtf8());
    m_influxDB->write(payload);
}

void ParticleCounterDatabase::slot_ParticleCounterArchiveDataReceived(int id, ParticleCounter::ArchiveDataset archiveData)
{
    // Example of payload:
    // 'particles,tag_id=2,tag_channel=1,tag_room=iso5-Raum id=2i,channel=1i,counts=15i 1678388136783721259'

    QString measurementName = m_settings->value("measurementName", QString()).toString();

    // Separate data points in influx for each channel of the particle counter
    for (int ch=0; ch<8; ch++)
    {
        if (archiveData.channelData[ch].status == ParticleCounter::OFF)
            continue;

        QByteArray payload;
        payload.append(measurementName.toUtf8() + ",");
        payload.append("tag_id=" + QByteArray().setNum(id) + ",");
        payload.append("tag_channel=" + QByteArray().setNum(archiveData.channelData[ch].channel) + ",");
        // tbd!!
        //    payload.append("tag_room=" + responseData.value("room").toUtf8());
        payload.append(" ");
        payload.append("id=" + QByteArray().setNum(id) + "i,");
        payload.append("channel=" + QByteArray().setNum(archiveData.channelData[ch].channel) + "i,");
        payload.append("counts=" + QByteArray().setNum(archiveData.channelData[ch].count) + "i ");
        payload.append(archiveData.timestamp.toMSecsSinceEpoch() * 1000000);    // Write timestamp to influx in nanoseconds since epoch
        m_influxDB->write(payload);
    }
}

void ParticleCounterDatabase::slot_timer_pollStatus_fired()
{
    foreach (ModBus* modBus, *m_pcModbusList)
    {
        int sizeOfTelegramQueue = qMax(modBus->getSizeOfTelegramQueue(false), modBus->getSizeOfTelegramQueue(true));
        if (sizeOfTelegramQueue < 20)
        {
            foreach(ParticleCounter* pc, m_particlecounters)
            {
                if (m_pcModbusList->indexOf(modBus) == pc->getBusID())
                {
                    pc->requestStatus();
                    pc->requestArchiveDataset();
                    pc->requestNextArchive();
                }
            }
        }
    }
}

void ParticleCounterDatabase::slot_timer_checkRealTimeClocks_fired()
{
    foreach (ModBus* modBus, *m_pcModbusList)
    {
        foreach(ParticleCounter* pc, m_particlecounters)
        {
            if (m_pcModbusList->indexOf(modBus) == pc->getBusID())
            {
                pc->setClock();
                // Later maybe just read clock and compare it in order to write it only if necessary
            }
        }
    }
}
