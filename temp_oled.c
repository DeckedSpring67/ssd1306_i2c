#include "ssd1306_i2c.h"
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <wiringPi.h>
#include <limits.h>

//Constants
static int const mailInterval = 15;
static int const lightInterval = 60;
static double const maxAC = 32;
static double const maxAMB = 32;
static char* const RECIPIENTS = "/opt/temp_oled/recipients";

typedef struct temps{
	float ac;
	float amb;
	float maxAC;
	float maxAMB;
	time_t timeLastSent;
	int isUrgent;
	char *recipients; //Path for recipients file
}t_temps;


void closeRelay1(){
	digitalWrite(28,LOW);
}

void closeRelay2(){
	digitalWrite(29,LOW);
}

void openRelay1(){
	digitalWrite(28,HIGH);
}

void openRelay2(){
	digitalWrite(29,HIGH);
}

void handleInterrupt(){
	openRelay1();
	openRelay2();
}


void send_email(t_temps *temps){
	char cmd[300];  
	char sender[50];
	char body[400];    // email body.

	//remove non printable characters
	if(temps->ac > temps->maxAC)
		sprintf(body,"Attenzione!\nLa Temperatura del Condizionatore ha superato i %.3f gradi:\nTemperatura Condizionatore rilevata: %.3f gradi Celsius.\nTemperatura Ambiente rilevata: %.3f gradi Celsius",temps->maxAC,temps->ac,temps->amb);
	if(temps->amb > temps->maxAMB)
		sprintf(body,"Attenzione!\nLa Temperatura Ambiente ha superato i %.3f gradi:\nTemperatura Condizionatore rilevata: %.3f gradi Celsius.\nTemperatura Ambiente rilevata: %.3f gradi Celsius",temps->maxAMB,temps->ac,temps->amb);
	char tempFile[100] = "/tmp/sendm.XXXXXX";
	int fd = mkstemp(tempFile);
	FILE *fp = fdopen(fd,"w"); // open it for writing.
	
	if(temps->isUrgent){
		fprintf(fp,"X-priority: 1\n");
		sprintf(sender,"urgente");
	}else{
		fprintf(fp,"X-priority: 5\n");
		sprintf(sender,"allerta");
	}
	fprintf(fp,"Subject: Rilevata temperatura Eccessiva\n%s\n",body);        // write body to it.
	fclose(fp);             // close it.
	close(fd);

	sprintf(cmd,"/usr/sbin/sendmail -f %s $(cat %s) < %s",sender,temps->recipients,tempFile); // prepare command.
	system(cmd);     // execute it.
	remove(tempFile); //remove TempFile
}

void *handle_HighTemp(void *args){
	t_temps *temps = (t_temps*)args;
	//Close Relay1 / Relay2
	
	closeRelay1();
	closeRelay2();
	
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
		time(&(temps->timeLastSent));
		temps->isUrgent = 0;
		send_email(temps);
	}else{
		time_t timeNow;
		time(&timeNow);
		double timeDiff = difftime(timeNow,(temps->timeLastSent)); 
		//TODO Change to 15 back again
		if((timeDiff/60) > mailInterval){
			temps->isUrgent = 1;
			//Keep Relays closed
			closeRelay1();
			closeRelay2();
			time(&(temps->timeLastSent));
			send_email(temps);
		}
	}
	return NULL;
}


void sendLowTempEmail(t_temps *temps){
	char cmd[300];  
	char body[400];    // email body.

	//remove non printable characters
	char tempFile[100] = "/tmp/sendm.XXXXXX";     // name of tempfile.
	int fd = mkstemp(tempFile);
	FILE *fp = fdopen(fd,"w"); // open it for writing.
	//Write body
	sprintf(body,"X-Priority: 5\n");
	fprintf(fp,"%s",body);
	sprintf(body,"Subject: Temperatura rientrata nei valori nominali\n");        
	fprintf(fp,"%s",body);
	sprintf(body,"Temperatura Condizionatore rilevata: %.3f gradi Celsius.\nTemperatura Ambiente rilevata: %.3f gradi Celsius\n",temps->ac,temps->amb);
	fprintf(fp,"%s",body);
	fclose(fp);             // close it.
	close(fd);

	sprintf(cmd,"/usr/sbin/sendmail -f notifica $(cat %s) < %s",temps->recipients,tempFile); // prepare command.
	system(cmd);     // execute it.
	//remove(tempFile); //remove TempFile
}

void *handle_LowTemp(void *args){
	t_temps *temps = (t_temps*)args;
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
	if(hasBeenSent[0] == 'y'){
		system("setEmailSent");
		sendLowTempEmail(temps);
		time(&(temps->timeLastSent));
		//Keep Relay2 closed but open Relay1
		openRelay1();
		closeRelay2();
	}
	if(temps->timeLastSent){
		time_t timeNow;
		time(&(timeNow));
		double timeDiff = difftime(timeNow,(temps->timeLastSent)); 
		//After lightInterval we open both relays
		if((timeDiff / 60) > lightInterval){
			openRelay1();
			openRelay2();
		}
	}
	return NULL;	
}



void *getAcTemp(void *args){
	t_temps *temps  = (t_temps*)args;
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
	temps->ac = temperature;
	return NULL;
}

void *getAmbTemp(void *args){
	t_temps *temps  = (t_temps*)args;
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
	temps->amb = temperature;
	return NULL;
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
	//setup pins and other stuff
	wiringPiSetup();
	//Interrupt for button
	wiringPiISR(27,INT_EDGE_RISING,&handleInterrupt);
	//Relay1
	pinMode(28,OUTPUT);
	//Relay2
	pinMode(29,OUTPUT);
	openRelay1();
	openRelay2();
	
	
	//Reset emailSent status
	system("setEmailSent");
	
	printf("Temperature checking daemon started\n");
	ssd1306_begin(SSD1306_SWITCHCAPVCC, SSD1306_I2C_ADDRESS);
	ssd1306_clearDisplay();
	
	//Create thread ids
	pthread_t refresh_id; 
	pthread_t hightemps_id;
	pthread_t ac_id;
	pthread_t amb_id;
	pthread_t lowtemps_id;

	t_temps temps;
	temps.maxAC = maxAC;
	temps.maxAMB = maxAMB;
	//Set recipients path
	//TODO implement sys argument
	temps.recipients = RECIPIENTS;

	temps.timeLastSent = 0;

	//Add a slight delay after the first reading to prevent wrong ones
	pthread_create(&ac_id, NULL, getAcTemp, &temps);
	pthread_create(&amb_id, NULL, getAmbTemp, &temps);
	delay(1000);

	while(1){
		pthread_create(&ac_id, NULL, getAcTemp, &temps);
		pthread_create(&amb_id, NULL, getAmbTemp, &temps);
		
		//Detach threads to avoid memory leak
		pthread_join(ac_id,NULL);
		pthread_join(amb_id,NULL);

		pthread_create(&refresh_id, NULL, refresh_display, &temps);
		pthread_detach(refresh_id);

		if(temps.ac > temps.maxAC || temps.amb > temps.maxAMB){	
			pthread_create(&hightemps_id, NULL, handle_HighTemp, &temps);
		}
		else if(temps.ac < (temps.maxAC - 1) && temps.amb < (temps.maxAMB - 1)){
			pthread_create(&lowtemps_id, NULL, handle_LowTemp, &temps);
		}
		pthread_detach(lowtemps_id);
		pthread_detach(hightemps_id);
		delay(500);
	}
	return 0;
}

