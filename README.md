### ESPHOME
##### service: /etc/systemd/system/esphome.service
##### server:  http://192.168.178.192:6052

### OctoPi
##### service: /etc/systemd/system/octoprint.service
##### server:  http://192.168.178.192:5000

### Influx-DB
##### service: influxdb
##### server:  http://192.168.178.192:8086
##### pass: 1passwort



### Mosquitto MQTT
##### service: mosquitto

### Performance/Heat
sysbench --num-threads=4 --test=cpu --cpu-max-prime=10000 run
watch -n 2 vcgencmd measure_temp

### Grafana
##### service: grafana-server
##### server: http://192.168.178.192:3000
##### pass: 1passwort



### MQTT-->Influx Service
#####  service: solar_m2i.service
#####  service: moisture_m2i.service

### Node-Red
##### service: nodered.service
##### server: http://192.168.178.192:1880


### MQTT-->Influx Service
##### sudo journalctl -u moisture_m2i.service -b


