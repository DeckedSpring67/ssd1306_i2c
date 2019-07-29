#!/bin/bash

if ! pgrep temp_oled >/dev/null
then
	echo "Temperature monitor is being restarted"
	/opt/ssd1306_i2c/temp_oled < /opt/ssd1306_i2c/recipients &
	/opt/ssd1306_i2c/setEmailSent.sh
else
	echo "Temperature monitor is already running"
fi


