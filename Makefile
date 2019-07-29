CC=gcc
CFLAGS=-Wall -g -pipe -lwiringPi -lpthread

main: 
	$(CC) $(CFLAGS) -o temp_oled temp_oled.c ssd1306_i2c.c


clean: rm -f *.o
