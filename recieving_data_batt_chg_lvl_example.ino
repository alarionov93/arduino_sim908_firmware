#include <SoftwareSerial.h>

SoftwareSerial SoftSerial(2,3);
int incomingByte = 0;   // for incoming serial data
char buff[50]="";

void setup() {
        Serial.begin(19200);     // opens serial port, sets data rate to 9600 bps
        SoftSerial.begin(19200);
}

void loop() {
    char * pch;
    char mode_str[4]="";
    char percent_str[4]="";
    char voltage[4]="";
    int i = 0;
    int mode, percent;
    memset(buff, '\0', 50);
    if (SoftSerial.available() > 0) {
        buff[i] = SoftSerial.read();
	i++;
    }
    if (strstr(buff, "+CBC") != NULL) {
        // if in interrupt, the code below may not work. so, print all buffer to
	// ensure that data recieved
	Serial.println(buff);
	pch = strtok(buff, ":");
        strcpy(mode, strtok(NULL, ","));
        strcpy(percent, strtok(NULL, ","));
        mode = atoi(mode);
        percent = atoi(percent);
    }
    // check chg lvl
}
