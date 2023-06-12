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