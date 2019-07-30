#!/bin/bash

if ! pgrep temp_oled >/dev/null
then
	echo "Temperature monitor is being restarted"
	setEmailSent
	temp_oled < /opt/temp_oled/recipients &
else
	echo "Temperature monitor is already running"
fi


