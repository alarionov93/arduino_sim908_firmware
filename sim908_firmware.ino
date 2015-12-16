/**
SIM 908 MODULE ARDUINO FIRMWARE
//make short description

TODO list:
1. Write setup for all features of module
2. Error handling, set critical error reporting
3. Write main loop to get gps and send it

*/

#include <SoftwareSerial.h>

#define SIM_908_PWR_PIN 9
#define SS_RX 10
#define SS_TX 11
#define SIM_908_PWR_RATE 170
#define GSM_LED_PIN 4
#define GPS_LED_PIN 7
#define BUTTON 12
#define PWR_LED_PIN 8
int buttonState = 0;
int timer = 0;
bool DEBUG = true;
uint8_t answer=0;
char aux_str[100];

char pin[4]="";
char apn[29]="internet.beeline.ru";
char user_name[17]="beeline";
char password[17]="beeline";
char url[50]="http://a-larionov.ru:5000/gps";
char longitude[30]="";
char latitude[30]="";
char altitude[10]="";
char speed[11]="";
char frame[200]="";
char date[17]="";
char satellites[4]="";
char course[11]="";


SoftwareSerial mySerial(SS_RX, SS_TX); // RX, TX


int8_t sendATcommand(const char* ATcommand, const char* expected_answer, unsigned int timeout){

    uint8_t x=0,  answer=0;
    char response[100];
    unsigned long previous;

    memset(response, '\0', 100);    // Initialize empty string

    delay(100);

    while (Serial.available() > 0)
    {
      Serial.read();    // Clean the input buffer
    }

    Serial.print(ATcommand);    // Send the AT command 
    Serial.print("\r\n");

    x = 0;
    previous = millis();

    // this loop waits for the answer
    do {
        if(Serial.available() != 0)
        {
            // if there are data in the UART input buffer, reads it and checks for the asnwer
            response[x] = Serial.read();
            // printf("%c",response[x]); // i am fucked by this shit ALL BECAUSE OF YHIS FUCKING line!!!!!!!!!!
            x++;
            // check if the desired answer  is in the response of the module
            if (strstr(response, expected_answer) != NULL)    
            {
              answer = 1;
            }
        }
    }
    // Waits for the asnwer with time out
    while ((answer == 0) && ((millis() - previous) < timeout));    
    // Serial.println(response);

    return answer;
}

void ledFlash(int del, int pin, int count) {
  for (int i = 0; i < count; i++) {
    digitalWrite(pin, HIGH);
    delay(del);
    digitalWrite(pin, LOW);
    delay(del);
  }
}

void reboot() {
 uint8_t answer=0;

 answer = sendATcommand("AT+CFUN=1,1", "OK", 5000);
 if (answer == 0) {
   while(answer == 0){     // Send AT every two seconds and wait for the answer
       answer = sendATcommand("AT+CFUN=1,1", "OK", 5000);
     }
   }
}

void power_down() {
 uint8_t answer=0;
    
    // checks if the module is started
    answer = sendATcommand("AT+CPOWD=1", "NORMAL POWER DOWN", 2000);
    if (answer == 0)
    {
        while(answer == 0){     // Send AT every two seconds and wait for the answer
            answer = sendATcommand("AT+CPOWD=1", "NORMAL POWER DOWN", 2000);
        }
    }
}

void check_stat() {
 uint8_t answer=0;
    
  // checks if the module is started
  answer = sendATcommand("AT", "OK", 2000);
  if (answer == 0) {
    while(answer == 0){     // Send AT every two seconds and wait for the answer
      ledFlash(200, GSM_LED_PIN, 5);
      answer = sendATcommand("AT", "OK", 2000);
    }
  }
  if (answer == 1) {
    ledFlash(500, GSM_LED_PIN, 5);
  }
}

void gps_up() {
 uint8_t pwr=0;
 uint8_t rst=0;
 uint8_t stat=0;
 uint8_t gps_pwr_stat = 0;

  gps_pwr_stat = sendATcommand("AT+CGPSPWR?", "1", 2000); 

  if(gps_pwr_stat == 0) {
    ledFlash(100, GPS_LED_PIN, 5);
    pwr = sendATcommand("AT+CGPSPWR=1", "OK", 2000);
    rst = sendATcommand("AT+CGPSRST=1", "OK", 2000);

    while (pwr == 0 && rst == 0) { 
      pwr = sendATcommand("AT+CGPSPWR=1", "OK", 2000);
      rst = sendATcommand("AT+CGPSRST=1", "OK", 2000);
      ledFlash(1000, GPS_LED_PIN, 5); // TODO: set flashing for 1 second as a signal that smthng went wrong! //report critical error here!
    }
  } else {
    pwr = 1;
    rst = 1;
    ledFlash(100, GPS_LED_PIN, 2);
  }
  if(pwr == 1 && rst == 1) {
    stat = sendATcommand("AT+CGPSSTATUS?", "D Fix", 2000);
    while(stat == 0) {
      ledFlash(200, GPS_LED_PIN, 5);
      stat = sendATcommand("AT+CGPSSTATUS?", "D Fix", 2000);
      delay(60000);
    }
    if (stat) {
      digitalWrite(GPS_LED_PIN, HIGH); // if got gps satellites - gps led is ON
    }
  } else {
    // critical error here!
  }
}

void gsm_up() {
 uint8_t reg=0;
 uint8_t pin=0;
 uint8_t x=0;

  pin = sendATcommand("AT+CPIN?", "READY", 2000);
  reg = sendATcommand("AT+CREG?", "1,1", 1000);

  if (!pin) {
    pin = sendATcommand("AT+CPIN=1777", "READY", 2000); //TODO: pin code - ?
  }
  while (!reg) { 
    x++;
    ledFlash(200, GSM_LED_PIN, 5);
    reg = sendATcommand("AT+CREG?", "1,1", 1000);
    if (x == 10)
    {
      break;
    }
  }
  if (pin && reg) {
    digitalWrite(GSM_LED_PIN, HIGH);
    delay(4000);
    ledFlash(50, GSM_LED_PIN, 8);
    // if registered to the network - gsm led is ON
  }
  if (pin) {
    ledFlash(50, GSM_LED_PIN, 8);
  }
}

void gprs_up() {
  uint8_t gprs = 0;
  uint8_t stat_gprs = 0;

    // sendATcommand("AT+CSTT=\"internet.beeline.ru\",\"beeline\",\"beeline\"", "OK", 2000);
    // stat_gprs = sendATcommand("AT+SAPBR=4,1", "internet.beeline.ru", 2000);
    // if(stat_gprs == 1) {
    //   gprs = sendATcommand("AT+SAPBR=0,1", "OK", 2000);
    //   ledFlash(500, GSM_LED_PIN, 3);
    // }
    sendATcommand("AT+SAPBR=0,1", "OK", 2000); //always turn gprs off - to avoid if AT+SAPBR=1,1 returning ERROR

    sendATcommand("AT+SAPBR=3,1,\"APN\",\"internet.beeline.ru\"", "OK", 2000);
    
    sendATcommand("AT+SAPBR=3,1,\"USER\",\"beeline\"", "OK", 2000);
    
    sendATcommand("AT+SAPBR=3,1,\"PWD\",\"beeline\"", "OK", 2000);

    // gets the GPRS bearer
    gprs = sendATcommand("AT+SAPBR=1,1", "OK", 30000); //30 seconds is the minimal value!
    while (gprs == 0)
    {
      // sendATcommand("AT+CSTT=\"internet.beeline.ru\",\"beeline\",\"beeline\"", "OK", 2000);

      sendATcommand("AT+SAPBR=3,1,\"APN\",\"internet.beeline.ru\"", "OK", 2000);
    
      sendATcommand("AT+SAPBR=3,1,\"USER\",\"beeline\"", "OK", 2000);
    
      sendATcommand("AT+SAPBR=3,1,\"PWD\",\"beeline\"", "OK", 2000);

      gprs = sendATcommand("AT+SAPBR=1,1", "OK", 30000);
      ledFlash(200, GSM_LED_PIN, 5);
      delay(1000);
      ledFlash(50, GSM_LED_PIN, 10);
    }
    if (gprs == 1) {
      digitalWrite(GSM_LED_PIN, HIGH);
    }
}

int8_t getCoordinates(){

    int8_t counter, answer;
    long previous;

    // First get the NMEA string
    // Clean the input buffer
    while( Serial.available() > 0) Serial.read(); 
    // request Basic string
    sendATcommand("AT+CGPSINF=0", "AT+CGPSINF=0\r\n\r\n", 2000);

    counter = 0;
    answer = 0;
    memset(frame, '\0', 100);    // Initialize the string
    previous = millis();
    // this loop waits for the NMEA string
    do {
        if(Serial.available() != 0){    
            frame[counter] = Serial.read();
            counter++;
            // check if the desired answer is in the response of the module
            if (strstr(frame, "OK") != NULL)    
            {
                answer = 1;
            }
        }
        // Waits for the asnwer with time out
    }
    while((answer == 0) && ((millis() - previous) < 2000));  

    frame[counter-3] = '\0'; 
    
    // Parses the string 
    strtok(frame, ",");
    strcpy(longitude,strtok(NULL, ",")); // Gets longitude
    strcpy(latitude,strtok(NULL, ",")); // Gets latitude
    strcpy(altitude,strtok(NULL, ".")); // Gets altitude 
    strtok(NULL, ",");    
    strcpy(date,strtok(NULL, ".")); // Gets date
    strtok(NULL, ",");
    strtok(NULL, ",");  
    strcpy(satellites,strtok(NULL, ",")); // Gets satellites
    strcpy(speed,strtok(NULL, ",")); // Gets speed over ground. Unit is knots.
    strcpy(course,strtok(NULL, "\r")); // Gets course

    // convert2Degrees(latitude);
    // convert2Degrees(longitude);
    
    return answer;
}

  void sendCoordinates() {
  bool success = false;
    if (atof(longitude) > 0 && atof(latitude) > 0) {
      answer = sendATcommand("AT+HTTPINIT", "OK", 5000);
        if (answer == 1)
        {
        // Sets CID parameter
          answer = sendATcommand("AT+HTTPPARA=\"CID\",1", "OK", 2000);
          if (answer == 1)
          {
            // Sets url 
            sprintf(aux_str, "AT+HTTPPARA=\"URL\",\"%s", url);
            Serial.print(aux_str);
            // answer = sendATcommand(aux_str, "OK", 2000);
            sprintf(frame, "?lat=%s&lon=%s&alt=%s&speed=%s", latitude, longitude, altitude, speed);
            Serial.print(frame);
            answer = sendATcommand("\"", "OK", 5000);
            if (answer == 1)
            {
              answer = sendATcommand("AT+HTTPPARA=\"CONTENT\",\"application/x-www-form-urlencoded\"", "OK", 2000);
              if(answer == 1) {
                // Starts GET action
                answer = sendATcommand("AT+HTTPACTION=0", "+HTTPACTION:0,200", 10000);
                if (answer == 1) {
                  success = true;
                } else {
                  Serial.println(F("Error getting url"));
                  //here if url is not available, set old url a-larionov.ru:2222/data_parser.php
                }
              } else {
                Serial.println(F("Error setting cont type"));
              }
            } else {
              Serial.println(F("Error setting the url"));
            }
          } else {
            Serial.println(F("Error setting the CID"));
          }    
        } else {
          Serial.println(F("Error initializating"));
        }
      sendATcommand("AT+HTTPTERM", "OK", 5000);
      if (success == true)
      {
        ledFlash(80, GSM_LED_PIN, 20);
      }
    } else {
      ledFlash(80, GPS_LED_PIN, 20);
    }
  }

void setup() {
  pinMode(SIM_908_PWR_PIN, OUTPUT);
  pinMode(GPS_LED_PIN, OUTPUT);
  pinMode(GSM_LED_PIN, OUTPUT);

  Serial.begin(115200);
  // mySerial.begin(115200);
  delay(2000);
  gsm_up();
  delay(100);
  gprs_up();
  delay(100);
  gps_up();
}

void loop() {

  getCoordinates();
  sendCoordinates();
  delay(10000); //TODO: coordinates difference should depend on calculation this value!!

  //TODO: use check_stat and gsm_up here, to report if smthng is down during normal work of module

  // buttonState = digitalRead(BUTTON);
  // if (buttonState == HIGH)
  // {
  //   // timer++;
  //   // delay(10);
  //   analogWrite(SIM_908_PWR_PIN, SIM_908_PWR_RATE);
  // } else {
  //   // timer = 0;
  //   // delay(10);
  //   digitalWrite(SIM_908_PWR_PIN, 0);
  // }

  // delay(2000);
  // if (DEBUG == true && mySerial.available()) {
  //    Serial.write(mySerial.read());
  // }
  // if (Serial.available()) {
  //   mySerial.write(Serial.read());
  // }
}
