#include "ssd1306_i2c.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <wiringPi.h>

//#define RECIPIENT "teobalda005@gmail.com"
#define RECIPIENT "deckedspring@gmail.com"


typedef struct temps{
	float ac;
	float amb;
	float maxAC;
	float maxAMB;
}t_temps;

void send_email(t_temps *temps){
	//TODO change recipient to an array of recipients that we get from a file
	char* recipient = RECIPIENT;
	char cmd[300];  
	char to[100]; // email id of the recepient.
	strcpy(to,recipient);
	char body[400];    // email body.
	char tempFile[100];     // name of tempfile.

	strcpy(tempFile,tempnam("/tmp","sendmail")); // generate temp file name with tempnam.
	if(temps->ac > temps->maxAC)
		sprintf(body,"Attenzione!\nLa Temperatura del Condizionatore ha superato i %.3f gradi:\nTemperatura Condizionatore rilevata: %.3f gradi Celsius.\nTemperatura Ambiente rilevata: %.3f gradi Celsius",temps->maxAC,temps->ac,temps->amb);
	if(temps->amb > temps->maxAMB)
		sprintf(body,"Attenzione!\nLa Temperatura Ambiente ha superato i %.3f gradi:\nTemperatura Condizionatore rilevata: %.3f gradi Celsius.\nTemperatura Ambiente rilevata: %.3f gradi Celsius",temps->maxAMB,temps->ac,temps->amb);
	FILE *fp = fopen(tempFile,"w"); // open it for writing.
	fprintf(fp,"Subject: Rilevata temperatura Eccessiva\n%s\n",body);        // write body to it.
	fclose(fp);             // close it.

	sprintf(cmd,"sendmail %s < %s",to,tempFile); // prepare command.
	system(cmd);     // execute it.
	remove(tempFile); //remove TempFile
}

void *handle_HighTemp(void *args){
	t_temps *temps = (t_temps*)args;
	//Checks if it has already sent an email in the last 15 minutes (cron resets it to n)
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
	//If it hasn't been sent yet prepare the mail, to do that we need a tempfile where we will redirect it to sendmail
	if(hasBeenSent[0] == 'n'){
		//Modify hasSentMail, well it hasn't sent the email yet but it's going to
		//We need to do this in order to only one mail since this method will be called alot when the temperatures
		//exceed their limit
		checkSent = fopen("/tmp/hasSentMail","w"); 
		if (checkSent == NULL){
			perror("Errore nello scrivere lo stato della mail");
			char* text = "Errore nello scrivere lo stato della mail";
			ssd1306_setTextSize(1);
			ssd1306_drawString(text);
			ssd1306_display();
		}
		fprintf(checkSent,"y\n");
		fclose(checkSent);
		//After we modify the tmpfile we can proceed on actually sending the mail
		send_email(temps);
	}
	return NULL;
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

void *refresh_display(void *args){
	t_temps *temps = (t_temps*)args;
	char text[300];
	sprintf(text,"Con:%.2f\nAmb:%.2f\n",temps->ac,temps->amb);
	ssd1306_setTextSize(2);
	ssd1306_drawString(text);
	ssd1306_display();
	ssd1306_clearDisplay();
	return NULL;
}

int main() {
	ssd1306_begin(SSD1306_SWITCHCAPVCC, SSD1306_I2C_ADDRESS);
	ssd1306_clearDisplay();
	pthread_t refresh_id; 
	pthread_t email_id;
	t_temps temps;

	temps.maxAC = 32.00;
	temps.maxAMB = 50.00;

	while(1){
		temps.ac = getAcTemp();
		temps.amb = getAmbTemp();
		pthread_create(&refresh_id, NULL, refresh_display, &temps);
		if(temps.ac > temps.maxAC || temps.amb > temps.maxAMB){	
			pthread_create(&email_id, NULL, handle_HighTemp, &temps);
		}
		//Detach the thread to avoid memory leak
		pthread_detach(refresh_id);
		pthread_detach(email_id);
		delay(500);
	}
	return 0;
}

