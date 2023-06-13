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

#ifndef PARTICLECOUNTER_H
#define PARTICLECOUNTER_H

#include <QObject>
#include <QObject>
#include <QMap>
#include <QDateTime>
#include "particlecountermodbussystem.h"
#include "loghandler.h"

class ParticleCounter : public QObject
{
    Q_OBJECT
public:
    explicit ParticleCounter(QObject *parent, ParticleCounterModbusSystem *pcModbusSystem, Loghandler* loghandler);
    ~ParticleCounter();

    #define MODBUS_FFU_BLOCKSIZE 0x10

    typedef enum {
        HOLDING_REG_0001_AlarmEnable = 0,
        HOLDING_REG_0002_OutputDataFormat= 1,
        HOLDING_REG_0003_FirstRinsingTimeInSeconds = 2,
        HOLDING_REG_0004_SubsequentRinsingTimeInSeconds = 3,
        HOLDING_REG_0005_SamplingTimeInSeconds = 4,
        HOLDING_REG_0017_RtcSeconds = 16,
        HOLDING_REG_0018_RtcMinutes = 17,
        HOLDING_REG_0019_RtcHours = 18,
        HOLDING_REG_0020_RtcDays = 19,
        HOLDING_REG_0021_RtcMonths = 20,
        HOLDING_REG_0022_RtcYears = 21,
        HOLDING_REG_0033_0034_UpperWarningLimitChannel1LH = 32,
        HOLDING_REG_0035_0036_UpperAlarmLimitChannel1LH = 34,
        HOLDING_REG_0037_WarningDelayChannel1 = 36,
        HOLDING_REG_0038_AlarmDelayChannel1 = 37,
        HOLDING_REG_0039_0040_UpperWarningLimitChannel2LH = 38,
        HOLDING_REG_0041_0042_UpperAlarmLimitChannel2LH = 40,
        HOLDING_REG_0043_WarningDelayChannel2 = 42,
        HOLDING_REG_0044_AlarmDelayChannel2 = 43,
        HOLDING_REG_0045_0046_UpperWarningLimitChannel3LH = 44,
        HOLDING_REG_0047_0048_UpperAlarmLimitChannel3LH = 46,
        HOLDING_REG_0049_WarningDelayChannel3 = 48,
        HOLDING_REG_0050_AlarmDelayChannel3 = 49,
        HOLDING_REG_0051_0052_UpperWarningLimitChannel4LH = 50,
        HOLDING_REG_0053_0054_UpperAlarmLimitChannel4LH = 52,
        HOLDING_REG_0055_WarningDelayChannel4 = 54,
        HOLDING_REG_0056_AlarmDelayChannel4 = 55,
        HOLDING_REG_0057_0058_UpperWarningLimitChannel5LH = 56,
        HOLDING_REG_0059_0060_UpperAlarmLimitChannel5LH = 58,
        HOLDING_REG_0061_WarningDelayChannel5 = 60,
        HOLDING_REG_0062_AlarmDelayChannel5 = 61,
        HOLDING_REG_0063_0064_UpperWarningLimitChannel6LH = 62,
        HOLDING_REG_0065_0066_UpperAlarmLimitChannel6LH = 64,
        HOLDING_REG_0067_WarningDelayChannel6 = 66,
        HOLDING_REG_0068_AlarmDelayChannel6 = 67,
        HOLDING_REG_0069_0070_UpperWarningLimitChannel7LH = 68,
        HOLDING_REG_0071_0072_UpperAlarmLimitChannel7LH = 70,
        HOLDING_REG_0073_WarningDelayChannel7 = 72,
        HOLDING_REG_0074_AlarmDelayChannel7 = 73,
        HOLDING_REG_0075_0076_UpperWarningLimitChannel8LH = 74,
        HOLDING_REG_0077_0078_UpperAlarmLimitChannel8LH = 76,
        HOLDING_REG_0079_WarningDelayChannel8 = 78,
        HOLDING_REG_0080_AlarmDelayChannel8 = 79,
        HOLDING_REG_0100_Command = 99
    } ParticleCounterModbusHoldingRegister;

    typedef enum {
        COMMAND_0001_SetClock = 1,
        COMMAND_0008_SaveAlarmRegistersToNonvolatileMemory = 8,
        COMMAND_0009_SaveAcquisitionRegistersToNonvolatileMemory = 9,
        COMMAND_0016_StopAcquisition = 16,
        COMMAND_0017_StartAcquisition = 17,
        COMMAND_0099_LoadNextArchiveDataSet = 99
    } ParticleCounterCommand;

    typedef enum {
        INPUT_REG_0001_0048_DeviceInfoString = 0,
        INPUT_REG_0065_0080_DeviceIDString = 64,
        INPUT_REG_0082_ModbusRegistersetVersion = 81,
        INPUT_REG_0089_StatusRegister = 88,
        INPUT_REG_0096_ErrorstateRegister = 95,
        INPUT_REG_0097_0112_PhysicalUnitString = 96,
        INPUT_REG_0257_LivecountsTimestampSeconds = 256,
        INPUT_REG_0258_LivecountsTimestampMinutes = 257,
        INPUT_REG_0259_LivecountsTimestampHours = 258,
        INPUT_REG_0260_LivecountsTimestampDays = 259,
        INPUT_REG_0261_LivecountsTimestampMonths = 260,
        INPUT_REG_0262_LivecountsTimestampYears = 261,
        INPUT_REG_0263_LivecountsChannel1Status = 262,
        INPUT_REG_0264_0265_LivecountsChannel1LH = 263,
        INPUT_REG_0266_LivecountsChannel2Status = 265,
        INPUT_REG_0267_0268_LivecountsChannel2LH = 266,
        INPUT_REG_0269_LivecountsChannel3Status = 268,
        INPUT_REG_0270_0271_LivecountsChannel3LH = 269,
        INPUT_REG_0272_LivecountsChannel4Status = 271,
        INPUT_REG_0273_0274_LivecountsChannel4LH = 272,
        INPUT_REG_0275_LivecountsChannel5Status = 274,
        INPUT_REG_0276_0277_LivecountsChannel5LH = 275,
        INPUT_REG_0278_LivecountsChannel6Status = 277,
        INPUT_REG_0279_0280_LivecountsChannel6LH = 278,
        INPUT_REG_0281_LivecountsChannel7Status = 280,
        INPUT_REG_0282_0283_LivecountsChannel7LH = 281,
        INPUT_REG_0284_LivecountsChannel8Status = 283,
        INPUT_REG_0285_0286_LivecountsChannel8LH = 284,
        INPUT_REG_0513_ArchiveDataSetTimestampSeconds = 512,
        INPUT_REG_0514_ArchiveDataSetTimestampMinutes = 513,
        INPUT_REG_0515_ArchiveDataSetTimestampHours = 514,
        INPUT_REG_0516_ArchiveDataSetTimestampDays = 515,
        INPUT_REG_0517_ArchiveDataSetTimestampMonths = 516,
        INPUT_REG_0518_ArchiveDataSetTimestampYears = 517,
        INPUT_REG_0519_ArchiveDataSetSamplingTimeInSeconds = 518,
        INPUT_REG_0520_ArchiveDataSetOutputDataFormat = 519,
        INPUT_REG_0521_ArchiveDataSetChannel1Status = 520,
        INPUT_REG_0522_0523_ArchiveDataSetChannel1LH = 521,
        INPUT_REG_0524_ArchiveDataSetChannel2Status = 523,
        INPUT_REG_0525_0526_ArchiveDataSetChannel2LH = 524,
        INPUT_REG_0527_ArchiveDataSetChannel3Status = 526,
        INPUT_REG_0528_0529_ArchiveDataSetChannel3LH = 527,
        INPUT_REG_0530_ArchiveDataSetChannel4Status = 529,
        INPUT_REG_0531_0532_ArchiveDataSetChannel4LH = 530,
        INPUT_REG_0533_ArchiveDataSetChannel5Status = 532,
        INPUT_REG_0534_0535_ArchiveDataSetChannel5LH = 533,
        INPUT_REG_0536_ArchiveDataSetChannel6Status = 535,
        INPUT_REG_0537_0538_ArchiveDataSetChannel6LH = 536,
        INPUT_REG_0539_ArchiveDataSetChannel7Status = 538,
        INPUT_REG_0540_0541_ArchiveDataSetChannel7LH = 539,
        INPUT_REG_0542_ArchiveDataSetChannel8Status = 541,
        INPUT_REG_0543_0544_ArchiveDataSetChannel8LH = 542
    } ParticleCounterModbusInputRegister;

    typedef enum {
        OFF = 0,
        OK = 1,
        WARNING = 2,
        ALARM = 3
    } ChannelStatus;

    typedef struct {
        ChannelStatus status;
        quint16 channel;
        quint32 count;
    } ChannelData;

    typedef enum {
        DISTRIBUTIVE = 0,
        CUMULATIVE = 1
    } OutputDataFormat;

    typedef struct {
        bool online;
        quint64 lostTelegrams;
        QDateTime lastSeen;
        quint32 clockSettingLostCount;
        QString statusString;
        ChannelData channelData[8];
        QDateTime timestamp;
    } ActualData;

    typedef struct {
        QDateTime timestamp;
        quint16 samplingTimeInSeconds;
        OutputDataFormat outputDataFormat;
        quint16 addupCount;
        ChannelData channelData[8];
    } ArchiveDataset;

    typedef struct {
        QString deviceInfoString;
        QString deviceIdString;
        QString modbusRegistersetVersion;
    } DeviceInfo;

    typedef struct {
        OutputDataFormat outputDataFormat;
        quint16 addupCount;
        quint16 firstRinsingTimeInSeconds;
        quint16 subsequentRinsingTimeInSeconds;
        quint16 samplingTimeInSeconds;
        bool valid;
    } ConfigData;

    typedef struct {
        bool deviceActive;
        bool currentlySampling;
        bool currentlyRinsing;
        bool dataReady;
    } StatusRegister;

    typedef struct {
        bool temperatureError;
        bool sdCardError;
        bool counterSettings;
        bool acquisitionSettings;
        bool remoteSettings;
        bool filterSettings;
        bool detectorLoop;
        bool laserError;
        bool flowError;
    } ErrorstateRegister;

    // Central id from the openFFUcontrol database
    int getId() const;
    void setId(int id);

    // Number of the RS485 bus the OCU is connected to
    int getBusID() const;
    void setBusID(int busID);

    // Modbus address of that particular particlecounter
    int getModbusAddress() const;
    void setModbusAddress(int modbusAddress);

    // Do all the initialization to get operational
    void init();

    // Get or set any data by name
    QString getData(QString key);
    void setData(QString key, QString value);

    // Start or Stop Sampling
    void setSamplingEnabled(bool on);
    bool isSampling() const;

    // Write acquisition parameters to permanent storage in order to load them at next startup
    void storeSettingsToFlash();

    // Get a list of data keys that this FFU can provide
    QStringList getActualKeys();

    // Get all actual data that this FFU can provide
    ActualData getActualData() const;

    // This function triggers bus requests to get information about the device (serial number etc.)
    void requestDeviceInfo();

    // This function triggers bus requests to get actual values, status, warnings and errors
    void requestStatus();

    // This function triggers bus requests to get current set of archive data values
    void requestArchiveDataset();

    // This function triggers bus request to switch register content to next available archive data values
    void requestNextArchive();

    // This function triggers bus requests to get the necessary config data from the particle counter
    void requestConfig();

    // This function triggers bus requests to set the necessary config data in the particle counter
    void setConfigData(ConfigData data);

    // This function triggers bus request to get current time from real time clock of the particle counter
    void requestClock();

    // This function triggers bus request to set current time in real time clock of the particle counter
    void setClock();

    // Save the setpoints and config to file
    void save();
    void setFiledirectory(QString path);

    // Load setpoints and config from file
    void load(QString filename);

    // Set if changes of important setpoints and config should automatically updated on file if changed
    void setAutoSave(bool on);

    void deleteFromHdd();

    // Tell loghandler that Errors are gone
    void deleteAllErrors();

    // Check if a modbus telegram id corresponds to a request from this FFU
    bool isThisYourTelegram(quint64 telegramID, bool deleteID = true);

private:
    ParticleCounterModbusSystem* m_pcModbusSystem;
    Loghandler* m_loghandler;
    QList<quint64> m_transactionIDs;

    int m_id;
    int m_busID;
    int m_modbusAddress;   // Modbus Data Address

    ActualData m_actualData;
    ConfigData m_configData;
    DeviceInfo m_deviceInfo;
    StatusRegister m_statusRegister;
    ErrorstateRegister m_errorstateRegister;
    QString m_physicalUnit;
    bool m_samplingEnabled;

    bool m_dataChanged;
    bool m_autosave;
    QString m_filepath;

    QString myFilename();

    bool isConfigured();    // Returns false if either fanAddress or busID is not set
    void markAsOnline();

    // This uses m_configData to configure particlecounter
    void processConfigData();

signals:
    void signal_needsSaving();
    void signal_ParticleCounterActualDataHasChanged(int id);
    void signal_ParticleCounterArchiveDataReceived(int id, ArchiveDataset archiveData);

public slots:
    // High level bus response slots
    void slot_transactionLost(quint64 id);
    void slot_receivedHoldingRegisterData(quint64 telegramID, quint16 adr, quint16 reg, QList<quint16> data);
    void slot_receivedInputRegisterData(quint64 telegramID, quint16 adr, quint16 reg, QList<quint16> data);

private slots:
    void slot_save();
};

#endif // PARTICLECOUNTER_H
