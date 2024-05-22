# Cleanroom particle measurement daemon
This software interfaces particle measurement systems via ModBus to the time
series database influxDB. At the moment only niotronic aeromon particle
measurement systems are supported.

## Building and installing
First make sure to have Qt5 and openffucontrol-qtmodbus installed on your system.
Of course you need an instance of influxDB somewhere in your network running.
Create a directory for the build
```
mkdir bin
```

Create the Makefile
```
cd bin
qmake ../src
```

Compile the application wih a number of concurrent jobs of your choice

```
make -j 8
```

Install the application as root user
```
sudo make install
```

After the first installation copy the config example
```
sudo cp /etc/openffucontrol/particleserver/config.ini.example /etc/openffucontrol/particleserver/config.ini
```

Edit the configuration according to your needs
```
sudo vi /etc/openffucontrol/particleserver/config.ini
```

The daemon can be managed with systemctl
```
sudo systemctl start openffucontrol-particleserver
sudo systemctl status openffucontrol-particleserver
sudo systemctl stop openffucontrol-particleserver
```

Automatic start at boot time is controlled with 

```
sudo systemctl enable openffucontrol-particleserver
sudo systemctl disable openffucontrol-particleserver
```
Make sure the influx database is running before you start the
particleserver.

## Configuration
The config file for the daemin is located at */etc/openffucontrol/particleserver/config.ini*  
You can edit the file with any text editor of your choice.

Any change to the config requires the deamon to be restarted in order to load the new config.
```
sudo systemctl restart openffucontrol-particleserver
```

### Configuration terminal server
After the initial basic configuration in this file the openffucontrol-particleserver provides an interface for changes on the fly. 
With this interface particlecounters are defined, configured or deleted while the system is running. No restart is required to do so.

The interface is provided as a clear text terminal on a dedicated tcp *port*.

This is configured in the section \[tcpTerminal\].

Set the port to any unused port you want to use. Make sure to limit access by your firewall setting as there are no user privileges are needed.
Additional *restrictToLocalhost=1* denies any connection request form other machines. Normally this interface is used from localhost only.

You may use it with a simple tcp text terminal to connect to localhost port 16002 like
```
nc localhost 16002
```

In more complex setups any process that needs to automate insertion or deletion of particlecounters can connect here and do so in real time without stopping the server.
E.g. webservices can access the particleserver configuration here in order to manage the system, checkout particlecounters for calibration and so on.

The terminal privides an integrated user manual, just type help an enter.

### Time series database
This deamon is designed to write all measurement data to influxDB. Therefor influxDB needs to be installed on the same system or on a server that is reachable by network.
If the time series database is not located at the same system the hostname or ip address needs to be defined in the parameter hostname.
```
hostname=mySpecialInfluxDBhost.com
# or
hostname=10.1.4.4
```

The influx server is expected at port 8086. If it runs on a different port, that needs to be configured in the parameter port.

The deamon is going to use a database with a predefined name. This database needs to be created with the name dbName. It is mandatory to define the name of the database.

Also *username* and *password* must be defined if access is restricted by the database.

*measurementName* defines the name of the time series that will be used to store the time series. Make sure no other system writes to this time series in the defined database.
In highly complex setups you may have more then one openffucontrol-particleserver writing to the same database. In order to have different namespaces in that case you can use different measurementNames.
Otherwise you must make sure that any measurement id is not used by more then exactly one particlecounter.

### Serial interfaces
#### Modbus lines
Any basic configuration requires at least one RS485 bus line to be defined. This is done in the section \[interfacesParticleCounterModBus\]. 
Edit this section according to the external hardware loops you have. E.g. *pcmodbus0=ttyUSB0* creates the first modbus interface at the physical device */dev/ttyUSB0*. 
Any further interface has increased indices by 1, e.g. *pcmodbus1=ttyUSB1*. Of cource it can be wired to any other physical serial interface of your choice in case non-standard usb-RS485 interfaces are used.
E.g. use of a standard uart without USB connection would look like *pcmodbus1=ttyS1*.

The use of redundant RS485 connections is not yet supported. However it can already be built in hardware, the configuration shoud be non redundant for now and the second interface left unused.

#### Backoff time
The parameter *txDelay* defines additional waiting time after any received telegram is complete until the next telegram will be sent by the modbus master. 
This setting is 200 milliseconds by default and can be adjusted according to the time needed by the particle counters to detect a bus line as idle.
By modbus specification this setting may not be less then 4 milliseconds.

## Setting up particle counters
After the hardware loops are connected to the particlecounters and the daemon has been configured and started as described above, particlecounters need to be inserted in the database.

In the most simple way, you may want to connect by hand to the configuration terminal. Use the port that is configured in the config file to do so.
```
nc localhost 16002
```

Type help and enter to see the available commands. All commands are designed in a syntax like 
```
commandname parameter=value [additionalParamater=valueX] [...]
```
and are executed by pressing enter. Some commands my print a result.

Type *list-particlecounters* in order to see all configured particlecounters. This list should be empty on a newly installed system.

### Adding a particlecounter
Add your first particlecounter now. Therefor you need to know which modbus-line it is placed at and you need to know the modbus slave address of the particlecounter.
Also you must give a unique id to the particlecounter. This id is used later to insert data in the time series database. Make sure to remember which id belongs to which particlecounter.

For example a particlecounter with slave address 15 on modbus line 2 is added here as id 1:
```
add-particlecounter --bus=2 --unit=15 --id=1
```

Remember: This requires pcmodbus2 to be defined in */etc/openffucontrol/particleserver/config.ini* and will use the according serial interface for communication.
It will write measurementdata to the influxDB in tha database and measurementName defined in the config.ini and will use id=1 as primary key in that database.

Measurement is started as soon as the particlecounter is added and the configuration has been automatically downloaded to the particlecounter.

### Removing a particlecounter
Particlecounters that are not in use should be removed from the system as they consume communication time on the bus. 
Especially particlecounters thar are not present on the bus will consume a lot of time by waiting for the timeouts. This can degrade overall system performance.

Particlecounters are simply deleted by use of their *id* and *BUSNR*
```
delete-particlecounter --id=ID --bus=BUSNR
```

If only the *BUSNR* is given without id, all particlecounters of that bus loop are deleted.

### Viewing the measurement data
A live mode is implemented in order to show all measurement data as it is received. Simply type *startlive* and enter to start it.
Type *stoplive* and enter to stop it.

If you want to show measurement data of a single dedicated particlecounter, use the command get
```
get --id=1 --actual
```
to show the most basic measurement data of particlecounter id 1.

You can also get different key-value pairs by usind different keys in the request, e.g.
```
get --id=1 --lastSeen
```
will show the timestamp of the last communication event with particlecounter id 1.

The following keys are available with the command *get*:

- id
- busID
- unit
- online
- lostTelegrams
- lastSeen
- clockSettingLostCount
- deviceInfo
- deviceID
- modbusRegistersetVersion
- errorstring
- actual

## Alternative way of configuration
If particlecounters are configured with the use of the tcp terminal, the configuration data is stored in the directory */var/openffucontrol/particlecounters/*  
CSV-files are used here, one per particlecounter. The files are named by the id of each particlecounter.
In these files additional settings are possible and the files can be edited in order to configure a large number of particlecounters.
Please notice that the deamon must be stopped before any change to these files is performed and started again afterwards.

The followiong parameters are stored in these files by default:

- id
- bus
- modbusAddress
- clockSettingLostCount
- outputDataFormat
- addupCount
- firstRinsingTimeInSeconds
- subsequentRinsingTimeInSeconds
- samplingTimeInSeconds
- samplingEnabled

Refer to the particle counters user manual and the source code [particlecounter.cpp](https://github.com/sme-gmbh/openffucontrol-particleserver/blob/master/src/particlecounter.cpp) if changes to these paramaters are needed. You can set these parameters specifically for each particlecounter.

## System configuration backup
In case you want to create a full configuration backup of openffucontrol-particleserver, these directories need to be considered:  
- /var/openffucontrol/particlecounters/
- /etc/openffucontrol/particleserver/

# Time series database
The openffucontrol-particleserver needs influxDB as a time series database.
Install influxDB on your server or somewhere in your network in order to store measurement data.
Please refer to the [InfluxDB Documentation](https://docs.influxdata.com/influxdb/v2/get-started/setup/) for installation and maintenance. No special configuration is needed beyond creation of a database and setting up user credentials.

You might want to run influxDB on a redundant cluster or at least on a raid volume with redundancy. For backup methods please refer to the [influxDB Backup Section](https://docs.influxdata.com/influxdb/v2/admin/backup-restore/)

# Visual representation
You might want to use some sort of visual processing of the recorded measurements. We can recommend using the open source version of [Grafana](https://grafana.com/oss/grafana/) for that purpose as it has an interface for influxDB already included.  
Of course any other processing of the data is also possible as long as it is able to read data from influxDB.  
Please refer to the [influxDB Query Documentation](https://docs.influxdata.com/influxdb/v2/query-data/)

