#include "ssd1306_i2c.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
//#define RECIPIENT "teobalda005@gmail.com"
#define RECIPIENT "deckedspring@gmail.com"

void sendMail(float ac, float amb, char* recipient){
	FILE *checkSent = fopen("/tmp/hasSentMail","r");
	if (checkSent == NULL){
		perror("Errore nel leggere lo stato della mail");
		char* text = "Errore nell'invio della mail";
		ssd1306_setTextSize(1);
		ssd1306_drawString(text);
		ssd1306_display();
	}
	char hasBeenSent[10];
	fscanf(checkSent,"%s",hasBeenSent);
	fclose(checkSent);
	if(hasBeenSent[0] == 'n'){
		char cmd[300];  
        	char to[100]; // email id of the recepient.
		strcpy(to,recipient);
        	char body[300];    // email body.
        	char tempFile[100];     // name of tempfile.

        	strcpy(tempFile,tempnam("/tmp","sendmail")); // generate temp file name.
		sprintf(body,"Attenzione!\nTemperatura Condizionatore: %.3f\nTemperatura Ambiente: %.3f",ac,amb);
        	FILE *fp = fopen(tempFile,"w"); // open it for writing.
        	fprintf(fp,"%s\n",body);        // write body to it.
        	fclose(fp);             // close it.

        	sprintf(cmd,"sendmail %s < %s",to,tempFile); // prepare command.
        	system(cmd);     // execute it.

		//Modify hasSentMail
		checkSent = fopen("/tmp/hasSentMail","w"); 
		if (checkSent == NULL){
			perror("Errore nello scrivere lo stato della mail");
			char* text = "Errore nello scrivere lo stato della mail";
			ssd1306_setTextSize(1);
			ssd1306_drawString(text);
			ssd1306_display();
		}
		fprintf(checkSent,"y");
		fclose(checkSent);
	}
}


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
	return temperature;
}

int main() {
	ssd1306_begin(SSD1306_SWITCHCAPVCC, SSD1306_I2C_ADDRESS);
	ssd1306_clearDisplay();
	while(1){
		char text[300];
		float ac = getAcTemp();
		float amb = getAmbTemp();
		if(ac > 32.00){	
		//	printf("Condizionatore > 32gradi\n");
			sendMail(ac,amb,RECIPIENT);
		}
		sprintf(text,"Con:%.2f\nAmb:%.2f\n",ac,amb);
		ssd1306_setTextSize(2);
		ssd1306_drawString(text);
		ssd1306_display();
		ssd1306_clearDisplay();
		usleep(300000);
	}
	return 0;
}

