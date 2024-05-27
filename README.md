----ESPHOME----

esphome dashboard esphome/config/


----OctoPi----

Automatic start up

Create a file /etc/systemd/system/octoprint.service and put this into it:

[Unit]
Description=The snappy web interface for your 3D printer
After=network-online.target
Wants=network-online.target

[Service]
Environment="LC_ALL=C.UTF-8"
Environment="LANG=C.UTF-8"
Type=exec
User=pi
ExecStart=/home/pi/OctoPrint/venv/bin/octoprint serve

[Install]
WantedBy=multi-user.target

Adjust the paths to your octoprint binary as needed. If you set it up in a virtualenv as described above make sure your /etc/systemd/system/octoprint.service looks like this:

ExecStart=/home/pi/OctoPrint/venv/bin/octoprint

Then add the script to autostart using sudo systemctl enable octoprint.service.

This will also allow you to start/stop/restart the OctoPrint daemon via

sudo service octoprint {start|stop|restart}
