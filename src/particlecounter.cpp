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

#include <QFile>
#include <QString>
#include <QStringList>
#include <QDir>
#include "particlecounter.h"

ParticleCounter::ParticleCounter(QObject *parent, ParticleCounterModbusSystem *pcModbusSystem, Loghandler* loghandler) : QObject(parent)
{
    m_pcModbusSystem = pcModbusSystem;
    m_loghandler = loghandler;

    m_dataChanged = false;
    setAutoSave(false);

    m_id = -1;

    m_busID = -1;         // Invalid bus
    m_modbusAddress = -1; // Invalid address

    m_samplingEnabled = false;

    m_actualData.clockSettingLostCount = 0;

    for (int i=0; i<8; i++)
    {
        m_actualData.channelData[i].channel = i + 1;
        m_actualData.channelData[i].status = OFF;
        m_actualData.channelData[i].count = 0;
    }

    m_actualData.lastSeen = QDateTime();
    m_actualData.lostTelegrams = 0;
    m_actualData.statusString = QString();
    m_actualData.timestamp = QDateTime();

    m_configData.outputDataFormat = CUMULATIVE;
    m_configData.addupCount = 1;
    m_configData.firstRinsingTimeInSeconds = 60;
    m_configData.subsequentRinsingTimeInSeconds = 30; // Must be >= 1 for internal loop check!
    m_configData.samplingTimeInSeconds = 59;
    m_configData.valid = true;

    m_deviceInfo.deviceInfoString = QString();
    m_deviceInfo.deviceIdString = QString();
    m_deviceInfo.modbusRegistersetVersion = QString();

    m_statusRegister.deviceActive = false;
    m_statusRegister.currentlySampling = false;
    m_statusRegister.currentlyRinsing = false;
    m_statusRegister.dataReady = false;

    m_errorstateRegister.temperatureError = false;
    m_errorstateRegister.sdCardError = false;
    m_errorstateRegister.counterSettings = false;
    m_errorstateRegister.acquisitionSettings = false;
    m_errorstateRegister.remoteSettings = false;
    m_errorstateRegister.filterSettings = false;
    m_errorstateRegister.detectorLoop = false;
    m_errorstateRegister.laserError = false;
    m_errorstateRegister.flowError = false;
}

ParticleCounter::~ParticleCounter()
{

}

int ParticleCounter::getId() const
{
    return m_id;
}

void ParticleCounter::setId(int id)
{
    if (id != m_id)
    {
        m_id = id;
        m_dataChanged = true;
        emit signal_needsSaving();
    }
}

int ParticleCounter::getBusID() const
{
    return m_busID;
}

void ParticleCounter::setBusID(int busID)
{
    if (busID != m_busID)
    {
        m_busID = busID;
        m_dataChanged = true;
        emit signal_needsSaving();
    }
}

int ParticleCounter::getModbusAddress() const
{
    return m_modbusAddress;
}

void ParticleCounter::setModbusAddress(int modbusAddress)
{
    if (m_modbusAddress != modbusAddress)
    {
        m_modbusAddress = modbusAddress;
        m_dataChanged = true;
        emit signal_needsSaving();
    }
}

void ParticleCounter::init()
{
    this->setClock();
    this->setConfigData(m_configData);
    this->requestDeviceInfo();
    this->setSamplingEnabled(true);
    this->storeSettingsToFlash();
    this->requestStatus();
}

QString ParticleCounter::getData(QString key)
{
    // ***** Static keys *****
    if (key == "id")
    {
        return (QString().setNum(m_id));
    }
    else if (key == "busID")
    {
        return (QString().setNum(getBusID()));
    }
    else if (key == "unit")
    {
        return (QString().setNum(getModbusAddress()));
    }
    // ***** Actual keys *****
    else if (key == "online")
    {
        return QString().sprintf("%i", m_actualData.online);
    }
    else if (key == "lostTelegrams")
    {
        return QString().sprintf("%lli", m_actualData.lostTelegrams);
    }
    else if (key == "lastSeen")
    {
        return m_actualData.lastSeen.toString("yyyy.MM.dd-hh:mm:ss.zzz");
    }
    else if (key == "clockSettingLostCount")
    {
        return QString().sprintf("%i", m_actualData.clockSettingLostCount);
    }
    else if (key == "deviceInfo")
    {
        return ("\"" + m_deviceInfo.deviceInfoString + "\"");
    }
    else if (key == "deviceID")
    {
        return ("\"" + m_deviceInfo.deviceIdString + "\"");
    }
    else if (key == "modbusRegistersetVersion")
    {
        return ("\"" + m_deviceInfo.modbusRegistersetVersion + "\"");
    }
    else if (key == "errorstring")
    {
        QString errorstring;

        if (m_errorstateRegister.temperatureError)
            errorstring += "error_temperatureError=1_";
        if (m_errorstateRegister.sdCardError)
            errorstring += "error_sdCardError=1_";
        if (m_errorstateRegister.counterSettings)
            errorstring += "error_counterSettings=1_";
        if (m_errorstateRegister.acquisitionSettings)
            errorstring += "error_acquisitionSettings=1_";
        if (m_errorstateRegister.remoteSettings)
            errorstring += "error_remoteSettings=1_";
        if (m_errorstateRegister.filterSettings)
            errorstring += "error_filterSettings=1_";
        if (m_errorstateRegister.detectorLoop)
            errorstring += "error_detectorLoop=1_";
        if (m_errorstateRegister.laserError)
            errorstring += "error_laserError=1_";
        if (m_errorstateRegister.flowError)
            errorstring += "error_flowError=1_";

        if (errorstring.isEmpty())
            errorstring = "noError";

        return errorstring;
    }

    return "Error[Particle Counter]: Key " + key + " not available";
}

void ParticleCounter::setData(QString key, QString value)
{
    if (key == "busID")
    {
        setBusID(value.toInt());
    }
    else if (key == "unit")
    {
        setModbusAddress(value.toInt());
    }
}

void ParticleCounter::setSamplingEnabled(bool on)
{
    if (!isConfigured())
    {
        m_loghandler->slot_newEntry(LogEntry::Error, "Particle Counter id=" + QString().setNum(m_id), " not configured.");
        return;
    }

    ModBus* bus = m_pcModbusSystem->getBusByID(m_busID);
    if (bus == nullptr)
    {
        m_loghandler->slot_newEntry(LogEntry::Error, "Particle Counter id=" + QString().setNum(m_id), " Bus id " + QString().setNum(m_busID) + " not found.");
        return;
    }

    if (!m_configData.valid)
        requestConfig();

    m_samplingEnabled = on;
}

bool ParticleCounter::isSampling() const
{
    return m_samplingEnabled;
}

void ParticleCounter::storeSettingsToFlash()
{
    if (!isConfigured())
    {
        m_loghandler->slot_newEntry(LogEntry::Error, "Particle Counter id=" + QString().setNum(m_id), " not configured.");
        return;
    }

    ModBus* bus = m_pcModbusSystem->getBusByID(m_busID);
    if (bus == nullptr)
    {
        m_loghandler->slot_newEntry(LogEntry::Error, "Particle Counter id=" + QString().setNum(m_id), " Bus id " + QString().setNum(m_busID) + " not found.");
        return;
    }

    if (!m_configData.valid)
        requestConfig();

    m_transactionIDs.append(bus->writeSingleRegister(m_modbusAddress, ParticleCounter::HOLDING_REG_0100_Command, ParticleCounter::COMMAND_0009_SaveAcquisitionRegistersToNonvolatileMemory));
}

QStringList ParticleCounter::getActualKeys()
{
    QStringList keys;

    keys += "online";
    keys += "lostTelegrams";
    keys += "lastSeen";
    keys += "clockSettingLostCount";
    keys += "statusString";
    keys += "countChannel_1";
    keys += "countChannel_2";
    keys += "countChannel_3";
    keys += "countChannel_4";
    keys += "countChannel_5";
    keys += "countChannel_6";
    keys += "countChannel_7";
    keys += "countChannel_8";
    keys += "timestamp";

    return keys;
}

ParticleCounter::ActualData ParticleCounter::getActualData() const
{
    return m_actualData;
}

void ParticleCounter::requestDeviceInfo()
{
    if (!isConfigured())
    {
        m_loghandler->slot_newEntry(LogEntry::Error, "Particle Counter id=" + QString().setNum(m_id), " not configured.");
        return;
    }

    ModBus* bus = m_pcModbusSystem->getBusByID(m_busID);
    if (bus == nullptr)
    {
        m_loghandler->slot_newEntry(LogEntry::Error, "Particle Counter id=" + QString().setNum(m_id), " Bus id " + QString().setNum(m_busID) + " not found.");
        return;
    }

    m_transactionIDs.append(bus->readInputRegisters(m_modbusAddress, ParticleCounter::INPUT_REG_0001_0048_DeviceInfoString, 48));
    m_transactionIDs.append(bus->readInputRegisters(m_modbusAddress, ParticleCounter::INPUT_REG_0065_0080_DeviceIDString, 16));
    m_transactionIDs.append(bus->readInputRegisters(m_modbusAddress, ParticleCounter::INPUT_REG_0082_ModbusRegistersetVersion, 1));
}

void ParticleCounter::requestStatus()
{
    if (!isConfigured())
    {
        m_loghandler->slot_newEntry(LogEntry::Error, "Particle Counter id=" + QString().setNum(m_id), " not configured.");
        return;
    }

    ModBus* bus = m_pcModbusSystem->getBusByID(m_busID);
    if (bus == nullptr)
    {
        m_loghandler->slot_newEntry(LogEntry::Error, "Particle Counter id=" + QString().setNum(m_id), " Bus id " + QString().setNum(m_busID) + " not found.");
        return;
    }

    if (!m_configData.valid)
        requestConfig();

    // Always set sampling enabled or disabled in every status request as stupid counters forget that sometimes and stop working
    if (m_samplingEnabled)
    {
        m_transactionIDs.append(bus->writeSingleRegister(m_modbusAddress, ParticleCounter::HOLDING_REG_0100_Command, ParticleCounter::COMMAND_0017_StartAcquisition));
    }
    else
    {
        m_transactionIDs.append(bus->writeSingleRegister(m_modbusAddress, ParticleCounter::HOLDING_REG_0100_Command, ParticleCounter::COMMAND_0016_StopAcquisition));
    }

    m_transactionIDs.append(bus->readInputRegisters(m_modbusAddress, ParticleCounter::INPUT_REG_0089_StatusRegister, 1));
    m_transactionIDs.append(bus->readInputRegisters(m_modbusAddress, ParticleCounter::INPUT_REG_0096_ErrorstateRegister, 1));
    m_transactionIDs.append(bus->readInputRegisters(m_modbusAddress, ParticleCounter::INPUT_REG_0097_0112_PhysicalUnitString, 16));
//    m_transactionIDs.append(bus->readInputRegisters(m_modbusAddress, ParticleCounter::INPUT_REG_0257_LivecountsTimestampSeconds,
//                                                      ParticleCounter::INPUT_REG_0285_0286_LivecountsChannel8LH + 1 - ParticleCounter::INPUT_REG_0257_LivecountsTimestampSeconds + 1));
}

void ParticleCounter::requestArchiveDataset()
{
    if (!isConfigured())
    {
        m_loghandler->slot_newEntry(LogEntry::Error, "Particle Counter id=" + QString().setNum(m_id), " not configured.");
        return;
    }

    ModBus* bus = m_pcModbusSystem->getBusByID(m_busID);
    if (bus == nullptr)
    {
        m_loghandler->slot_newEntry(LogEntry::Error, "Particle Counter id=" + QString().setNum(m_id), " Bus id " + QString().setNum(m_busID) + " not found.");
        return;
    }

    if (!m_configData.valid)
        requestConfig();

    m_transactionIDs.append(bus->readInputRegisters(m_modbusAddress, ParticleCounter::INPUT_REG_0513_ArchiveDataSetTimestampSeconds,
                                                    ParticleCounter::INPUT_REG_0543_0544_ArchiveDataSetChannel8LH + 1 - ParticleCounter::INPUT_REG_0513_ArchiveDataSetTimestampSeconds + 1));
}

void ParticleCounter::requestNextArchive()
{
    if (!isConfigured())
    {
        m_loghandler->slot_newEntry(LogEntry::Error, "Particle Counter id=" + QString().setNum(m_id), " not configured.");
        return;
    }

    ModBus* bus = m_pcModbusSystem->getBusByID(m_busID);
    if (bus == nullptr)
    {
        m_loghandler->slot_newEntry(LogEntry::Error, "Particle Counter id=" + QString().setNum(m_id), " Bus id " + QString().setNum(m_busID) + " not found.");
        return;
    }

    if (!m_configData.valid)
        requestConfig();

    m_transactionIDs.append(bus->writeSingleRegister(m_modbusAddress, ParticleCounter::HOLDING_REG_0100_Command, ParticleCounter::COMMAND_0099_LoadNextArchiveDataSet));
}

void ParticleCounter::requestConfig()
{
    if (!isConfigured())
        return;

    ModBus* bus = m_pcModbusSystem->getBusByID(m_busID);
    if (bus == nullptr)
        return;

    m_transactionIDs.append(bus->readHoldingRegisters(m_modbusAddress, ParticleCounter::HOLDING_REG_0002_OutputDataFormat, 1));
    m_transactionIDs.append(bus->readHoldingRegisters(m_modbusAddress, ParticleCounter::HOLDING_REG_0003_FirstRinsingTimeInSeconds, 1));
    m_transactionIDs.append(bus->readHoldingRegisters(m_modbusAddress, ParticleCounter::HOLDING_REG_0004_SubsequentRinsingTimeInSeconds, 1));
    m_transactionIDs.append(bus->readHoldingRegisters(m_modbusAddress, ParticleCounter::HOLDING_REG_0005_SamplingTimeInSeconds, 1));
}

void ParticleCounter::setConfigData(ConfigData data)
{
    if (!isConfigured())
        return;

    ModBus* bus = m_pcModbusSystem->getBusByID(m_busID);
    if (bus == nullptr)
        return;

    m_transactionIDs.append(bus->writeSingleRegister(m_modbusAddress, ParticleCounter::HOLDING_REG_0002_OutputDataFormat, data.outputDataFormat + ((data.addupCount * 4) & 0xff)));
    m_transactionIDs.append(bus->writeSingleRegister(m_modbusAddress, ParticleCounter::HOLDING_REG_0003_FirstRinsingTimeInSeconds, data.firstRinsingTimeInSeconds));
    m_transactionIDs.append(bus->writeSingleRegister(m_modbusAddress, ParticleCounter::HOLDING_REG_0004_SubsequentRinsingTimeInSeconds, data.subsequentRinsingTimeInSeconds));
    m_transactionIDs.append(bus->writeSingleRegister(m_modbusAddress, ParticleCounter::HOLDING_REG_0005_SamplingTimeInSeconds, data.samplingTimeInSeconds));
}

void ParticleCounter::requestClock()
{
    if (!isConfigured())
        return;

    ModBus* bus = m_pcModbusSystem->getBusByID(m_busID);
    if (bus == nullptr)
        return;

    m_transactionIDs.append(bus->readHoldingRegisters(m_modbusAddress, ParticleCounter::HOLDING_REG_0017_RtcSeconds,
                                                      ParticleCounter::HOLDING_REG_0022_RtcYears - ParticleCounter::HOLDING_REG_0017_RtcSeconds + 1));
}

void ParticleCounter::setClock()
{
    if (!isConfigured())
        return;

    ModBus* bus = m_pcModbusSystem->getBusByID(m_busID);
    if (bus == nullptr)
        return;

    QDateTime dt = QDateTime::currentDateTimeUtc();
    m_transactionIDs.append(bus->writeSingleRegister(m_modbusAddress, ParticleCounter::HOLDING_REG_0017_RtcSeconds, dt.time().second()));
    m_transactionIDs.append(bus->writeSingleRegister(m_modbusAddress, ParticleCounter::HOLDING_REG_0018_RtcMinutes, dt.time().minute()));
    m_transactionIDs.append(bus->writeSingleRegister(m_modbusAddress, ParticleCounter::HOLDING_REG_0019_RtcHours, dt.time().hour()));
    m_transactionIDs.append(bus->writeSingleRegister(m_modbusAddress, ParticleCounter::HOLDING_REG_0020_RtcDays, dt.date().day()));
    m_transactionIDs.append(bus->writeSingleRegister(m_modbusAddress, ParticleCounter::HOLDING_REG_0021_RtcMonths, dt.date().month()));
    m_transactionIDs.append(bus->writeSingleRegister(m_modbusAddress, ParticleCounter::HOLDING_REG_0022_RtcYears, dt.date().year() - 2000));
    m_transactionIDs.append(bus->writeSingleRegister(m_modbusAddress, ParticleCounter::HOLDING_REG_0100_Command, ParticleCounter::COMMAND_0001_SetClock));
}

void ParticleCounter::save()
{
    if (!m_dataChanged)
        return;

    QFile file(myFilename());
    if (!file.open(QIODevice::WriteOnly))
        return;

    QString wdata;

    wdata.append(QString().sprintf("id=%i ", m_id));
    wdata.append(QString().sprintf("bus=%i ", m_busID));
    wdata.append(QString().sprintf("modbusAddress=%i ", m_modbusAddress));
    wdata.append(QString().sprintf("clockSettingLostCount=%i ", m_actualData.clockSettingLostCount));
    wdata.append(QString().sprintf("outputDataFormat=%i ", m_configData.outputDataFormat));
    wdata.append(QString().sprintf("addupCount=%i ", m_configData.addupCount));
    wdata.append(QString().sprintf("firstRinsingTimeInSeconds=%i ", m_configData.firstRinsingTimeInSeconds));
    wdata.append(QString().sprintf("subsequentRinsingTimeInSeconds=%i\n", m_configData.subsequentRinsingTimeInSeconds));
    wdata.append(QString().sprintf("samplingTimeInSeconds=%i\n", m_configData.samplingTimeInSeconds));
    wdata.append(QString().sprintf("samplingEnabled=%i\n", m_samplingEnabled));

    file.write(wdata.toUtf8());

    file.close();
}

void ParticleCounter::setFiledirectory(QString path)
{
    if (!path.endsWith("/"))
        path.append("/");
    m_filepath = path;

    QDir dir;
    dir.mkpath(path);
}

void ParticleCounter::load(QString filename)
{
    QFile file(filename);

    if (!file.open(QIODevice::ReadOnly))
        return;

    QString rdata = QString().fromUtf8(file.readLine());

    QStringList dataList = rdata.split(" ");
    foreach (QString data, dataList)
    {
        QStringList pair = data.split("=");
        if (pair.count() != 2)
            continue;

        QString key = pair.at(0);
        QString value = pair.at(1);

        if (key == "id")
            m_id = value.toInt();

        if (key == "bus")
            m_busID = value.toInt();

        if (key == "modbusAddress")
            m_modbusAddress = value.toInt();

        if (key == "clockSettingLostCount")
        {
            m_actualData.clockSettingLostCount = value.toInt();
        }

        if (key == "outputDataFormat")
        {
            if (value.toUInt() == DISTRIBUTIVE)
                m_configData.outputDataFormat = DISTRIBUTIVE;
            else if (value.toUInt() == CUMULATIVE)
                m_configData.outputDataFormat = CUMULATIVE;
        }

        if (key == "addupCount")
        {
            m_configData.addupCount = value.toInt();
        }

        if (key == "firstRinsingTimeInSeconds")
        {
            m_configData.firstRinsingTimeInSeconds = value.toInt();
        }

        if (key == "subsequentRinsingTimeInSeconds")
        {
            m_configData.subsequentRinsingTimeInSeconds = value.toInt();
        }

        if (key == "samplingTimeInSeconds")
        {
            m_configData.samplingTimeInSeconds = value.toInt();
        }

        if (key == "samplingEnabled")
        {
            m_samplingEnabled = value.toInt();
        }
    }

    file.close();
}

void ParticleCounter::setAutoSave(bool on)
{
    m_autosave = on;
    if (m_autosave)
        connect(this, SIGNAL(signal_needsSaving()), this, SLOT(slot_save()));
    else
        disconnect(this, SIGNAL(signal_needsSaving()), this, SLOT(slot_save()));
}

void ParticleCounter::deleteFromHdd()
{
    QFile file(myFilename());
    file.remove();
}

void ParticleCounter::deleteAllErrors()
{
    m_loghandler->slot_entryGone(LogEntry::Error, "Particle Counter id=" + QString().setNum(m_id), "Not online.");
    m_loghandler->slot_entryGone(LogEntry::Warning, "Particle Counter id=" + QString().setNum(m_id), "Warnings present.");
}

bool ParticleCounter::isThisYourTelegram(quint64 telegramID, bool deleteID)
{
    bool found = m_transactionIDs.contains(telegramID);

    if (found && deleteID)
    {
        m_transactionIDs.removeOne(telegramID);
    }

    return found;
}

QString ParticleCounter::myFilename()
{
    return (m_filepath + QString().sprintf("particlecounter-%06i.csv", m_id));
}

bool ParticleCounter::isConfigured()
{
    bool configured = true;

    if ((m_modbusAddress == -1) || (m_busID == -1))
        configured = false;

    return configured;
}

void ParticleCounter::markAsOnline()
{
    // If we reach this point we are going to parse a telegram for this device, so mark it as online
    if (!m_actualData.online)
    {
        m_loghandler->slot_entryGone(LogEntry::Error, "Particle Counter id=" + QString().setNum(m_id), "Not online.");
        m_actualData.online = true;
    }
    m_actualData.lastSeen = QDateTime::currentDateTime();
}

void ParticleCounter::processConfigData()
{
//    setNmaxFromConfigData();
}

// ************************************************** Bus response handling **************************************************

void ParticleCounter::slot_transactionLost(quint64 id)
{
    Q_UNUSED(id)

    // If the device has a lost telegram, mark it as offline and increment error counter
    m_actualData.lostTelegrams++;
    if (m_actualData.online)
    {
        m_loghandler->slot_newEntry(LogEntry::Error, "Particle Counter id=" + QString().setNum(m_id), "Not online.");
        m_actualData.online = false;
    }
//    emit signal_ParticleCounterActualDataHasChanged(m_id);
}

void ParticleCounter::slot_receivedHoldingRegisterData(quint64 telegramID, quint16 adr, quint16 reg, QList<quint16> data)
{
    Q_UNUSED(telegramID)

    if (adr != m_modbusAddress)
        return;

    markAsOnline();

    QDateTime deviceRTC = QDateTime();
    QTime deviceTime = QTime();
    QDate deviceDate = QDate();
    quint16 seconds = 0;
    quint16 minutes = 0;
    quint16 hours = 0;
    quint16 days = 0;
    quint16 months = 0;
    quint16 years = 0;

    foreach(quint16 rawdata, data)
    {
        switch (reg)
        {
        case ParticleCounter::HOLDING_REG_0001_AlarmEnable:
            break;
        case ParticleCounter::HOLDING_REG_0002_OutputDataFormat:
            if ((rawdata & 0x01) == 1)
                m_configData.outputDataFormat = CUMULATIVE;
            else
                m_configData.outputDataFormat = DISTRIBUTIVE;
            break;
        case ParticleCounter::HOLDING_REG_0003_FirstRinsingTimeInSeconds:
            m_configData.firstRinsingTimeInSeconds = (rawdata & 0xff) >> 2;
            break;
        case ParticleCounter::HOLDING_REG_0004_SubsequentRinsingTimeInSeconds:
            m_configData.subsequentRinsingTimeInSeconds = rawdata;
            break;
        case ParticleCounter::HOLDING_REG_0005_SamplingTimeInSeconds:
            m_configData.samplingTimeInSeconds = rawdata;
            processConfigData();
            break;
        case ParticleCounter::HOLDING_REG_0017_RtcSeconds:
            seconds = rawdata;
            break;
        case ParticleCounter::HOLDING_REG_0018_RtcMinutes:
            minutes = rawdata;
            break;
        case ParticleCounter::HOLDING_REG_0019_RtcHours:
            hours = rawdata;
            deviceTime.setHMS(hours, minutes, seconds);
            break;
        case ParticleCounter::HOLDING_REG_0020_RtcDays:
            days = rawdata;
            break;
        case ParticleCounter::HOLDING_REG_0021_RtcMonths:
            months = rawdata;
            break;
        case ParticleCounter::HOLDING_REG_0022_RtcYears:
            years = rawdata;
            deviceDate.setDate(years, months, days);
            deviceRTC.setDate(deviceDate);
            deviceRTC.setTime(deviceTime);
            break;
        case ParticleCounter::HOLDING_REG_0033_0034_UpperWarningLimitChannel1LH:
            break;
        case ParticleCounter::HOLDING_REG_0035_0036_UpperAlarmLimitChannel1LH:
            break;
        case ParticleCounter::HOLDING_REG_0037_WarningDelayChannel1:
            break;
        case ParticleCounter::HOLDING_REG_0038_AlarmDelayChannel1:
            break;
        case ParticleCounter::HOLDING_REG_0039_0040_UpperWarningLimitChannel2LH:
            break;
        case ParticleCounter::HOLDING_REG_0041_0042_UpperAlarmLimitChannel2LH:
            break;
        case ParticleCounter::HOLDING_REG_0043_WarningDelayChannel2:
            break;
        case ParticleCounter::HOLDING_REG_0044_AlarmDelayChannel2:
            break;
        case ParticleCounter::HOLDING_REG_0045_0046_UpperWarningLimitChannel3LH:
            break;
        case ParticleCounter::HOLDING_REG_0047_0048_UpperAlarmLimitChannel3LH:
            break;
        case ParticleCounter::HOLDING_REG_0049_WarningDelayChannel3:
            break;
        case ParticleCounter::HOLDING_REG_0050_AlarmDelayChannel3:
            break;
        case ParticleCounter::HOLDING_REG_0051_0052_UpperWarningLimitChannel4LH:
            break;
        case ParticleCounter::HOLDING_REG_0053_0054_UpperAlarmLimitChannel4LH:
            break;
        case ParticleCounter::HOLDING_REG_0055_WarningDelayChannel4:
            break;
        case ParticleCounter::HOLDING_REG_0056_AlarmDelayChannel4:
            break;
        case ParticleCounter::HOLDING_REG_0057_0058_UpperWarningLimitChannel5LH:
            break;
        case ParticleCounter::HOLDING_REG_0059_0060_UpperAlarmLimitChannel5LH:
            break;
        case ParticleCounter::HOLDING_REG_0061_WarningDelayChannel5:
            break;
        case ParticleCounter::HOLDING_REG_0062_AlarmDelayChannel5:
            break;
        case ParticleCounter::HOLDING_REG_0063_0064_UpperWarningLimitChannel6LH:
            break;
        case ParticleCounter::HOLDING_REG_0065_0066_UpperAlarmLimitChannel6LH:
            break;
        case ParticleCounter::HOLDING_REG_0067_WarningDelayChannel6:
            break;
        case ParticleCounter::HOLDING_REG_0068_AlarmDelayChannel6:
            break;
        case ParticleCounter::HOLDING_REG_0069_0070_UpperWarningLimitChannel7LH:
            break;
        case ParticleCounter::HOLDING_REG_0071_0072_UpperAlarmLimitChannel7LH:
            break;
        case ParticleCounter::HOLDING_REG_0073_WarningDelayChannel7:
            break;
        case ParticleCounter::HOLDING_REG_0074_AlarmDelayChannel7:
            break;
        case ParticleCounter::HOLDING_REG_0075_0076_UpperWarningLimitChannel8LH:
            break;
        case ParticleCounter::HOLDING_REG_0077_0078_UpperAlarmLimitChannel8LH:
            break;
        case ParticleCounter::HOLDING_REG_0079_WarningDelayChannel8:
            break;
        case ParticleCounter::HOLDING_REG_0080_AlarmDelayChannel8:
            break;
        case ParticleCounter::HOLDING_REG_0100_Command:
            break;
        default:
            break;
        }
    reg++;
    }
}

void ParticleCounter::slot_receivedInputRegisterData(quint64 telegramID, quint16 adr, quint16 reg, QList<quint16> data)
{
    Q_UNUSED(telegramID)

    if (adr != m_modbusAddress)
        return;

    markAsOnline();

    QDateTime samplingTimestamp = QDateTime();
    samplingTimestamp.setTimeSpec(Qt::UTC);
    QTime samplingTime = QTime();
    QDate samplingDate = QDate();
    quint16 seconds = 0;
    quint16 minutes = 0;
    quint16 hours = 0;
    quint16 days = 0;
    quint16 months = 0;
    quint16 years = 0;

    ArchiveDataset archiveDataset;

    foreach(quint16 rawdata, data)
    {
        // Clear strings if first byte of corrsponding string is received because more will follow to complete the string in the same response
        if (reg == ParticleCounter::INPUT_REG_0001_0048_DeviceInfoString)
            m_deviceInfo.deviceInfoString.clear();

        if (reg == ParticleCounter::INPUT_REG_0065_0080_DeviceIDString)
            m_deviceInfo.deviceIdString.clear();

        if (reg == ParticleCounter::INPUT_REG_0097_0112_PhysicalUnitString)
            m_physicalUnit.clear();


        switch (reg)
        {
        case ParticleCounter::INPUT_REG_0001_0048_DeviceInfoString ... (ParticleCounter::INPUT_REG_0001_0048_DeviceInfoString + 48 - 1):
            m_deviceInfo.deviceInfoString.append(QChar(rawdata));
            break;
        case ParticleCounter::INPUT_REG_0065_0080_DeviceIDString ... (ParticleCounter::INPUT_REG_0065_0080_DeviceIDString + 16 - 1):
            m_deviceInfo.deviceIdString.append(QChar(rawdata));
            break;
        case ParticleCounter::INPUT_REG_0082_ModbusRegistersetVersion:
            m_deviceInfo.modbusRegistersetVersion = QString().sprintf("%i.%i", rawdata / 100, rawdata % 100);
            break;
        case ParticleCounter::INPUT_REG_0089_StatusRegister:
            m_statusRegister.deviceActive = rawdata & (1 << 0);
            m_statusRegister.currentlySampling = rawdata & (1 << 1);
            m_statusRegister.currentlyRinsing = rawdata & (1 << 2);
            m_statusRegister.dataReady = rawdata & (1 << 3);
            break;
        case ParticleCounter::INPUT_REG_0096_ErrorstateRegister:
            m_errorstateRegister.temperatureError = rawdata & (1 << 0);
            m_errorstateRegister.sdCardError = rawdata & (1 << 1);
            m_errorstateRegister.counterSettings = rawdata & (1 << 2);
            m_errorstateRegister.acquisitionSettings = rawdata & (1 << 3);
            m_errorstateRegister.remoteSettings = rawdata & (1 << 4);
            m_errorstateRegister.filterSettings = rawdata & (1 << 5);
            m_errorstateRegister.detectorLoop = rawdata & (1 << 6);
            m_errorstateRegister.laserError = rawdata & (1 << 7);
            m_errorstateRegister.flowError = rawdata & (1 << 9);

            if (rawdata == 0)
            {
                m_actualData.statusString = "healthy";
                m_loghandler->slot_entryGone(LogEntry::Error, "Particle Counter id=" + QString().setNum(m_id), "Status error present.");
            }
            else
            {
                m_actualData.statusString = "problem";
                m_loghandler->slot_newEntry(LogEntry::Error, "Particle Counter id=" + QString().setNum(m_id), "Status error present.");
            }

            break;
        case ParticleCounter::INPUT_REG_0097_0112_PhysicalUnitString ... (ParticleCounter::INPUT_REG_0097_0112_PhysicalUnitString + 16 - 1):
            m_physicalUnit.append(QChar(rawdata));
            break;
        case ParticleCounter::INPUT_REG_0257_LivecountsTimestampSeconds:
            seconds = rawdata;
            break;
        case ParticleCounter::INPUT_REG_0258_LivecountsTimestampMinutes:
            minutes = rawdata;
            break;
        case ParticleCounter::INPUT_REG_0259_LivecountsTimestampHours:
            hours = rawdata;
            samplingTime.setHMS(hours, minutes, seconds);
            break;
        case ParticleCounter::INPUT_REG_0260_LivecountsTimestampDays:
            days = rawdata;
            break;
        case ParticleCounter::INPUT_REG_0261_LivecountsTimestampMonths:
            months = rawdata;
            break;
        case ParticleCounter::INPUT_REG_0262_LivecountsTimestampYears:
            years = rawdata + 2000;
            samplingDate.setDate(years, months, days);
            samplingTimestamp.setDate(samplingDate);
            samplingTimestamp.setTime(samplingTime);
            m_actualData.timestamp = samplingTimestamp;
            break;
        case ParticleCounter::INPUT_REG_0263_LivecountsChannel1Status:
            m_actualData.channelData[0].channel=1;
            m_actualData.channelData[0].status = (ChannelStatus) rawdata;
            break;
        case ParticleCounter::INPUT_REG_0264_0265_LivecountsChannel1LH:
            m_actualData.channelData[0].count = rawdata;
            break;
        case ParticleCounter::INPUT_REG_0264_0265_LivecountsChannel1LH + 1:
            m_actualData.channelData[0].count += rawdata << 16;
            break;
        case ParticleCounter::INPUT_REG_0266_LivecountsChannel2Status:
            m_actualData.channelData[1].channel=2;
            m_actualData.channelData[1].status = (ChannelStatus) rawdata;
            break;
        case ParticleCounter::INPUT_REG_0267_0268_LivecountsChannel2LH:
            m_actualData.channelData[1].count = rawdata;
            break;
        case ParticleCounter::INPUT_REG_0267_0268_LivecountsChannel2LH + 1:
            m_actualData.channelData[1].count += rawdata << 16;
            break;
        case ParticleCounter::INPUT_REG_0269_LivecountsChannel3Status:
            m_actualData.channelData[2].channel=3;
            m_actualData.channelData[2].status = (ChannelStatus) rawdata;
            break;
        case ParticleCounter::INPUT_REG_0270_0271_LivecountsChannel3LH:
            m_actualData.channelData[2].count = rawdata;
            break;
        case ParticleCounter::INPUT_REG_0270_0271_LivecountsChannel3LH + 1:
            m_actualData.channelData[2].count += rawdata << 16;
            break;
        case ParticleCounter::INPUT_REG_0272_LivecountsChannel4Status:
            m_actualData.channelData[3].channel=4;
            m_actualData.channelData[3].status = (ChannelStatus) rawdata;
            break;
        case ParticleCounter::INPUT_REG_0273_0274_LivecountsChannel4LH:
            m_actualData.channelData[3].count = rawdata;
            break;
        case ParticleCounter::INPUT_REG_0273_0274_LivecountsChannel4LH + 1:
            m_actualData.channelData[3].count += rawdata << 16;
            break;
        case ParticleCounter::INPUT_REG_0275_LivecountsChannel5Status:
            m_actualData.channelData[4].channel=5;
            m_actualData.channelData[4].status = (ChannelStatus) rawdata;
            break;
        case ParticleCounter::INPUT_REG_0276_0277_LivecountsChannel5LH:
            m_actualData.channelData[4].count = rawdata;
            break;
        case ParticleCounter::INPUT_REG_0276_0277_LivecountsChannel5LH + 1:
            m_actualData.channelData[4].count += rawdata << 16;
            break;
        case ParticleCounter::INPUT_REG_0278_LivecountsChannel6Status:
            m_actualData.channelData[5].channel=6;
            m_actualData.channelData[5].status = (ChannelStatus) rawdata;
            break;
        case ParticleCounter::INPUT_REG_0279_0280_LivecountsChannel6LH:
            m_actualData.channelData[5].count = rawdata;
            break;
        case ParticleCounter::INPUT_REG_0279_0280_LivecountsChannel6LH + 1:
            m_actualData.channelData[5].count += rawdata << 16;
            break;
        case ParticleCounter::INPUT_REG_0281_LivecountsChannel7Status:
            m_actualData.channelData[6].channel=7;
            m_actualData.channelData[6].status = (ChannelStatus) rawdata;
            break;
        case ParticleCounter::INPUT_REG_0282_0283_LivecountsChannel7LH:
            m_actualData.channelData[6].count = rawdata;
            break;
        case ParticleCounter::INPUT_REG_0282_0283_LivecountsChannel7LH + 1:
            m_actualData.channelData[6].count += rawdata << 16;
            break;
        case ParticleCounter::INPUT_REG_0284_LivecountsChannel8Status:
            m_actualData.channelData[7].channel=8;
            m_actualData.channelData[7].status = (ChannelStatus) rawdata;
            break;
        case ParticleCounter::INPUT_REG_0285_0286_LivecountsChannel8LH:
            m_actualData.channelData[7].count = rawdata;
            break;
        case ParticleCounter::INPUT_REG_0285_0286_LivecountsChannel8LH + 1:
            m_actualData.channelData[7].count += rawdata << 16;
            // INPUT_REG_0285_0286_LivecountsChannel8LH + 1 is the last data we get from automatic query, so signal new data now
            emit signal_ParticleCounterActualDataReceived(m_id, m_actualData);
            break;
        case ParticleCounter::INPUT_REG_0513_ArchiveDataSetTimestampSeconds:
            seconds = rawdata;
            break;
        case ParticleCounter::INPUT_REG_0514_ArchiveDataSetTimestampMinutes:
            minutes = rawdata;
            break;
        case ParticleCounter::INPUT_REG_0515_ArchiveDataSetTimestampHours:
            hours = rawdata;
            samplingTime.setHMS(hours, minutes, seconds);
            break;
        case ParticleCounter::INPUT_REG_0516_ArchiveDataSetTimestampDays:
            days = rawdata;
            break;
        case ParticleCounter::INPUT_REG_0517_ArchiveDataSetTimestampMonths:
            months = rawdata;
            break;
        case ParticleCounter::INPUT_REG_0518_ArchiveDataSetTimestampYears:
            years = rawdata + 2000;
            samplingDate.setDate(years, months, days);
            samplingTimestamp.setDate(samplingDate);
            samplingTimestamp.setTime(samplingTime);
            archiveDataset.timestamp = samplingTimestamp;
            break;
        case ParticleCounter::INPUT_REG_0519_ArchiveDataSetSamplingTimeInSeconds:
            archiveDataset.samplingTimeInSeconds = rawdata;
            break;
        case ParticleCounter::INPUT_REG_0520_ArchiveDataSetOutputDataFormat:
            archiveDataset.outputDataFormat = (OutputDataFormat) (rawdata & 0x01);
            archiveDataset.addupCount = (rawdata & 0xff) >> 2;
            break;
        case ParticleCounter::INPUT_REG_0521_ArchiveDataSetChannel1Status:
            archiveDataset.channelData[0].channel = 1;
            archiveDataset.channelData[0].status = (ChannelStatus) rawdata;
            break;
        case ParticleCounter::INPUT_REG_0522_0523_ArchiveDataSetChannel1LH:
            archiveDataset.channelData[0].count = rawdata;
            break;
        case ParticleCounter::INPUT_REG_0522_0523_ArchiveDataSetChannel1LH + 1:
            archiveDataset.channelData[0].count += rawdata << 16;
            break;
        case ParticleCounter::INPUT_REG_0524_ArchiveDataSetChannel2Status:
            archiveDataset.channelData[1].channel = 2;
            archiveDataset.channelData[1].status = (ChannelStatus) rawdata;
            break;
        case ParticleCounter::INPUT_REG_0525_0526_ArchiveDataSetChannel2LH:
            archiveDataset.channelData[1].count = rawdata;
            break;
        case ParticleCounter::INPUT_REG_0525_0526_ArchiveDataSetChannel2LH + 1:
            archiveDataset.channelData[1].count += rawdata << 16;
            break;
        case ParticleCounter::INPUT_REG_0527_ArchiveDataSetChannel3Status:
            archiveDataset.channelData[2].channel = 3;
            archiveDataset.channelData[2].status = (ChannelStatus) rawdata;
            break;
        case ParticleCounter::INPUT_REG_0528_0529_ArchiveDataSetChannel3LH:
            archiveDataset.channelData[2].count = rawdata;
            break;
        case ParticleCounter::INPUT_REG_0528_0529_ArchiveDataSetChannel3LH + 1:
            archiveDataset.channelData[2].count += rawdata << 16;
            break;
        case ParticleCounter::INPUT_REG_0530_ArchiveDataSetChannel4Status:
            archiveDataset.channelData[3].channel = 4;
            archiveDataset.channelData[3].status = (ChannelStatus) rawdata;
            break;
        case ParticleCounter::INPUT_REG_0531_0532_ArchiveDataSetChannel4LH:
            archiveDataset.channelData[3].count = rawdata;
            break;
        case ParticleCounter::INPUT_REG_0531_0532_ArchiveDataSetChannel4LH + 1:
            archiveDataset.channelData[3].count += rawdata << 16;
            break;
        case ParticleCounter::INPUT_REG_0533_ArchiveDataSetChannel5Status:
            archiveDataset.channelData[4].channel = 5;
            archiveDataset.channelData[4].status = (ChannelStatus) rawdata;
            break;
        case ParticleCounter::INPUT_REG_0534_0535_ArchiveDataSetChannel5LH:
            archiveDataset.channelData[4].count = rawdata;
            break;
        case ParticleCounter::INPUT_REG_0534_0535_ArchiveDataSetChannel5LH + 1:
            archiveDataset.channelData[4].count += rawdata << 16;
            break;
        case ParticleCounter::INPUT_REG_0536_ArchiveDataSetChannel6Status:
            archiveDataset.channelData[5].channel = 6;
            archiveDataset.channelData[5].status = (ChannelStatus) rawdata;
            break;
        case ParticleCounter::INPUT_REG_0537_0538_ArchiveDataSetChannel6LH:
            archiveDataset.channelData[5].count = rawdata;
            break;
        case ParticleCounter::INPUT_REG_0537_0538_ArchiveDataSetChannel6LH + 1:
            archiveDataset.channelData[5].count += rawdata << 16;
            break;
        case ParticleCounter::INPUT_REG_0539_ArchiveDataSetChannel7Status:
            archiveDataset.channelData[6].channel = 7;
            archiveDataset.channelData[6].status = (ChannelStatus) rawdata;
            break;
        case ParticleCounter::INPUT_REG_0540_0541_ArchiveDataSetChannel7LH:
            archiveDataset.channelData[6].count = rawdata;
            break;
        case ParticleCounter::INPUT_REG_0540_0541_ArchiveDataSetChannel7LH + 1:
            archiveDataset.channelData[6].count += rawdata << 16;
            break;
        case ParticleCounter::INPUT_REG_0542_ArchiveDataSetChannel8Status:
            archiveDataset.channelData[7].channel = 8;
            archiveDataset.channelData[7].status = (ChannelStatus) rawdata;
            break;
        case ParticleCounter::INPUT_REG_0543_0544_ArchiveDataSetChannel8LH:
            archiveDataset.channelData[7].count = rawdata;
            break;
        case ParticleCounter::INPUT_REG_0543_0544_ArchiveDataSetChannel8LH + 1:
            archiveDataset.channelData[7].count += rawdata << 16;
            if (archiveDataset.channelData[0].count != 0xffffffff)
                emit signal_ParticleCounterArchiveDataReceived(m_id, archiveDataset);
            break;
        default:
            break;
        }
        reg++;
    }
}

void ParticleCounter::slot_save()
{
    save();
}
