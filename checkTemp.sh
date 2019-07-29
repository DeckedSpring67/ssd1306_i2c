#!/bin/bash

if ! pgrep temp_oled >/dev/null
then
	echo "Temperature monitor is being restarted"
	/opt/temp_oled/temp_oled &
else
	echo "Temperature monitor is already running"
fi


