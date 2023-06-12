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

#include <stdio.h>
#include "maincontroller.h"

MainController::MainController(QObject *parent) : QObject(parent)
{
    fprintf(stdout, "ebmBus main controller startup...\n");

    m_settings = new QSettings("/etc/openffucontrol/particleserver/config.ini", QSettings::IniFormat);

    m_loghandler = new Loghandler(this);
    connect(m_loghandler, &Loghandler::signal_newError, this, &MainController::slot_newError);
    connect(m_loghandler, &Loghandler::signal_allErrorsQuit, this, &MainController::slot_allErrorsQuit);
    connect(m_loghandler, &Loghandler::signal_allErrorsGone, this, &MainController::slot_allErrorsGone);

    m_influxDB = new InfluxDB(this, m_loghandler);

    m_pcModbusSystem = new ParticleCounterModbusSystem(this, m_loghandler);

    m_pcDatabase = new ParticleCounterDatabase(this, m_pcModbusSystem, m_influxDB, m_loghandler);
    m_pcDatabase->loadFromHdd();

    m_remotecontroller = new RemoteController(this, m_pcDatabase, m_loghandler);
    connect(m_remotecontroller, &RemoteController::signal_connected, this, &MainController::slot_remoteControlConnected);
    connect(m_remotecontroller, &RemoteController::signal_disconnected, this, &MainController::slot_remoteControlDisconnected);
}

MainController::~MainController()
{
}

// This slot is called as soon as the first server connects to the remotecontroller
void MainController::slot_remoteControlConnected()
{
}

// This slot is called if the remotecontroller is not connected to at least ONE server
void MainController::slot_remoteControlDisconnected()
{
}

// This slot is called if the errorhandler gets a new error, that we want to show the operator by a blinking red led
void MainController::slot_newError()
{
}

void MainController::slot_allErrorsQuit()
{
}

void MainController::slot_allErrorsGone()
{
}
