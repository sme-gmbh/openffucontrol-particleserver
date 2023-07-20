#**********************************************************************
#* openffucontrol-particleserver - a daemon for data acquisition from
#* cleanroom particle monitoring devices into an influx time-series database
#* Copyright (C) 2023 Smart Micro Engineering GmbH
#* This program is free software: you can redistribute it and/or modify
#* it under the terms of the GNU General Public License as published by
#* the Free Software Foundation, either version 3 of the License, or
#* (at your option) any later version.
#* This program is distributed in the hope that it will be useful,
#* but WITHOUT ANY WARRANTY; without even the implied warranty of
#* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
#* GNU General Public License for more details.
#* You should have received a copy of the GNU General Public License
#* along with this program. If not, see <http://www.gnu.org/licenses/>.
#*********************************************************************/

QT += core network
QT -= gui

TARGET = openffucontrol-particleserver

CONFIG += c++11 console
CONFIG -= app_bundle

TEMPLATE = app

OBJECTS_DIR = .obj/
MOC_DIR = .moc/
UI_DIR = .ui/
RCC_DIR = .rcc/

# make install / make uninstall target
unix {
    target.path = /usr/bin/
    systemdfiles.files += ../unix/openffucontrol-particleserver.service
    systemdfiles.path = /lib/systemd/system/
    etcfiles.files += ../etc/config.ini.example
    etcfiles.path = /etc/openffucontrol/particleserver/
    INSTALLS += target
    INSTALLS += systemdfiles
    INSTALLS += etcfiles
}

SOURCES += \
        influxdb.cpp \
        logentry.cpp \
        loghandler.cpp \
        main.cpp \
        maincontroller.cpp \
        particlecounter.cpp \
        particlecounterdatabase.cpp \
        particlecountermodbussystem.cpp \
        remoteclienthandler.cpp \
        remotecontroller.cpp

LIBS     += -lopenffucontrol-qtmodbus

HEADERS += \
    influxdb.h \
    logentry.h \
    loghandler.h \
    maincontroller.h \
    particlecounter.h \
    particlecounterdatabase.h \
    particlecountermodbussystem.h \
    remoteclienthandler.h \
    remotecontroller.h

DISTFILES += \
    ../etc/config.ini.example \
    ../unix/openffucontrol-particleserver.service

linux-g++: QMAKE_TARGET.arch = $$QMAKE_HOST.arch
linux-g++-32: QMAKE_TARGET.arch = x86
linux-g++-64: QMAKE_TARGET.arch = x86_64
linux-cross: QMAKE_TARGET.arch = x86
win32-cross-32: QMAKE_TARGET.arch = x86
win32-cross: QMAKE_TARGET.arch = x86_64
win32-g++: QMAKE_TARGET.arch = $$QMAKE_HOST.arch
win32-msvc*: QMAKE_TARGET.arch = $$QMAKE_HOST.arch
linux-raspi: QMAKE_TARGET.arch = armv6l
linux-armv6l: QMAKE_TARGET.arch = armv6l
linux-armv7l: QMAKE_TARGET.arch = armv7l
linux-arm*: QMAKE_TARGET.arch = armv6l
linux-aarch64*: QMAKE_TARGET.arch = aarch64

unix {
    equals(QMAKE_TARGET.arch , x86_64): {
        message("Configured for x86_64")
    }

    equals(QMAKE_TARGET.arch , x86): {
        message("Configured for x86")
    }

    equals(QMAKE_TARGET.arch , armv6l): {
        message("Configured for armv6l")
    }

    equals(QMAKE_TARGET.arch , armv7l): {
        message("Configured for armv7l")
    }
}

