/******
Demo for ssd1306 i2c driver for  Raspberry Pi 
******/

#include "ssd1306_i2c.h"
#include <stdio.h>

float getAcTemp(){
	FILE* fp = fopen("/sys/bus/w1/devices/28-0114536602aa/w1_slave","r");
	if (fp == NULL){
		perror("Errore nel leggere la temperatura");
		char* text = "Errore nella lettura della temperatura della sonda \"Condizionatore\"";
		ssd1306_setTextSize(1);
		ssd1306_drawString(text);
		ssd1306_display();
	}
	
	char temp_output[1000];	
	while (fscanf(fp, "%s", temp_output)!=EOF){}
	fclose(fp);
	float temperature = 0.0;
	sscanf(temp_output,"t=%f",&temperature);
	temperature/=1000;
	//printf("%f\n",temperature);
	return temperature;
}

float getAmbTemp(){
	FILE* fp = fopen("/sys/bus/w1/devices/28-0114536858aa/w1_slave","r");
	if (fp == NULL){
		perror("Errore nel leggere la temperatura");
		char* text = "Errore nella lettura della temperatura della sonda \"Condizionatore\"";
		ssd1306_setTextSize(1);
		ssd1306_drawString(text);
		ssd1306_display();
	}
	
	char temp_output[1000];	
	while (fscanf(fp, "%s", temp_output)!=EOF){}
	fclose(fp);
	float temperature = 0.0;
	sscanf(temp_output,"t=%f",&temperature);
	temperature/=1000;
	//printf("%f\n",temperature);
	return temperature;
}

int main() {

	ssd1306_begin(SSD1306_SWITCHCAPVCC, SSD1306_I2C_ADDRESS);
	ssd1306_clearDisplay();
	ssd1306_setTextSize(2);
	while(1){
		char text[300];
		float ac = getAcTemp();
		float amb = getAmbTemp();
		sprintf(text,"Con:%.2f\nAmb:%.2f\n",ac,amb);
		ssd1306_drawString(text);
		ssd1306_display();
		ssd1306_clearDisplay();
		delay(300);
	}
	return 0;
}


