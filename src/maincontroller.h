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

#ifndef MAINCONTROLLER_H
#define MAINCONTROLLER_H

#include <QObject>
#include <QTimer>
#include <QList>
#include <QSettings>
#include <libebmbus/ebmbus.h>
#include <libopenffucontrol-qtmodbus/modbus.h>
#include "particlecountermodbussystem.h"
#include "particlecounterdatabase.h"
#include "remotecontroller.h"
#include "influxdb.h"
#include "loghandler.h"

class MainController : public QObject
{
    Q_OBJECT

public:
    explicit MainController(QObject *parent = nullptr);
    ~MainController();

private:
    Loghandler* m_loghandler;
    QSettings* m_settings;

    ParticleCounterModbusSystem* m_pcModbusSystem;

    ParticleCounterDatabase* m_pcDatabase;

    RemoteController* m_remotecontroller;

    InfluxDB* m_influxDB;

    QTimer m_timer;

private slots:
    void slot_remoteControlConnected();
    void slot_remoteControlDisconnected();

    void slot_newError();
    void slot_allErrorsQuit();
    void slot_allErrorsGone();
};

#endif // MAINCONTROLLER_H
