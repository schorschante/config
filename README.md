### ESPHOME
source scrpts/start_esphome

http://192.168.178.192:6052

### OctoPi
service: /etc/systemd/system/octoprint.service

http://192.168.178.192:5000


### Influx-DB
sudo service influxdb status

http://192.168.178.192:8086

admin1passwort

### Mosquitto MQTT
sudo systemctl enable mosquitto

### Performance/Heat
sysbench --num-threads=4 --test=cpu --cpu-max-prime=10000 run

watch -n 2 vcgencmd measure_temp

### Grafana
http://192.168.178.192:3000
admin:1passwort


