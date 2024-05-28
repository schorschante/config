__ESPHOME__

source scrpts/start_esphome
http://192.168.178.192:6052

__OctoPi__

service: /etc/systemd/system/octoprint.service
http://192.168.178.192:5000


__Influx-DB__

sudo service influxdb status

http://192.168.178.192:8086
