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
        char mode[4]="";
        char percent[4]="";
        char voltage[4]="";
        memset(buff, '\0', 50);
        if (SoftSerial.available() > 0) {
                SoftSerial.readBytes(buff, 40);
        }
        if (strstr(buff, "+CBC") != NULL) {
//                Serial.println(buff);
            pch = strtok(buff, ":");
//            Serial.println(pch);
            strcpy(mode, strtok(NULL, ","));
            strcpy(percent, strtok(NULL, ","));
//            strcpy(mode, strtok(NULL, ","));
//            strcpy(percent, strtok(NULL, ","));
//            strcpy(voltage, strtok(NULL, "\r"));
            Serial.println(atoi(mode));
            Serial.println(atoi(percent));
//            Serial.println(buff);
        }
}
