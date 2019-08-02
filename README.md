# Temperature Daemon

Program used to drive a ssd1306 in i2c paired with two DS18B20 temperature sensors that automatically shows them on screen, warns every mail in the **recipients** user created file and does stuff with pinouts.

## Installation

You need to create a recipients file somewhere and change the startup script *checkTemps* accordingly

You also need to symlink to your path checkTemps.sh into checkTemps and setEmailSent.sh to setEmailSent

### Requirements

Needs to be run on Linux with sendmail installed.


