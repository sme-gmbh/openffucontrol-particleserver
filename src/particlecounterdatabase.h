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

#ifndef PARTICLECOUNTERDATABASE_H
#define PARTICLECOUNTERDATABASE_H

#include <QObject>
#include <QStringList>
#include <QList>
#include <QVariant>
#include <QMap>
#include <QDir>
#include <QDirIterator>
#include <QTimer>
#include <QMap>
#include <QSettings>
#include "particlecountermodbussystem.h"
#include "loghandler.h"
#include "particlecounter.h"
#include "influxdb.h"


class ParticleCounterDatabase : public QObject
{
    Q_OBJECT
public:
    explicit ParticleCounterDatabase(QObject *parent, ParticleCounterModbusSystem *pcModbusSystem, InfluxDB *influxDB, Loghandler *loghandler);

    void loadFromHdd();
    void saveToHdd();

    QList<ModBus *> *getBusList();

    QString addParticleCounter(int id, int busID, int modbusAddress);
    QString deleteParticleCounter(int id);

    QList<ParticleCounter*> getParticleCounters(int busNr = -1);    // If busNr is specified only OCUs of that bus are returned
    ParticleCounter* getParticleCounterByID(int id);

    QString getParticleCounterData(int id, QString key);
    QMap<QString,QString> getParticleCounterData(int id, QStringList keys);
    QString setParticleCounterData(int id, QString key, QString value);
    QString setParticleCounterData(int id, QMap<QString,QString> dataMap);

    // Broadcast is not implemented yet
    //QString broadcast(int busID, QMap<QString,QString> dataMap);



private:
    QSettings* m_settings;
    ParticleCounterModbusSystem* m_pcModbusSystem;
    QList<ModBus*>* m_pcModbusList;
    InfluxDB* m_influxDB;
    Loghandler* m_loghandler;
    QList<ParticleCounter*> m_particlecounters;
    QTimer m_timer_pollStatus;
    QTimer m_timer_checkRealTimeClocks;

    ParticleCounter* getParticleCounterByTelegramID(quint64 telegramID);

signals:
    void signal_ParticleCounterActualDataHasChanged(int id);

public slots:

private slots:
    // High level bus response slots
    void slot_transactionFinished();
    void slot_transactionLost(quint64 telegramID);
    void slot_receivedHoldingRegisterData(quint64 telegramID, quint16 adr, quint16 reg, QList<quint16> data);
    void slot_receivedInputRegisterData(quint64 telegramID, quint16 adr, quint16 reg, QList<quint16> data);

    void slot_ParticleCounterActualDataReceived(int id, ParticleCounter::ActualData actualData);
    void slot_ParticleCounterArchiveDataReceived(int id, ParticleCounter::ArchiveDataset archiveData);
    // Timer slots
    void slot_timer_pollStatus_fired();
    void slot_timer_checkRealTimeClocks_fired();
};

#endif // PARTICLECOUNTERDATABASE_H
