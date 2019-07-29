#!/bin/bash

if ! pgrep temp_oled >/dev/null
then
	echo "Temperature monitor is being restarted"
	/opt/temp_oled/temp_oled < /opt/temp_oled/recipients &
	/opt/temp_oled/setEmailSent.sh
else
	echo "Temperature monitor is already running"
fi


