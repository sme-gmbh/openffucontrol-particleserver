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

#include "particlecountermodbussystem.h"

ParticleCounterModbusSystem::ParticleCounterModbusSystem(QObject *parent, Loghandler *loghandler)
{
    m_loghandler = loghandler;

    QSettings settings("/etc/openffucontrol/particleserver/config.ini", QSettings::IniFormat);
    settings.beginGroup("interfacesParticleCounterModBus");

    QStringList interfaceKeyList = settings.childKeys();

    foreach(QString interfacesKey, interfaceKeyList)
    {
        if (!interfacesKey.startsWith("pcmodbus"))
            continue;
        QString interfacesString = settings.value(interfacesKey).toString();
        QStringList interfaces = interfacesString.split(",", QString::SkipEmptyParts);

        if (interfaces.length() == 1)       // Non redundant bus
        {
            QString interface_0 = interfaces.at(0);

#ifdef QT_DEBUG
            ModBus* newModbus = new ModBus(nullptr, QString("/dev/").append(interface_0), true);    // parent must be 0 in order to be moved to workerThread later
#else
            ModBus* newModbus = new ModBus(nullptr, QString("/dev/").append(interface_0), false);    // parent must be 0 in order to be moved to workerThread later
#endif
            m_pcModbuslist.append(newModbus);

            connect(this, &ParticleCounterModbusSystem::signal_newEntry, m_loghandler, &Loghandler::slot_newEntry);
            connect(this, &ParticleCounterModbusSystem::signal_entryGone, m_loghandler, &Loghandler::slot_entryGone);

            // Routing of bus results to master
            connect(newModbus, &ModBus::signal_transactionFinished, this, &ParticleCounterModbusSystem::slot_transactionFinished);
            connect(newModbus, &ModBus::signal_transactionFinished, this, &ParticleCounterModbusSystem::signal_transactionFinished);
            connect(newModbus, &ModBus::signal_transactionLost, this, &ParticleCounterModbusSystem::slot_transactionLost);
            connect(newModbus, &ModBus::signal_transactionLost, this, &ParticleCounterModbusSystem::signal_transactionLost);
            connect(newModbus, &ModBus::signal_responseRaw, this, &ParticleCounterModbusSystem::slot_responseRaw);
            connect(newModbus, &ModBus::signal_holdingRegistersRead, this, &ParticleCounterModbusSystem::slot_holdingRegistersRead);
            connect(newModbus, &ModBus::signal_inputRegistersRead, this, &ParticleCounterModbusSystem::slot_inputRegistersRead);

            quint32 txDelay = settings.value("txDelay", 200).toUInt();
            newModbus->setDelayTxTimer(txDelay);

            if (!newModbus->open(QSerialPort::Baud19200, QSerialPort::Data8, QSerialPort::EvenParity, QSerialPort::OneStop))
                fprintf(stderr, "OcuModbusSystem::OcuModbusSystem(): Unable to open serial line %s!\n", interface_0.toUtf8().data());
            else
                fprintf(stderr, "OcuModbusSystem::OcuModbusSystem(): Activated on %s!\n", interface_0.toUtf8().data());
            fflush(stderr);
        }
        else if (interfaces.length() == 2)  // Redundant bus
        {
            QString interface_0 = interfaces.at(0);
            QString interface_1 = interfaces.at(1);
            // Not implemented yet
        }
    }

}

ParticleCounterModbusSystem::~ParticleCounterModbusSystem()
{

}

QList<ModBus *> *ParticleCounterModbusSystem::pcModbuslist()
{
    return (&m_pcModbuslist);
}

ModBus *ParticleCounterModbusSystem::getBusByID(int busID)
{
    if (m_pcModbuslist.length() <= busID)
        return nullptr; // Bus id not available

    ModBus* bus = m_pcModbuslist.at(busID);

    return bus;
}

void ParticleCounterModbusSystem::slot_responseRaw(quint64 telegramID, quint8 address, quint8 functionCode, QByteArray data)
{
#ifdef QT_DEBUG
    printf("ID: %llu ADR: %02X  FC: %02X data: ", telegramID, address, functionCode);
    foreach (quint8 byte, data)
    {
        printf("%02X ", byte);
    }
    printf("\n");
    fflush(stdout);
#else
    Q_UNUSED(telegramID)
    Q_UNUSED(address)
    Q_UNUSED(functionCode)
    Q_UNUSED(data)
#endif
}

void ParticleCounterModbusSystem::slot_transactionLost(quint64 telegramID)
{
    emit signal_newEntry(LogEntry::Info, "OcuModbusSystem", QString("Transaction lost."));

#ifdef QT_DEBUG
    printf("ID: %llu Transaction lost.\n", telegramID);
    fflush(stdout);
#else
    Q_UNUSED(telegramID)
#endif
}

void ParticleCounterModbusSystem::slot_transactionFinished()
{
#ifdef QT_DEBUG
    printf("Transaction finished.\n");
    fflush(stdout);
#endif
}

void ParticleCounterModbusSystem::slot_holdingRegistersRead(quint64 telegramID, quint8 slaveAddress, quint16 dataStartAddress, QList<quint16> data)
{
    emit signal_receivedHoldingRegisterData(telegramID, slaveAddress, dataStartAddress, data);

}

void ParticleCounterModbusSystem::slot_inputRegistersRead(quint64 telegramID, quint8 slaveAddress, quint16 dataStartAddress, QList<quint16> data)
{
    emit signal_receivedInputRegisterData(telegramID, slaveAddress, dataStartAddress, data);

}
