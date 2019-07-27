#!/bin/bash

if ! pgrep temp_oled >/dev/null
then
	echo "non va"
	/opt/ssd1306_i2c/temp_oled &
else
	echo "va"
fi


